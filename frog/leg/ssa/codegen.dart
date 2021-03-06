// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

class SsaCodeGeneratorTask extends CompilerTask {
  SsaCodeGeneratorTask(Compiler compiler) : super(compiler);
  String get name() => 'SSA code generator';

  String generate(FunctionElement function, HGraph graph) {
    return measure(() {
      Map<Element, String> parameterNames =
          new LinkedHashMap<Element, String>();
      for (Link<Element> link = function.parameters;
           !link.isEmpty();
           link = link.tail) {
        Element element = link.head;
        parameterNames[element] = JsNames.getValid('${element.name}');
      }

      String code = generateMethod(parameterNames, graph);
      return code;
    });
  }

  void preGenerateMethod(HGraph graph) {
    if (GENERATE_SSA_TRACE) {
      new HTracer.singleton().traceGraph("codegen", graph);
    }
    new SsaPhiEliminator().visitGraph(graph);
    if (GENERATE_SSA_TRACE) {
      new HTracer.singleton().traceGraph("no-phi", graph);
    }
    // Replace the results of type guard instructions with the
    // original value, if the result is used. This is safe now,
    // since we don't do code motion after this point.
    new SsaTypeGuardUnuser().visitGraph(graph);
  }

  String generateMethod(Map<Element, String> parameterNames, HGraph graph) {
    preGenerateMethod(graph);
    StringBuffer buffer = new StringBuffer();
    SsaCodeGenerator codegen =
        new SsaCodeGenerator(compiler, buffer, parameterNames);
    codegen.visitGraph(graph);
    StringBuffer parameters = new StringBuffer();
    List<String> names = parameterNames.getValues();
    for (int i = 0; i < names.length; i++) {
      if (i != 0) parameters.add(', ');
      parameters.add(names[i]);
    }
    return 'function($parameters) {\n$buffer}';
  }
}

class SsaCodeGenerator implements HVisitor {
  final Compiler compiler;
  final StringBuffer buffer;

  final Map<Element, String> parameterNames;
  final Map<int, String> names;
  final Map<String, int> prefixes;

  int indent = 0;
  HGraph currentGraph;
  HBasicBlock currentBlock;

  SsaCodeGenerator(this.compiler, this.buffer, this.parameterNames)
    : names = new Map<int, String>(),
      prefixes = new Map<String, int>() {
    for (final name in parameterNames.getValues()) {
      prefixes[name] = 0;
    }
  }

  visitGraph(HGraph graph) {
    currentGraph = graph;
    indent++;  // We are already inside a function.
    visitBasicBlock(graph.entry);
  }

  String parameter(HParameterValue parameter) => parameterNames[parameter.element];

  String temporary(HInstruction instruction) {
    int id = instruction.id;
    String name = names[id];
    if (name !== null) return name;

    String prefix = 't';
    if (!prefixes.containsKey(prefix)) prefixes[prefix] = 0;
    return newName(id, '${prefix}${prefixes[prefix]++}');
  }

  String local(HLocal local) {
    Element element = local.element;
    if (element != null && element.kind == ElementKind.PARAMETER) {
      return parameterNames[element];
    }
    int id = local.id;
    String name = names[id];
    if (name !== null) return name;

    String prefix;
    if (element !== null) {
      prefix = element.name.stringValue;
    } else {
      prefix = 'v';
    }
    if (!prefixes.containsKey(prefix)) {
      prefixes[prefix] = 0;
      return newName(id, prefix);
    } else {
      return newName(id, '${prefix}_${prefixes[prefix]++}');
    }
  }

  String newName(int id, String name) {
    String result = JsNames.getValid(name);
    names[id] = result;
    return result;
  }

  void invoke(Element element, List<HInstruction> inputs) {
    assert(inputs.length >= 1);
    use(inputs[0]);
    buffer.add('(');
    for (int i = 1; i < inputs.length; i++) {
      if (i != 1) buffer.add(', ');
      use(inputs[i]);
    }
    buffer.add(")");
  }

  void define(HInstruction instruction) {
    buffer.add('var ${temporary(instruction)} = ');
    visit(instruction);
  }

  void use(HInstruction argument) {
    if (argument.generateAtUseSite()) {
      visit(argument);
    } else {
      buffer.add(temporary(argument));
    }
  }

  visit(node) {
    return node.accept(this);
  }

  visitBasicBlock(HBasicBlock node) {
    // While loop will be closed by the conditional loop-branch.
    // TODO(floitsch): HACK HACK HACK.
    if (node.isLoopHeader()) {
      addIndentation();
      buffer.add('while (true) {\n');
      indent++;
    }
    currentBlock = node;

    HInstruction instruction = node.first;
    while (instruction != null) {
      if (instruction is HGoto || instruction is HExit) {
        visit(instruction);
      } else if (!instruction.generateAtUseSite()) {
        addIndentation();
        if (instruction.usedBy.isEmpty() || instruction is HLocal) {
          visit(instruction);
        } else {
          define(instruction);
        }
        // Control flow instructions know how to handle ';'.
        if (instruction is !HControlFlow) {
          buffer.add(';\n');
        }
      }
      instruction = instruction.next;
    }
  }

  visitInvokeBinary(HInvokeBinary node, String op) {
    if (node.builtin) {
      buffer.add('(');
      use(node.left);
      buffer.add(' $op ');
      use(node.right);
      buffer.add(')');
    } else {
      visitInvokeStatic(node);
    }
  }

  visitInvokeUnary(HInvokeUnary node, String op) {
    if (node.builtin) {
      buffer.add('($op');
      use(node.operand);
      buffer.add(')');
    } else {
      visitInvokeStatic(node);
    }
  }

  visitAdd(HAdd node)               => visitInvokeBinary(node, '+');
  visitDivide(HDivide node)         => visitInvokeBinary(node, '/');
  visitMultiply(HMultiply node)     => visitInvokeBinary(node, '*');
  visitSubtract(HSubtract node)     => visitInvokeBinary(node, '-');
  // Truncating divide does not have a JS equivalent.
  visitTruncatingDivide(HTruncatingDivide node) => visitInvokeStatic(node);
  // Modulo cannot be mapped to the native operator (different semantics).
  visitModulo(HModulo node)                     => visitInvokeStatic(node);
  // Bit-operations require its argument to be of type integer.
  // TODO(floitsch): use shift operators when we can detect that the inputs
  // are integers.
  visitBitAnd(HBitAnd node)         => visitInvokeStatic(node);
  visitBitNot(HBitNot node)         => visitInvokeStatic(node);
  visitBitOr(HBitOr node)           => visitInvokeStatic(node);
  visitBitXor(HBitXor node)         => visitInvokeStatic(node);
  visitShiftLeft(HShiftLeft node)   => visitInvokeStatic(node);
  visitShiftRight(HShiftRight node) => visitInvokeStatic(node);

  visitNegate(HNegate node)         => visitInvokeUnary(node, '-');

  visitEquals(HEquals node)             => visitInvokeBinary(node, '===');
  visitLess(HLess node)                 => visitInvokeBinary(node, '<');
  visitLessEqual(HLessEqual node)       => visitInvokeBinary(node, '<=');
  visitGreater(HGreater node)           => visitInvokeBinary(node, '>');
  visitGreaterEqual(HGreaterEqual node) => visitInvokeBinary(node, '>=');

  visitBoolify(HBoolify node) {
    assert(node.inputs.length == 1);
    buffer.add('(');
    use(node.inputs[0]);
    buffer.add(' === true)');
  }

  visitExit(HExit node) {
    // Don't do anything.
  }

  visitGoto(HGoto node) {
    assert(currentBlock.successors.length == 1);
    List<HBasicBlock> dominated = currentBlock.dominatedBlocks;
    // With the exception of the entry-node which is dominates its successor
    // and the exit node, no block finishing with a 'goto' can have more than
    // one dominated block (since it has only one successor).
    // If the successor is dominated by another block, then the other block
    // is responsible for visiting the successor.
    if (dominated.isEmpty()) return;
    if (dominated.length > 2) unreachable();
    if (dominated.length == 2 && currentBlock !== currentGraph.entry) {
      unreachable();
    }
    assert(dominated[0] == currentBlock.successors[0]);
    visitBasicBlock(dominated[0]);
  }

  visitIf(HIf node) {
    // The currentBlock will be changed when we visit the successors. So keep
    // a local copy around.
    HBasicBlock ifBlock = currentBlock;
    buffer.add('if (');
    use(node.inputs[0]);
    buffer.add(') {\n');
    indent++;
    List<HBasicBlock> dominated = currentBlock.dominatedBlocks;
    assert(dominated[0] === ifBlock.successors[0]);
    visitBasicBlock(ifBlock.successors[0]);
    indent--;
    addIndentation();
    int nextDominatedIndex;
    if (node.hasElse) {
      assert(dominated[1] === ifBlock.successors[1]);
      buffer.add('} else {\n');
      indent++;
      visitBasicBlock(ifBlock.successors[1]);
      indent--;
      addIndentation();
      buffer.add("}\n");
    } else {
      buffer.add("}\n");
    }

    // Normally the HIf dominates the join-block. In this case there is one
    // dominated block that we need to visit:
    // If both the then and else blocks return/throw, then the join-block is
    // either the exit-block, or there is none.
    // We can also have the case where the HIf has no else, but the then-branch
    // terminates. If the code after the 'if' terminates, then the
    // if could become the dominator of the exit-block, thus having
    // three dominated blocks: the then, the code after the if, and the exit
    // block.

    if (node.hasElse && dominated.length == 3) {
      // Normal case. If both branches terminate then the third dominated
      // block is the exit-block.
      visitBasicBlock(dominated[2]);
    } else if (node.hasElse) {
      // Both branches terminate, but this HIf is not the dominator of the exit
      // block.
      assert(dominated.length == 2);
      return;
    } else if (!node.hasElse && dominated.length == 2) {
      // Normal case. Even if the then-branch terminated there is still
      // a join-block.
      assert(!dominated[1].isExitBlock());
      visitBasicBlock(dominated[1]);
    } else {
      // The then-branch terminates, and the code following the if terminates
      // too. The if happens to dominate the exit-block.
      assert(!node.hasElse);
      assert(dominated.length == 3);
      assert(dominated[2].isExitBlock());
      visitBasicBlock(dominated[1]);
      visitBasicBlock(dominated[2]);
    }
  }

  visitInvokeStatic(HInvokeStatic node) {
    compiler.addToWorklist(node.element);
    invoke(node.element, node.inputs);
  }

  visitForeign(HForeign node) {
    String code = '${node.code}';
    List<HInstruction> inputs = node.inputs;
    for (int i = 0; i < inputs.length; i++) {
      HInstruction input = inputs[i];
      String name;
      if (input is HParameterValue) {
        name = parameter(input);
      } else {
        assert(!input.generateAtUseSite());
        name = temporary(input);
      }
      code = code.replaceAll('\$$i', name);
    }
    buffer.add('($code)');
  }

  visitLiteral(HLiteral node) {
    if (node.value === null) {
      buffer.add("(void 0)");
    } else if (node.value is num && node.value < 0) {
      buffer.add('(${node.value})');
    } else {
      buffer.add(node.value);
    }
  }

  visitLoopBranch(HLoopBranch node) {
    HBasicBlock branchBlock = currentBlock;
    buffer.add('if (!(');
    use(node.inputs[0]);
    buffer.add(')) break;\n');
    List<HBasicBlock> dominated = currentBlock.dominatedBlocks;
    HBasicBlock loopSuccessor;
    if (dominated.length == 1) {
      // Do While.
      // The first successor is the loop-body and thus a back-edge.
      assert(branchBlock.successors[0].id < branchBlock.id);
      assert(dominated[0] === branchBlock.successors[1]);
      // The body has already been visited. Nothing to do in this branch.
    } else {
      // A normal while loop. Visit the body.
      assert(dominated.length == 2);
      assert(dominated[0] === branchBlock.successors[0]);
      assert(dominated[1] === branchBlock.successors[1]);
      visit(dominated[0]);
    }
    indent--;
    addIndentation();
    buffer.add('}\n');  // Close 'while' loop.
    visit(branchBlock.successors[1]);
  }

  visitNot(HNot node) {
    assert(node.inputs.length == 1);
    buffer.add('(!');
    use(node.inputs[0]);
    buffer.add(')');
  }

  visitParameterValue(HParameterValue node) {
    buffer.add(parameter(node));
  }

  visitPhi(HPhi node) {
    // The SsaPhiEliminator made sure phis are gone in the function.
    unreachable();
  }

  visitReturn(HReturn node) {
    assert(node.inputs.length == 1);
    HInstruction input = node.inputs[0];
    if (input.isLiteralNull()) {
      buffer.add('return;\n');
    } else {
      buffer.add('return ');
      use(node.inputs[0]);
      buffer.add(';\n');
    }
  }

  visitThrow(HThrow node) {
    buffer.add('throw ');
    use(node.inputs[0]);
    buffer.add(';\n');
  }

  visitTypeGuard(HTypeGuard node) {
    SourceString name;
    if (node.isNumber()) {
      name = const SourceString('guard\$num');
    } else if (node.isString()) {
      name = const SourceString('guard\$string');
    } else {
      unreachable();
    }
    Element element = compiler.universe.find(name);
    assert(element !== null);
    compiler.addToWorklist(element);
    buffer.add(compiler.namer.isolateAccess(element));
    buffer.add('(');
    use(node.inputs[0]);
    buffer.add(')');
  }

  void addIndentation() {
    for (int i = 0; i < indent; i++) {
      buffer.add('  ');
    }
  }

  void visitStatic(HStatic node) {
    buffer.add(compiler.namer.isolateAccess(node.element));
  }

  void visitStore(HStore node) {
    if (node.local.declaredBy === node) {
      buffer.add('var ');
    }
    buffer.add('${local(node.local)} = ');
    use(node.value);
  }

  void visitLoad(HLoad node) {
    buffer.add('${local(node.local)}');
  }

  void visitLocal(HLocal node) {
    buffer.add('var ${local(node)}');
  }

  void visitLiteralList(HLiteralList node) {
    buffer.add('[');
    int len = node.inputs.length;
    for (int i = 0; i < len; i++) {
      if (i != 0) buffer.add(', ');
      use(node.inputs[i]);
    }
    buffer.add(']');
  }

  void visitIndex(HIndex node) => visitInvokeStatic(node);
  void visitIndexAssign(HIndexAssign node) => visitInvokeStatic(node);
}
