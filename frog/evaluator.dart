// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.


#import('file_system.dart');
#import('lang.dart');

interface JsEvaluator {
  var eval(String source);
}

class Evaluator {
  JsEvaluator _jsEvaluator;
  static Set<String> _marked;
  Library _lib;

  static void initWorld(String homedir, List<String> args, FileSystem files) {
    parseOptions(homedir, args);
    initializeWorld(files);
    world.process();
    world.resolveAll();

    _marked = new Set();

    world.gen = new WorldGenerator(null, new CodeWriter());
    _markAllUsed(world.corelib);
    world.gen.writeTypes(world.coreimpl);
    world.gen.writeTypes(world.corelib);

    // Set these here so that we can compile the corelib without its errors
    // killing us
    options.throwOnErrors = true;
    options.throwOnFatal = true;
  }

  static void _markMethodUsed(Member m) {
    if (m == null || m.isGenerated || m.definition == null || m.isAbstract) {
      return;
    }
    new MethodGenerator(m, null).run();
  }

  // TODO(nweiz): use this logic for the --compile_all flag
  static void _markAllUsed(Library l) {
    if (_marked.contains(l.name)) return;
    _marked.add(l.name);

    l.imports.forEach((i) => _markAllUsed(i.library));
    for (var type in l.types.getValues()) {
      if (!type.isClass) return;

      type.markUsed();
      for (var member in type.members.getValues()) {
        if (member is FieldMember) {
          _markMethodUsed(member.getter);
          _markMethodUsed(member.setter);
        }

        if (member.isMethod) _markMethodUsed(member);
      }
    }
  }

  Evaluator(JsEvaluator this._jsEvaluator) {
    if (_marked == null) {
      throw new UnsupportedOperationException(
          "Must call Evaluator.initWorld before creating a Evaluator.");
    }
    this._jsEvaluator.eval(world.gen.writer.text);
    _lib = new Library(new SourceFile("#ifrog", ""));
    _lib.imports.add(new LibraryImport(world.corelib));
    _lib.resolve();
  }

  var eval(String dart) {
    var source = new SourceFile("#ifrog", dart);
    // TODO(jimhug): This is usually frowned on - one gen per world...
    var gen = new WorldGenerator(null, new CodeWriter());

    var code;
    var parsed = new Parser(source).evalUnit();
    var method = new MethodMember("_ifrog_dummy", _lib.topType, null);
    var methGen = new MethodGenerator(method, null);

    if (parsed is ExpressionStatement) {
      code = parsed.body.visit(methGen).code;
    } else if (parsed is VariableDefinition) {
      // TODO(nweiz): Make this more user-friendly (don't require explicit
      // variable declarations, allow overwriting variables/functions).
      var emptyDef = new VariableDefinition(parsed.modifiers, parsed.type,
          parsed.names, new List(parsed.names.length), parsed.span);
      _lib.topType.addField(emptyDef);
      parsed.visit(methGen);
      code = methGen.writer.text;
    } else if (parsed is FunctionDefinition) {
      _lib.topType.addMethod(parsed.name.name, parsed);
      MethodMember definedMethod = _lib.topType.getMember(parsed.name.name);
      definedMethod.resolve(_lib.topType);
      var definedMethGen = new MethodGenerator(definedMethod, null);
      definedMethGen.run();
      definedMethGen.writeDefinition(gen.writer, null);
      code = gen.writer.text;
    } else if (parsed is TypeDefinition) {
      var type = _lib.addType(parsed.name.name, parsed, parsed.isClass);
      type.resolve();
      gen.writeType(type);
      code = gen.writer.text;
    } else {
      parsed.visit(methGen);
      code = methGen.writer.text;
    }

    return this._jsEvaluator.eval(code);
  }
}