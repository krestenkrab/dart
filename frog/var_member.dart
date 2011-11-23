// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

/** A dynamic member stub. */
class VarMember {
  final String name;

  VarMember(this.name);

  abstract void generate(CodeWriter code);

  Type get returnType() => world.varType;

  Value invoke(MethodGenerator context, Node node, Value target, Arguments args) {
    return new Value(returnType,
      '${target.code}.$name(${args.getCode()})', node.span);
  }
}

/**
 * This function generates a dynamic call stub for functions. It's part of a
 * series of steps described below. Most of the code is generated by
 * gen.dart, with some helpers in core.js
 *
 * Given a call site in Dart like:
 *   f(1, 2, capture:true);
 *
 * We compile to JS like:
 *   f.call$2$capture(1, 2, true);
 *
 * And then generate this function:
 *   Function.prototype.call$2$capture = function($0, $1, capture) {
 *     this.call$2$capture = this.$genStub(3, ['capture']);
 *     return this.call$2$capture($0, $1, capture);
 *   }
 *
 * Or for a fixed-arity function, generate this:
 *   Function.prototype.call$2 = function($0, $1) {
 *     return this.to$call$2()($0, $1);
 *   }
 *   Function.prototype.to$call$2 = function() {
 *     this.call$2 = this.$genStub(2);
 *     this.to$call$2 = function() { return this.call$2; }
 *     return this.to$call$2();
 *   }
 * We use .to$call$2 to convert to a typed function.
 *
 * For each method that can be passed as a function, such as a "get" on the
 * method or is a lambda, generate optional argument info. Given a function
 * like:
 *   class SomeType {
 *     void add(x, y, [bubbles = true, capture = false]) { ... }
 *     ... }
 *
 * The generated argument info looks like:
 *  SomeType.prototype.add.$optional = ['bubbles','capture', 'true','false'];
 */
// TODO(jmesserly): we don't currently put $optional on lambdas.
// Also, maybe a string encoding would perform better?
// TODO(jmesserly): $genStub is a hole in the run-time type checker.
// It bypasses the checks we would do at the callsite for methods.
// Also, it won't work properly for native JS functions (those don't have
// an accurate .length)
class VarFunctionStub extends VarMember {
  final Arguments args;

  VarFunctionStub(String name, Arguments callArgs)
    : super(name), args = callArgs.toCallStubArgs() {
    world.gen.corejs.useGenStub = true;
  }

  void generate(CodeWriter code) {
    if (args.hasNames) {
      generateNamed(code);
    } else {
      generatePositional(code);
    }
  }

  void generatePositional(CodeWriter w) {
    // Positional arg functions can be converted from "var" to a fixed arity
    // function type. So emit a to$N stub as well as the call$N stub.
    int arity = args.length;
    w.enterBlock('Function.prototype.to\$$name = function() {');
    w.writeln('this.$name = this.\$genStub($arity);');
    w.writeln('this.to\$$name = function() { return this.$name; };');
    w.writeln('return this.$name;');
    w.exitBlock('};');
    var argsCode = args.getCode();
    w.enterBlock('Function.prototype.$name = function(${argsCode}) {');
    w.writeln('return this.to\$$name()($argsCode);');
    w.exitBlock('};');

    // TODO(jmesserly): HACK, we couldn't allocate temps from Value, so we
    // needed this stub to check for null.
    w.writeln('function to\$$name(f) { return f && f.to\$$name(); }');
  }

  void generateNamed(CodeWriter w) {
    // Named functions use simpler stubs, because we never convert to a named
    // stub type.
    var named = Strings.join(args.getNames(), '", "');
    var argsCode = args.getCode();
    w.enterBlock('Function.prototype.$name = function(${argsCode}) {');
    w.writeln('this.$name = this.\$genStub(${args.length}, ["$named"]);');
    w.writeln('return this.$name($argsCode);');
    w.exitBlock('}');
  }
}

class VarMethodStub extends VarMember {
  final Member member;
  final Arguments args;
  final Value body;

  VarMethodStub(String name, this.member, this.args, this.body): super(name);

  Type get returnType() =>
      member != null ? member.returnType : world.varType;

  String get typeName() =>
      member != null ? member.declaringType.jsname : 'Object';

  void generate(CodeWriter code) {
    code.write('$typeName.prototype.$name = ');
    generateBody(code, ';');
  }

  void generateBody(CodeWriter code, String end) {
    if (_useDirectCall(member, args)) {
      code.writeln('$typeName.prototype.${member.jsname}$end');
    } else {
      code.enterBlock('function(${args.getCode()}) {');
      code.writeln('return ${body.code};');
      code.exitBlock('}$end');
    }
  }

  bool _useDirectCall(Member member, Arguments args) {
    // TODO(jmesserly): for now disallow direct references to DOM types until we
    // figure out which types can be patched reliably.
    // I don't think our other native libs have this issue.
    if (member is MethodMember && !member.declaringType.isHiddenNativeType) {
      MethodMember method = member;
      if (method.needsArgumentConversion(args)) {
        return false;
      }

      // If we have the right number of parameters, or all defaults would be
      // filled in as "undefined" anyway, we can just call the method directly.
      for (int i = args.length; i < method.parameters.length; i++) {
        if (method.parameters[i].value.code != 'null') {
          return false;
        }
      }
      return method.namesInOrder(args);
    } else {
      return false;
    }
  }
}

/**
 * A special member with a mangled name that represents a dynamic call
 * (i.e. a call with multiple valid targets). We generate this if we have
 * a dynamic call that needs different implementation methods for different
 * members.
 */
class VarMethodSet extends VarMember {
  final List<Member> members;
  final Type returnType;
  final Arguments args;

  /** The fallback stubs that need to be in our Object.prototype stub. */
  List<VarMethodStub> _fallbackStubs;

  VarMethodSet(String name, this.members, Arguments callArgs, this.returnType)
    : super(name), args = callArgs.toCallStubArgs() {
  }

  /** The unmangled member name. */
  String get baseName() => members[0].name;

  Value invoke(MethodGenerator context, Node node, Value target, Arguments args) {
    _invokeMembers(context, node);
    return super.invoke(context, node, target, args);
  }

  /** Invokes members to ensure they're generated. */
  _invokeMembers(MethodGenerator context, Node node) {
    if (_fallbackStubs != null) return;

    var objectStub = null;
    _fallbackStubs = [];
    for (var member in members) {
      // Invoke the member with the stub args (this gives us the method body),
      // then create the stub method.
      final target = new Value(member.declaringType, 'this', node.span);
      var result = member.invoke(context, node, target, args, isDynamic:true);
      var stub = new VarMethodStub(name, member, args, result);

      // Put the stub on the type directly if possible. Otherwise
      // put the stub on Object.prototype.
      var type = member.declaringType;
      if (type.isObject) {
        objectStub = stub;
      } else if (!type.isHiddenNativeType) {
        _addVarStub(type, stub);
      } else {
        _fallbackStubs.add(stub);
      }
    }

    // Create a noSuchMethod fallback on Object if needed.
    // Some methods, like toString and == already have a fallback on Object.
    if (objectStub == null) {
      final target = new Value(world.objectType, 'this', node.span);
      var result = target.invokeNoSuchMethod(context, baseName, node, args);
      objectStub = new VarMethodStub(name, null, args, result);
    }
    if (_fallbackStubs.length == 0) {
      _addVarStub(world.objectType, objectStub);
    } else {
      _fallbackStubs.add(objectStub);
      world.gen.corejs.useVarMethod = true;
    }
  }

  static _addVarStub(Type type, VarMember stub) {
    if (type.varStubs == null) type.varStubs = {};
    type.varStubs[stub.name] = stub;
  }

  /**
   * Generate var call fallbacks, like this:
   *
   * $varMethod('addEventListener$1$capture', {
   *   'HTMLElement': function($0, capture) {
   *     return this.addEventListener($0, capture);
   *   },
   *   'SomeOtherDOMType': function($0, capture) {
   *     return this.addEventListener($0, false, true, capture);
   *   },
   *   'Object': function($0, capture) {
   *     return this.noSuchMethod('addEventListener', [$0],
   *       {'capture': capture});
   *   }
   * });
   */
  void generate(CodeWriter code) {
    if (_fallbackStubs.length == 0) return;

    code.enterBlock('\$varMethod("$name", {');
    var lastOne = _fallbackStubs.last();
    for (var stub in _fallbackStubs) {
      code.write('"${stub.typeName}": ');
      stub.generateBody(code, stub == lastOne ? '' : ',');
    }
    code.exitBlock('});');
  }
}

String _getCallStubName(String name, Arguments args) {
  final nameBuilder = new StringBuffer('${name}\$${args.bareCount}');
  for (int i = args.bareCount; i < args.length; i++) {
    nameBuilder.add('\$').add(args.getName(i));
  }
  return nameBuilder.toString();
}
