// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

interface HVisitor<R> {
  R visitAdd(HAdd node);
  R visitBasicBlock(HBasicBlock node);
  R visitDivide(HDivide node);
  R visitExit(HExit node);
  R visitGoto(HGoto node);
  R visitInvoke(HInvoke node);
  R visitLiteral(HLiteral node);
  R visitSubtract(HSubtract node);
  R visitMultiply(HMultiply node);
  R visitReturn(HReturn node);
  R visitTruncatingDivide(HTruncatingDivide node);
}

class HGraphVisitor {
  visitDominatorTree(HGraph graph) {
    void visitBasicBlockAndSuccessors(HBasicBlock block) {
      // TODO(floitsch): visit graph in dominator-tree order.
      visitBasicBlock(block);
      for (int i = 0; i < block.successors.length; i++) {
        visitBasicBlockAndSuccessors(block.successors[i]);
      }
    }

    visitBasicBlockAndSuccessors(graph.entry);
  }

  visitPostDominatorTree(HGraph graph) {
    void visitBasicBlockAndSuccessors(HBasicBlock block) {
      // TODO(floitsch): visit graph in inverse dominator-tree order.
      for (int i = 0; i < block.successors.length; i++) {
        visitBasicBlockAndSuccessors(block.successors[i]);
      }
      visitBasicBlock(block);
    }

    visitBasicBlockAndSuccessors(graph.entry);
  }

  abstract visitBasicBlock(HBasicBlock block);
}

class HInstructionVisitor extends HGraphVisitor {
  HBasicBlock currentBlock;

  abstract visitInstruction(HInstruction node);

  visitBasicBlock(HBasicBlock node) {
    currentBlock = node;
    HInstruction instruction = node.first;
    while (instruction !== null) {
      visitInstruction(instruction);
      instruction = instruction.next;
    }
  }
}

class HGraph {
  HBasicBlock entry;
  HBasicBlock exit;
  HGraph() : entry = new HBasicBlock(), exit = new HBasicBlock() {
    exit.add(new HExit());
  }

  void number() {
    int basicBlockId = 0;
    int numberBasicBlockAndSuccessors(HBasicBlock block, int id) {
      id = block.number(basicBlockId++, id);
      for (int i = 0; i < block.successors.length; i++) {
        id = numberBasicBlockAndSuccessors(block.successors[i], id);
      }
      return id;
    }

    numberBasicBlockAndSuccessors(entry, 0);
  }

  void setSuccessors(HBasicBlock source, List<HBasicBlock> targets) {
    assert((source.last is HGoto || source.last is HReturn) &&
           targets.length == 1);
    assert(source.successors.isEmpty());
    source.successors = targets;
    for (int i = 0; i < targets.length; i++) {
      targets[i].predecessors.add(source);
    }
  }

  bool isValid() {
    HValidator validator = new HValidator();
    validator.visitGraph(this);
    return validator.isValid;
  }
}

class HBaseVisitor extends HGraphVisitor implements HVisitor {
  HBasicBlock currentBlock;

  visitBasicBlock(HBasicBlock node) {
    currentBlock = node;
    HInstruction instruction = node.first;
    while (instruction !== null) {
      instruction.accept(this);
      instruction = instruction.next;
    }
  }

  visitInstruction(HInstruction) {}

  visitArithmetic(HArithmetic node) => visitInvoke(node);

  visitAdd(HAdd node) => visitArithmetic(node);
  visitDivide(HDivide node) => visitArithmetic(node);
  visitExit(HExit node) => visitInstruction(node);
  visitGoto(HGoto node) => visitInstruction(node);
  visitInvoke(HInvoke node) => visitInstruction(node);
  visitLiteral(HLiteral node) => visitInstruction(node);
  visitSubtract(HSubtract node) => visitArithmetic(node);
  visitMultiply(HMultiply node) => visitArithmetic(node);
  visitReturn(HReturn node) => visitInstruction(node);
  visitTruncatingDivide(HTruncatingDivide node) => visitArithmetic(node);
}

class HBasicBlock {
  int id;
  HInstruction first = null;
  HInstruction last = null;
  final List<HBasicBlock> predecessors;
  List<HBasicBlock> successors;

  HBasicBlock()
      : predecessors = <HBasicBlock>[], successors = const <HBasicBlock>[];

  // TODO(kasperl): I really don't want to pass the compiler into this
  // method. Maybe we need a better logging framework.
  void print(Compiler compiler) {
    HInstruction instruction = first;
    while (instruction != null) {
      var id = instruction.id;
      compiler.log('$id: $instruction ${instruction.inputsToString()}');
      instruction = instruction.next;
    }
  }

  // TODO(kasperl): Temporary helper method to number the instructions
  // in this basic block for printing and debugging purposes.
  int number(int basicBlockId, int id) {
    this.id = basicBlockId;
    HInstruction instruction = first;
    while (instruction != null) {
      instruction.id = id++;
      instruction = instruction.next;
    }
    return id;
  }

  accept(HVisitor visitor) => visitor.visitBasicBlock(this);

  void add(HInstruction instruction) {
    addAfter(last, instruction);
  }

  void addAfter(HInstruction cursor, HInstruction instruction) {
    if (cursor === null) {
      first = last = instruction;
    } else if (cursor === last) {
      last.next = instruction;
      instruction.previous = last;
      last = instruction;
    } else {
      instruction.previous = cursor;
      instruction.next = cursor.next;
      cursor.next.previous = instruction;
      cursor.next = instruction;
    }
    instruction.notifyAddedToBlock();
  }

  void remove(HInstruction instruction) {
    assert(instruction.isInBasicBlock());
    assert(instruction.usedBy.isEmpty());
    if (instruction.previous === null) {
      first = instruction.next;
    } else {
      instruction.previous.next = instruction.next;
    }
    if (instruction.next === null) {
      last = instruction.previous;
    } else {
      instruction.next.previous = instruction.previous;
    }
    instruction.notifyRemovedFromBlock();
  }

  /**
   * Rewrites all uses of the [from] instruction to using the [to]
   * instruction instead.
   */
  void rewrite(HInstruction from, HInstruction to) {
    for (HInstruction use in from.usedBy) {
      rewriteInput(use, from, to);
    }
    to.usedBy.addAll(from.usedBy);
    from._usedBy = [];
    assert(isValid());
  }

  static void rewriteInput(HInstruction instruction,
                           HInstruction from,
                           HInstruction to) {
    List inputs = instruction.inputs;
    for (int i = 0; i < inputs.length; i++) {
      if (inputs[i] === from) inputs[i] = to;
    }
  }

  bool isExitBlock() {
    return first === last && first is HExit;
  }

  bool isValid() {
    HValidator validator = new HValidator();
    validator.visitBasicBlock(this);
    return validator.isValid;
  }
}

class HInstruction {
  int id;
  final List<HInstruction> inputs;
  List<HInstruction> _usedBy = null;  // If [null] then the instruction is not
                                      // in a basic block.
  HInstruction previous = null;
  HInstruction next = null;
  int flags = 0;

  // Changes flags.
  static final int FLAG_CHANGES_SOMETHING    = 0;
  static final int FLAG_CHANGES_COUNT        = FLAG_CHANGES_SOMETHING + 1;

  // Depends flags (one for each changes flag).
  static final int FLAG_DEPENDS_ON_SOMETHING = FLAG_CHANGES_COUNT;

  // Other flags.
  static final int FLAG_GENERATE_AT_USE_SITE = FLAG_DEPENDS_ON_SOMETHING + 1;
  static final int FLAG_USE_GVN              = FLAG_GENERATE_AT_USE_SITE + 1;

  HInstruction(this.inputs) {
    prepareGvn();
  }

  bool getFlag(int position) => (flags & (1 << position)) != 0;
  void setFlag(int position) { flags |= (1 << position); }
  void clearFlag(int position) { flags &= ~(1 << position); }

  static int computeDependsOnFlags(int flags) => flags << FLAG_CHANGES_COUNT;

  int getChangesFlags() => flags & ((1 << FLAG_CHANGES_COUNT) - 1);
  bool hasSideEffects() => getChangesFlags() != 0;
  void prepareGvn() { setAllSideEffects();  }

  void setAllSideEffects() { flags |= ((1 << FLAG_CHANGES_COUNT) - 1); }
  void clearAllSideEffects() { flags &= ~((1 << FLAG_CHANGES_COUNT) - 1); }

  bool generateAtUseSite() => getFlag(FLAG_GENERATE_AT_USE_SITE);
  void setGenerateAtUseSite() { setFlag(FLAG_GENERATE_AT_USE_SITE); }

  bool useGvn() => getFlag(FLAG_USE_GVN);
  void setUseGvn() { setFlag(FLAG_USE_GVN); }

  bool canBeSkipped() => generateAtUseSite();

  List<HInstruction> get usedBy() {
    if (_usedBy == null) return const <HInstruction>[];
    return _usedBy;
  }

  bool isInBasicBlock() => _usedBy !== null;

  String inputsToString() {
    void addAsCommaSeparated(StringBuffer buffer, List<HInstruction> list) {
      for (int i = 0; i < list.length; i++) {
        if (i != 0) buffer.add(', ');
        buffer.add("@${list[i].id}");
      }
    }

    StringBuffer buffer = new StringBuffer();
    buffer.add('(');
    addAsCommaSeparated(buffer, inputs);
    buffer.add(') - used at [');
    addAsCommaSeparated(buffer, usedBy);
    buffer.add(']');
    return buffer.toString();
  }

  bool equals(HInstruction other) {
    assert(useGvn() && other.useGvn());
    // Check that the type and the flags match.
    if (!typeEquals(other)) return false;
    if (flags != other.flags) return false;
    // Check that the inputs match.
    final int inputsLength = inputs.length;
    final List<HInstruction> otherInputs = other.inputs;
    if (inputsLength != otherInputs.length) return false;
    for (int i = 0; i < inputsLength; i++) {
      if (inputs[i] !== otherInputs[i]) return false;
    }
    // Check that the data in the instruction matches.
    return dataEquals(other);
  }

  // These methods should be overwritten by instructions that
  // participate in global value numbering.
  bool typeEquals(HInstruction other) => false;
  bool dataEquals(HInstruction other) => false;

  abstract accept(HVisitor visitor);

  void notifyAddedToBlock() {
    assert(!isInBasicBlock());
    _usedBy = <HInstruction>[];
    // Add [this] to the inputs' uses.
    for (int i = 0; i < inputs.length; i++) {
      inputs[i].usedBy.add(this);
    }
    assert(isValid());
  }

  void notifyRemovedFromBlock() {
    assert(isInBasicBlock());
    assert(usedBy.isEmpty());

    // Remove [this] from the inputs' uses.
    for (int i = 0; i < inputs.length; i++) {
      List inputUsedBy = inputs[i].usedBy;
      for (int j = 0; j < inputUsedBy.length; j++) {
        if (inputUsedBy[j] === this) {
          inputUsedBy[j] = inputUsedBy[inputUsedBy.length - 1];
          inputUsedBy.removeLast();
          break;
        }
      }
    }
    _usedBy = null;
    assert(isValid());
  }

  bool isLiteralNumber() => false;
  bool isLiteralString() => false;

  bool isValid() {
    HValidator validator = new HValidator();
    validator.visitInstruction(this);
    return validator.isValid;
  }
}

class HInvoke extends HInstruction {
  final SourceString selector;
  HInvoke(this.selector, inputs) : super(inputs);
  toString() => 'invoke: $selector';
  accept(HVisitor visitor) => visitor.visitInvoke(this);
}

class HArithmetic extends HInvoke {
  HArithmetic(selector, inputs) : super(selector, inputs);
  void prepareGvn() {
    // Arithmetic expressions can take part in global value numbering
    // and do not have any side-effects if the left-hand side is a
    // literal.
    if (inputs[0] is !HLiteral) return;
    clearAllSideEffects();
    setUseGvn();
  }
  abstract num evaluate(num a, num b);
}

class HAdd extends HArithmetic {
  HAdd(inputs) : super(const SourceString('+'), inputs);
  void prepareGvn() {
    // Only if the left-hand side is a literal number are we
    // sure the operation will not have any side-effects.
    if (!inputs[0].isLiteralNumber()) return;
    clearAllSideEffects();
    setUseGvn();
  }
  accept(HVisitor visitor) => visitor.visitAdd(this);
  num evaluate(num a, num b) => a + b;
  bool typeEquals(other) => other is HAdd;
  bool dataEquals(HInstruction other) => true;
}

class HDivide extends HArithmetic {
  HDivide(inputs) : super(const SourceString('/'), inputs);
  accept(HVisitor visitor) => visitor.visitDivide(this);
  num evaluate(num a, num b) => a / b;
  bool typeEquals(other) => other is HDivide;
  bool dataEquals(HInstruction other) => true;
}

class HMultiply extends HArithmetic {
  HMultiply(inputs) : super(const SourceString('*'), inputs);
  accept(HVisitor visitor) => visitor.visitMultiply(this);
  num evaluate(num a, num b) => a * b;
  bool typeEquals(other) => other is HMultiply;
  bool dataEquals(HInstruction other) => true;
}

class HSubtract extends HArithmetic {
  HSubtract(inputs) : super(const SourceString('-'), inputs);
  accept(HVisitor visitor) => visitor.visitSubtract(this);
  num evaluate(num a, num b) => a - b;
  bool typeEquals(other) => other is HSubtract;
  bool dataEquals(HInstruction other) => true;
}

class HTruncatingDivide extends HArithmetic {
  HTruncatingDivide(inputs) : super(const SourceString('~/'), inputs);
  accept(HVisitor visitor) => visitor.visitTruncatingDivide(this);
  num evaluate(num a, num b) => a ~/ b;
  bool typeEquals(other) => other is HTruncatingDivide;
  bool dataEquals(HInstruction other) => true;
}

class HExit extends HInstruction {
  HExit() : super(const <HInstruction>[]);
  canBeSkipped() => true;
  toString() => 'exit';
  accept(HVisitor visitor) => visitor.visitExit(this);
}

class HGoto extends HInstruction {
  HGoto() : super(const <HInstruction>[]);
  canBeSkipped() => true;
  toString() => 'goto';
  accept(HVisitor visitor) => visitor.visitGoto(this);
}

class HLiteral extends HInstruction {
  final value;
  HLiteral(this.value) : super([]);
  void prepareGvn() {
    clearAllSideEffects();
    setUseGvn();
  }
  toString() => 'literal: $value';
  accept(HVisitor visitor) => visitor.visitLiteral(this);
  bool isLiteralNumber() => value is num;
  bool isLiteralString() => value is String;
  bool typeEquals(other) => other is HLiteral;
  bool dataEquals(other) => value == other.value;
}

class HReturn extends HInstruction {
  HReturn(value) : super([value]);
  toString() => 'return';
  accept(HVisitor visitor) => visitor.visitReturn(this);
}
