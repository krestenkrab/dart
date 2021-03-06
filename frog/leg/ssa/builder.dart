// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

class SsaBuilderTask extends CompilerTask {
  SsaBuilderTask(Compiler compiler) : super(compiler);
  String get name() => 'SSA builder';

  HGraph build(FunctionElement element, TreeElements elements) {
    return measure(() {
      FunctionExpression function = element.node;
      HInstruction.idCounter = 0;
      HGraph graph =
          compileMethod(function.parameters, function.body, elements);
      assert(graph.isValid());
      if (GENERATE_SSA_TRACE) {
        Identifier name = function.name;
        new HTracer.singleton().traceCompilation(name.source.toString());
        new HTracer.singleton().traceGraph('builder', graph);
      }
      return graph;
    });
  }

  HGraph compileMethod(NodeList parameters,
                       Node body,
                       TreeElements elements) {
    SsaBuilder builder = new SsaBuilder(compiler, elements);
    HGraph graph = builder.build(parameters, body);
    return graph;
  }
}

class SsaBuilder implements Visitor {
  final Compiler compiler;
  final TreeElements elements;
  HGraph graph;

  // We build the Ssa graph by simulating a stack machine.
  List<HInstruction> stack;

  Map<Element, HInstruction> definitions;

  // The current block to add instructions to. Might be null, if we are
  // visiting dead code.
  HBasicBlock current;

  SsaBuilder(this.compiler, this.elements);

  HGraph build(NodeList parameters, Node body) {
    stack = new List<HInstruction>();
    definitions = new Map<Element, HInstruction>();

    graph = new HGraph();
    HBasicBlock block = graph.addNewBlock();

    open(graph.entry);
    visitParameterValues(parameters);
    close(new HGoto()).addSuccessor(block);

    open(block);
    body.accept(this);

    // TODO(kasperl): Make this goto an implicit return.
    if (!isAborted()) close(new HGoto()).addSuccessor(graph.exit);
    graph.finalize();
    return graph;
  }

  void open(HBasicBlock block) {
    block.open();
    current = block;
  }

  HBasicBlock close(HControlFlow end) {
    HBasicBlock result = current;
    current.close(end);
    current = null;
    return result;
  }

  void goto(HBasicBlock from, HBasicBlock to) {
    from.close(new HGoto());
    from.addSuccessor(to);
  }

  bool isAborted() {
    return current === null;
  }

  void add(HInstruction instruction) {
    current.add(instruction);
  }

  void push(HInstruction instruction) {
    add(instruction);
    stack.add(instruction);
  }

  HInstruction pop() {
    return stack.removeLast();
  }

  HBoolify popBoolified() {
    HBoolify boolified = new HBoolify(pop());
    add(boolified);
    return boolified;
  }

  HInstruction guard(Type type, HInstruction value) {
    if (type !== null) {
      if (type.toString() == 'int') {
        value = new HTypeGuard(HInstruction.TYPE_NUMBER, value);
        add(value);
      } else if (type.toString() == 'String') {
        value = new HTypeGuard(HInstruction.TYPE_STRING, value);
        add(value);
      }
    }
    return value;
  }

  void visit(Node node) {
    if (node !== null) node.accept(this);
  }

  visitParameterValues(NodeList parameters) {
    int parameterIndex = 0;
    for (Link<Node> link = parameters.nodes;
         !link.isEmpty();
         link = link.tail) {
      VariableDefinitions container = link.head;
      Link<Node> identifierLink = container.definitions.nodes;
      // The identifier link must contain exactly one argument.
      assert(!identifierLink.isEmpty() && identifierLink.tail.isEmpty());
      if (identifierLink.head is !Identifier) {
        compiler.unimplemented("SsaBuilder.visitParameterValues non-identifier");
      }
      Identifier parameterId = identifierLink.head;
      VariableElement element = elements[parameterId];
      HParameterValue parameter = new HParameterValue(element);
      add(parameter);
      definitions[element] = guard(element.type, parameter);
    }
  }

  visitBlock(Block node) {
    for (Link<Node> link = node.statements.nodes;
         !link.isEmpty();
         link = link.tail) {
      visit(link.head);
      if (isAborted()) {
        // The block has been aborted by a return or a throw.
        if (!stack.isEmpty()) compiler.cancel('non-empty instruction stack');
        return;
      }
    }
    assert(!current.isClosed());
    if (!stack.isEmpty()) compiler.cancel('non-empty instruction stack');
  }

  visitClassNode(ClassNode node) {
    unreachable();
  }

  visitExpressionStatement(ExpressionStatement node) {
    visit(node.expression);
    pop();
  }

  /**
   * Creates a new loop-header block and fills it with phis of the current
   * definitions. The previous [current] block is closed with an [HGoto] and
   * replace with the newly created block.
   * Returns a copy of the definitions at the moment of entering the loop.
   */
  Map<Element, HInstruction> startLoop() {
    assert(!isAborted());
    HBasicBlock previousBlock = close(new HGoto());

    Map definitionsCopy = new Map<Element, HInstruction>.from(definitions);
    HBasicBlock loopBlock = graph.addNewLoopHeaderBlock();
    previousBlock.addSuccessor(loopBlock);
    open(loopBlock);

    // Create phis for all elements in the definitions environment.
    definitionsCopy.forEach((Element element, HInstruction instruction) {
      HPhi phi = new HPhi.singleInput(element, instruction);
      loopBlock.addPhi(phi);
      definitions[element] = phi;
    });

    return definitionsCopy;
  }

  /**
   * Ends the loop:
   * - Updates the phis in the [loopEntry].
   * - if [doUpdateDefinitions] is true, fills the [exitDefinitions] with the
   *   updated values.
   * - sets [exitDefinitions] as the new [definitions].
   * - creates a new block and adds it as successor to the [branchBlock].
   * - opens the new block (setting as [current]). 
   */
  void endLoop(HBasicBlock loopEntry, HBasicBlock branchBlock,
               bool doUpdateDefinitions,
               Map<Element, HInstruction> exitDefinitions) {
    loopEntry.forEachPhi((HPhi phi) {
      Element element = phi.element;
      HInstruction postLoopDefinition = definitions[element];
      phi.addInput(postLoopDefinition);
      if (doUpdateDefinitions &&
          phi.inputs[0] !== postLoopDefinition &&
          exitDefinitions.containsKey(element)) {
        exitDefinitions[element] = postLoopDefinition;
      }
    });

    HBasicBlock loopExitBlock = graph.addNewBlock();
    assert(branchBlock.successors.length == 1);
    branchBlock.addSuccessor(loopExitBlock);
    open(loopExitBlock);
    definitions = exitDefinitions;
  }

  // For while loops, initializer and update are null.
  visitLoop(Statement initializer, Expression condition, Expression update,
            Node body) {
    assert(condition !== null && body !== null);
    // The initializer.
    if (initializer !== null) visit(initializer);
    assert(!isAborted());

    Map initializerDefinitions = startLoop();
    HBasicBlock conditionBlock = current;

    // The condition.
    visit(condition);
    HBasicBlock conditionExitBlock = close(new HLoopBranch(popBoolified()));

    Map conditionDefinitions = new Map<Element, HInstruction>.from(definitions);

    // The body.
    HBasicBlock bodyBlock = graph.addNewBlock();
    conditionExitBlock.addSuccessor(bodyBlock);
    open(bodyBlock);
    visit(body);
    if (isAborted()) {
      compiler.unimplemented("SsaBuilder for loop with aborting body");
    }
    bodyBlock = close(new HGoto());

    // Update.
    // We create an update block, even when we are in a while loop. There the
    // update block is the jump-target for continue statements. We could avoid
    // the creation if there is no continue, but for now we always create it.
    HBasicBlock updateBlock = graph.addNewBlock();
    bodyBlock.addSuccessor(updateBlock);
    open(updateBlock);
    if (update !== null) {
      visit(update);
      assert(!isAborted());
      // The update instruction can just be popped. It is not used.
      HInstruction updateInstruction = pop();
    }
    updateBlock = close(new HGoto());
    // The back-edge completing the cycle.
    updateBlock.addSuccessor(conditionBlock);
    conditionBlock.postProcessLoopHeader();    

    endLoop(conditionBlock, conditionExitBlock, false, conditionDefinitions);
  }

  visitFor(For node) {
    assert(node.initializer !== null && node.condition !== null &&
           node.update !== null && node.body !== null);
    visitLoop(node.initializer, node.condition, node.update, node.body);
  }

  visitWhile(While node) {
    visitLoop(null, node.condition, null, node.body);
  }

  visitDoWhile(DoWhile node) {
    Map entryDefinitions = startLoop();
    HBasicBlock loopEntryBlock = current;

    visit(node.body);
    if (isAborted()) {
      compiler.unimplemented("SsaBuilder for loop with aborting body");
    }

    // If there are no continues we could avoid the creation of the condition
    // block. This could also lead to a block having multiple entries and exits.
    HBasicBlock bodyExitBlock = close(new HGoto());
    HBasicBlock conditionBlock = graph.addNewBlock();
    bodyExitBlock.addSuccessor(conditionBlock);
    open(conditionBlock);
    visit(node.condition);
    assert(!isAborted());
    conditionBlock = close(new HLoopBranch(popBoolified()));

    conditionBlock.addSuccessor(loopEntryBlock);  // The back-edge.
    loopEntryBlock.postProcessLoopHeader();

    endLoop(loopEntryBlock, conditionBlock, true, entryDefinitions);
  }

  visitFunctionExpression(FunctionExpression node) {
    compiler.unimplemented('SsaBuilder.visitFunctionExpression');
  }

  visitIdentifier(Identifier node) {
    Element element = elements[node];
    compiler.ensure(element !== null);
    HInstruction def = definitions[element];
    assert(def !== null);
    stack.add(def);
  }

  Map<Element, HInstruction> joinDefinitions(
      HBasicBlock joinBlock,
      Map<Element, HInstruction> incoming1,
      Map<Element, HInstruction> incoming2) {
    // If an element is in one map but not the other we can safely
    // ignore it. It means that a variable was declared in the
    // block. Since variable declarations are scoped the declared
    // variable cannot be alive outside the block. Note: this is only
    // true for nodes where we do joins.
    Map<Element, HInstruction> joinedDefinitions =
        new Map<Element, HInstruction>();
    incoming1.forEach((element, instruction) {
      HInstruction other = incoming2[element];
      if (other === null) return;
      if (instruction === other) {
        joinedDefinitions[element] = instruction;
      } else {
        HInstruction phi = new HPhi.manyInputs(element, [instruction, other]);
        joinBlock.addPhi(phi);
        joinedDefinitions[element] = phi;
      }
    });
    return joinedDefinitions;
  }

  visitIf(If node) {
    // Add the condition to the current block.
    bool hasElse = node.hasElsePart;
    visit(node.condition);
    HBasicBlock conditionBlock = close(new HIf(popBoolified(), hasElse));

    Map conditionDefinitions =
        new Map<Element, HInstruction>.from(definitions);

    // The then part.
    HBasicBlock thenBlock = graph.addNewBlock();
    conditionBlock.addSuccessor(thenBlock);
    open(thenBlock);
    visit(node.thenPart);
    thenBlock = current;
    Map thenDefinitions = definitions;

    // Reset the definitions to the state after the condition.
    definitions = conditionDefinitions;

    // Now the else part.
    HBasicBlock elseBlock = null;
    if (hasElse) {
      elseBlock = graph.addNewBlock();
      conditionBlock.addSuccessor(elseBlock);
      open(elseBlock);
      visit(node.elsePart);
      elseBlock = current;
    }

    if (thenBlock === null && elseBlock === null && hasElse) {
      current = null;
    } else {
      HBasicBlock joinBlock = graph.addNewBlock();
      if (thenBlock !== null) goto(thenBlock, joinBlock);
      if (elseBlock !== null) goto(elseBlock, joinBlock);
      else if (!hasElse) conditionBlock.addSuccessor(joinBlock);
      // If the join block has two predecessors we have to merge the
      // definition maps. The current definitions is what either the
      // condition or the else block left us with, so we merge that
      // with the set of definitions we got after visiting the then
      // part of the if.
      open(joinBlock);
      if (joinBlock.predecessors.length == 2) {
        definitions = joinDefinitions(joinBlock,
                                      thenDefinitions,
                                      definitions);
      }
    }
  }

  SourceString unquote(LiteralString literal) {
    String str = '${literal.value}';
    compiler.ensure(str[0] == '@');
    int quotes = 1;
    String quote = str[1];
    while (str[quotes + 1] === quote) quotes++;
    return new SourceString(str.substring(quotes + 1, str.length - quotes));
  }

  void visitLogicalAndOr(Send node, Operator op) {
    // x && y is transformed into:
    //   t0 = boolify(x);
    //   if (t0) t1 = boolify(y);
    //   result = phi(t0, t1);
    //
    // x || y is transformed into:
    //   t0 = boolify(x);
    //   if (not(t0)) t1 = boolify(y);
    //   result = phi(t0, t1);
    bool isAnd = (const SourceString("&&") == op.source);

    visit(node.receiver);
    HInstruction boolifiedLeft = popBoolified();
    HInstruction condition;
    if (isAnd) {
      condition = boolifiedLeft;
    } else {
      condition = new HNot(boolifiedLeft);
      add(condition);
    }
    HBasicBlock leftBlock = close(new HIf(condition, false));
    Map leftDefinitions = new Map<Element, HInstruction>.from(definitions);

    HBasicBlock rightBlock = graph.addNewBlock();
    leftBlock.addSuccessor(rightBlock);
    open(rightBlock);
    visit(node.argumentsNode);
    HInstruction boolifiedRight = popBoolified();
    rightBlock = close(new HGoto());

    HBasicBlock joinBlock = graph.addNewBlock();
    leftBlock.addSuccessor(joinBlock);
    rightBlock.addSuccessor(joinBlock);
    open(joinBlock);

    definitions = joinDefinitions(joinBlock, leftDefinitions, definitions);
    HPhi result = new HPhi.manyInputs(null, [boolifiedLeft, boolifiedRight]);
    joinBlock.addPhi(result);
    stack.add(result);
  }

  void visitLogicalNot(Send node) {
    assert(node.argumentsNode is Prefix);
    visit(node.receiver);
    HNot not = new HNot(popBoolified());
    push(not);
  }

  void visitUnary(Send node, Operator op, Element element) {
    assert(node.argumentsNode is Prefix);
    visit(node.receiver);
    HInstruction operand = pop();
    HInstruction target = new HStatic(element);
    add(target);
    switch (op.source.stringValue) {
      case "-": push(new HNegate(target, operand)); break;
      case "~": push(new HBitNot(target, operand)); break;
      default: unreachable();
    }
  }

  void visitBinary(HInstruction left, Operator op, HInstruction right,
                   Element element) {
    HInstruction target = new HStatic(element);
    add(target);
    switch (op.source.stringValue) {
      case "+":
      case "++":
      case "+=":  push(new HAdd(target, left, right)); break;
      case "-":
      case "--":
      case "-=":  push(new HSubtract(target, left, right)); break;
      case "*":
      case "*=":  push(new HMultiply(target, left, right)); break;
      case "/":
      case "/=":  push(new HDivide(target, left, right)); break;
      case "~/":
      case "~/=": push(new HTruncatingDivide(target, left, right)); break;
      case "%":
      case "%=":  push(new HModulo(target, left, right)); break;
      case "<<":
      case "<<=": push(new HShiftLeft(target, left, right)); break;
      case ">>":
      case ">>=": push(new HShiftRight(target, left, right)); break;
      case "|":
      case "|=":  push(new HBitOr(target, left, right)); break;
      case "&":
      case "&=":  push(new HBitAnd(target, left, right)); break;
      case "^":
      case "^=":  push(new HBitXor(target, left, right)); break;
      case "==":  push(new HEquals(target, left, right)); break;
      case "<":   push(new HLess(target, left, right)); break;
      case "<=":  push(new HLessEqual(target, left, right)); break;
      case ">":   push(new HGreater(target, left, right)); break;
      case ">=":  push(new HGreaterEqual(target, left, right)); break;
      default: compiler.unimplemented("SsaBuilder.visitBinary");
    }
  }

  visitSend(Send node) {
    Element element = elements[node];
    // TODO(kasperl): This only works for very special cases. Make
    // this way more general soon.
    if (node.selector is Operator) {
      Operator op = node.selector;
      if (const SourceString("[]") == op.source) {
        HStatic target = new HStatic(element);
        add(target);
        visit(node.receiver);
        HInstruction receiver = pop();
        visit(node.argumentsNode);
        HInstruction index = pop();
        push(new HIndex(target, receiver, index));
      } else if (const SourceString("&&") == op.source ||
                 const SourceString("||") == op.source) {
        visitLogicalAndOr(node, op);
      } else if (const SourceString("!") == op.source) {
        visitLogicalNot(node);
      } else if (node.argumentsNode is Prefix ||
                 node.argumentsNode is Postfix) {
        visitUnary(node, op, element);
      } else {
        visit(node.receiver);
        visit(node.argumentsNode);
        var right = pop();
        var left = pop();
        visitBinary(left, op, right, element);
      }
    } else if (node.isPropertyAccess) {
      if (node.receiver !== null) {
        compiler.unimplemented("SsaBuilder.visitSend with receiver");
      }
      HInstruction instruction = definitions[element];
      assert(instruction !== null);
      stack.add(instruction);
    } else {
      if (element === null) {
        compiler.unimplemented("SsaBuilder.visitSend with receiver");
      }
      Link<Node> link = node.arguments;
      if (element.kind === ElementKind.FOREIGN) {
        // If the invoke is on foreign code, don't visit the first
        // argument, which is the foreign code.
        link = link.tail;
      }
      var arguments = [];
      if (node.receiver !== null) {
        visit(node.receiver);
        arguments.add(pop());
      }
      for (; !link.isEmpty(); link = link.tail) {
        visit(link.head);
        arguments.add(pop());
      }

      if (element.kind === ElementKind.FOREIGN) {
        LiteralString literal = node.arguments.head;
        compiler.ensure(literal is LiteralString);
        push(new HForeign(arguments, unquote(literal)));
      } else {
        HStatic target = new HStatic(element);
        add(target);
        List inputs = <HInstruction>[target];
        inputs.addAll(arguments);
        push(new HInvokeStatic(inputs));
      }
    }
  }

  visitNewExpression(NewExpression node) {
    compiler.unimplemented("SsaBuilder: new expression");
  }

  HInstruction updateDefinition(Node node, HInstruction value) {
    VariableElement element = elements[node];
    value = guard(element.type, value);
    definitions[element] = value;
    return value;
  }

  visitSendSet(SendSet node) {
    Operator op = node.assignmentOperator;
    if (node.isIndex) {
      HStatic target = new HStatic(elements[node]);
      add(target);
      visit(node.receiver);
      HInstruction receiver = pop();
      visit(node.argumentsNode);
      HInstruction value = pop();
      HInstruction index = pop();
      if (const SourceString("=") == op.source) {
        push(new HIndexAssign(target, receiver, index, value));
      } else if (const SourceString("++") == op.source ||
                 const SourceString("--") == op.source) {
        compiler.unimplemented("SsaBuilder: increment on array access");
      } else {
        HStatic indexMethod = new HStatic(elements[node.selector]);
        add(indexMethod);
        HInstruction left = new HIndex(indexMethod, receiver, index);
        add(left);
        Element opElement = elements[op];
        visitBinary(left, op, value, opElement);
        push(new HIndexAssign(target, receiver, index, pop()));
      }
    } else if (node.receiver != null) {
      compiler.unimplemented("SsaBuilder: property access");
    } else if (const SourceString("=") == op.source) {
      Link<Node> link = node.arguments;
      assert(!link.isEmpty() && link.tail.isEmpty());
      visit(link.head);
      stack.add(updateDefinition(node, pop()));
    } else {
      assert(const SourceString("++") == op.source ||
             const SourceString("--") == op.source ||
             node.assignmentOperator.source.stringValue.endsWith("="));
      bool isCompoundAssignment = !node.arguments.isEmpty();
      bool isPrefix = !node.isPostfix;  // Compound assignments are prefix.
      Element getter = elements[node.selector];
      HInstruction left = definitions[getter];
      HInstruction right;
      if (isCompoundAssignment) {
        visit(node.argumentsNode);
        right = pop();
      } else {
        right = new HLiteral(1);
        add(right);
      }
      Element opElement = elements[op];
      visitBinary(left, op, right, opElement);
      HInstruction operation = pop();
      assert(operation !== null);
      // updateDefinition might guard the operation thus returning a new node.
      operation = updateDefinition(node, operation);
      if (isPrefix) {
        stack.add(operation);
      } else {
        stack.add(left);
      }
    }
  }

  void visitLiteralInt(LiteralInt node) {
    push(new HLiteral(node.value));
  }

  void visitLiteralDouble(LiteralDouble node) {
    push(new HLiteral(node.value));
  }

  void visitLiteralBool(LiteralBool node) {
    push(new HLiteral(node.value));
  }

  void visitLiteralString(LiteralString node) {
    push(new HLiteral(node.value));
  }

  void visitLiteralNull(LiteralNull node) {
    push(new HLiteral(null));
  }

  visitNodeList(NodeList node) {
    for (Link<Node> link = node.nodes; !link.isEmpty(); link = link.tail) {
      visit(link.head);
    }
  }

  void visitParenthesizedExpression(ParenthesizedExpression node) {
    visit(node.expression);
  }

  visitOperator(Operator node) {
    // Operators are intercepted in their surrounding Send nodes.
    unreachable();
  }

  visitReturn(Return node) {
    HInstruction value;
    if (node.expression === null) {
      value = new HLiteral(null);
      add(value);
    } else {
      visit(node.expression);
      value = pop();
    }
    close(new HReturn(value)).addSuccessor(graph.exit);
  }

  visitThrow(Throw node) {
    if (node.expression === null) {
      compiler.unimplemented("SsaBuilder: throw without expression");
    }
    visit(node.expression);
    close(new HThrow(pop()));
  }

  visitTypeAnnotation(TypeAnnotation node) {
    // We currently ignore type annotations for generating code.
  }

  visitVariableDefinitions(VariableDefinitions node) {
    for (Link<Node> link = node.definitions.nodes;
         !link.isEmpty();
         link = link.tail) {
      Node definition = link.head;
      if (definition is Identifier) {
        HInstruction initialValue = new HLiteral(null);
        add(initialValue);
        updateDefinition(definition, initialValue);
      } else {
        assert(definition is SendSet);
        visitSendSet(definition);
        pop();  // Discard value.
      }
    }
  }

  visitLiteralList(LiteralList node) {
    List<HInstruction> inputs = <HInstruction>[];
    for (Link<Node> link = node.elements.nodes;
         !link.isEmpty();
         link = link.tail) {
      visit(link.head);
      inputs.add(pop());
    }
    push(new HLiteralList(inputs));
  }
}
