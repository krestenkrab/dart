// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

final bool VERBOSE = false;

class Listener {
  void beginArguments(Token token) {
  }

  void endArguments(int count, Token beginToken, Token endToken) {
  }

  void beginBlock(Token token) {
  }

  void endBlock(int count, Token beginToken, Token endToken) {
  }

  void beginClassBody(Token token) {
  }

  void endClassBody(int memberCount, Token beginToken, Token endToken) {
  }

  void beginClassDeclaration(Token token) {
  }

  void endClassDeclaration(int interfacesCount, Token beginToken,
                           Token extendsKeyword, Token implementsKeyword,
                           Token endToken) {
  }

  void beginDoWhileStatement(Token token) {
  }

  void endDoWhileStatement(Token doKeyword, Token whileKeyword,
                           Token endToken) {
  }

  void beginExpressionStatement(Token token) {
  }

  void endExpressionStatement(Token token) {
  }

  void beginFormalParameter(Token token) {
  }

  void endFormalParameter(Token token) {
  }

  void beginFormalParameters(Token token) {
  }

  void endFormalParameters(int count, Token beginToken, Token endToken) {
  }

  void endField(Token beginToken, Token endToken) {
  }

  void beginForStatement(Token token) {
  }

  void endForStatement(Token beginToken, Token endToken) {
  }

  void beginFunction(Token token) {
  }

  void endFunction(Token token) {
  }

  void beginFunctionBody(Token token) {
  }

  void endFunctionBody(int count, Token beginToken, Token endToken) {
  }

  void handleNoFunctionBody(Token token) {
  }

  void beginFunctionName(Token token) {
  }

  void endFunctionName(Token token) {
  }

  void beginFunctionTypeAlias(Token token) {
  }

  void endFunctionTypeAlias(Token token) {
  }

  void beginIfStatement(Token token) {
  }

  void endIfStatement(Token ifToken, Token elseToken) {
  }

  void beginInitializedIdentifier(Token token) {
  }

  void endInitializedIdentifier() {
  }

  void beginInitializer(Token token) {
  }

  void endInitializer(Token assignmentOperator) {
  }

  void beginInitializers(Token token) {
  }

  void endInitializers(int count, Token beginToken, Token endToken) {
  }

  void handleNoInitializers() {
  }

  void beginInterface(Token token) {
  }

  void endInterface(Token token) {
  }

  void beginLibraryTag(Token token) {
  }

  void endLibraryTag(bool hasPrefix, Token beginToken, Token endToken) {
  }

  void beginLiteralMapEntry(Token token) {
  }

  void endLiteralMapEntry(Token token) {
  }

  void beginMember(Token token) {
  }

  void endMethod(Token beginToken, Token endToken) {
  }

  void beginOptionalFormalParameters(Token token) {
  }

  void endOptionalFormalParameters(int count,
                                   Token beginToken, Token endToken) {
  }

  void beginReturnStatement(Token token) {
  }

  void endReturnStatement(bool hasExpression,
                          Token beginToken, Token endToken) {
  }

  void beginSend(Token token) {
  }

  void endSend(Token token) {
  }

  void beginSwitchStatement(Token token) {
  }

  void endSwitchStatement(Token switchKeyword) {
  }

  void beginThrowStatement(Token token) {
  }

  void endThrowStatement(Token throwToken, Token endToken) {
  }

  void endRethrowStatement(Token throwToken, Token endToken) {
  }

  void beginTopLevelMember(Token token) {
  }

  void endTopLevelField(Token beginToken, Token endToken) {
  }

  void endTopLevelMethod(Token beginToken, Token endToken) {
  }

  void beginTryStatement(Token token) {
  }

  void handleCatchBlock(Token catchKeyword) {
  }

  void handleFinallyBlock(Token finallyKeyword) {
  }

  void endTryStatement(int catchCount, Token tryKeyword, Token finallyKeyword) {
  }

  void endType(int count, Token beginToken, Token endToken) {
  }

  void beginTypeArguments(Token token) {
  }

  void endTypeArguments(int count, Token beginToken, Token endToken) {
  }

  void handleNoTypeArguments(Token token) {
  }

  void beginTypeVariable(Token token) {
  }

  void endTypeVariable(Token token) {
  }

  void beginTypeVariables(Token token) {
  }

  void endTypeVariables(int count, Token beginToken, Token endToken) {
  }

  void beginVariablesDeclaration(Token token) {
  }

  void endVariablesDeclaration(int count, Token endToken) {
  }

  void beginWhileStatement(Token token) {
  }

  void endWhileStatement(Token whileKeyword, Token endToken) {
  }

  void handleAssignmentExpression(Token token) {
  }

  void handleBinaryExpression(Token token) {
  }

  void handleConditionalExpression(Token question, Token colon) {
  }

  void handleConstExpression(Token token, bool named) {
  }

  void handleFinalKeyword(Token finalKeyword) {
  }

  void handleFunctionTypedFormalParameter(Token token) {
  }

  void handleIdentifier(Token token) {
  }

  void handleIndexedExpression(Token openCurlyBracket,
                               Token closeCurlyBracket) {
  }

  void handleLiteralBool(Token token) {
  }

  void handleBreakStatement(bool hasTarget,
                            Token breakKeyword, Token endToken) {
  }

  void handleContinueStatement(bool hasTarget,
                               Token continueKeyword, Token endToken) {
  }

  void handleEmptyStatement(Token token) {
  }

  void handleLiteralDouble(Token token) {
  }

  void handleLiteralInt(Token token) {
  }

  void handleLiteralList(int count, Token beginToken, Token endToken) {
  }

  void handleLiteralMap(int count, Token beginToken, Token endToken) {
  }

  void handleLiteralNull(Token token) {
  }

  void handleLiteralString(Token token) {
  }

  void handleNamedArgument(Token colon) {
  }

  void handleNewExpression(Token token, bool named) {
  }

  void handleNoArguments(Token token) {
  }

  void handleNoFieldInitializer(Token token) {
  }

  void handleNoType(Token token) {
  }

  void handleNoTypeVariables(Token token) {
  }

  void handleOperatorName(Token operatorKeyword, Token token) {
  }

  void handleParenthesizedExpression(BeginGroupToken token) {
  }

  void handleStringInterpolationPart(Token token) {
  }

  void handleSuperExpression(Token token) {
  }

  void handleThisExpression(Token token) {
  }

  void handleUnaryPostfixAssignmentExpression(Token token) {
  }

  void handleUnaryPrefixExpression(Token token) {
  }

  void handleUnaryPrefixAssignmentExpression(Token token) {
  }

  void handleValuedFormalParameter(Token equals, Token token) {
  }

  void handleVarKeyword(Token token) {
  }

  void handleVoidKeyword(Token token) {
  }

  Token expected(String string, Token token) {
    error("Expected '$string', but got '$token'", token);
  }

  void expectedIdentifier(Token token) {
    error("Expected identifier, but got '$token'", token);
  }

  Token expectedType(Token token) {
    error("Expected a type, but got '$token'", token);
  }

  Token expectedBlock(Token token) {
    error("Expected a block, but got '$token'", token);
  }

  Token unexpected(Token token) {
    error("Unexpected token '$token'", token);
  }

  void error(String message, Token token) {
    throw new ParserError("$message @ ${token.charOffset}");
  }
}

class ParserError {
  final String reason;
  ParserError(this.reason);
  toString() => reason;
}

/**
 * A listener for parser events.
 */
class ElementListener extends Listener {
  final Canceler canceler;

  Link<Node> nodes = const EmptyLink<Node>();
  Link<Element> topLevelElements = const EmptyLink<Element>();

  ElementListener(Canceler this.canceler);

  void beginLibraryTag(Token token) {
    // TODO(ahe): Implement this.
    canceler.cancel("Cannot handle library tags", token: token);
  }

  void endClassDeclaration(int interfacesCount, Token beginToken,
                           Token extendsKeyword, Token implementsKeyword,
                           Token endToken) {
    discardNodes(interfacesCount);
    Identifier supertype = popNode();
    Identifier name = popNode();
    pushElement(new PartialClassElement(name.source, beginToken, endToken));
  }

  void endInterface(Token token) {
    // TODO(ahe): Implement this.
    canceler.cancel("Cannot handle interfaces", token: token);
  }

  void endFunctionTypeAlias(Token token) {
    // TODO(ahe): Implement this.
    canceler.cancel("Cannot handle typedefs", token: token);
  }

  void endTopLevelMethod(Token beginToken, Token endToken) {
    Identifier name = popNode();
    pushElement(new PartialFunctionElement(name.source, beginToken, endToken));
  }

  void endTopLevelField(Token beginToken, Token endToken) {
    Identifier name = popNode();
    // TODO(ahe): Implement this.
    canceler.cancel("Cannot handle fields", token: beginToken);
  }

  void handleIdentifier(Token token) {
    pushNode(new Identifier(token));
  }

  void handleNoType(Token token) {
    pushNode(null);
  }

  void endTypeVariable(Token token) {
    TypeAnnotation bound = popNode();
    Identifier name = popNode();
  }

  void endTypeArguments(int count, Token beginToken, Token endToken) {
    discardNodes(count);
  }

  void handleParenthesizedExpression(BeginGroupToken token) {
    Expression expression = popNode();
    pushNode(new ParenthesizedExpression(expression, token));
  }

  void discardNodes(int count) {
    for (; count > 0; --count) {
      popNode();
    }
  }

  Token expected(String string, Token token) {
    canceler.cancel("Expected '$string', but got '$token'", token: token);
  }

  void expectedIdentifier(Token token) {
    canceler.cancel("Expected identifier, but got '$token'", token: token);
  }

  Token expectedType(Token token) {
    canceler.cancel("Expected a type, but got '$token'", token: token);
  }

  Token expectedBlock(Token token) {
    canceler.cancel("Expected a block, but got '$token'", token: token);
  }

  Token unexpected(Token token) {
    canceler.cancel("Unexpected token '$token'", token: token);
  }

  void pushElement(Element element) {
    topLevelElements = topLevelElements.prepend(element);
  }

  void pushNode(Node node) {
    nodes = nodes.prepend(node);
    if (VERBOSE) log("push $nodes");
  }

  Node popNode() {
    assert(!nodes.isEmpty());
    Node node = nodes.head;
    nodes = nodes.tail;
    if (VERBOSE) log("pop $nodes");
    return node;
  }

  void log(message) {
  }
}

class NodeListener extends ElementListener {
  final Logger logger;
  Link<Element> memberElements = const EmptyLink<Element>();

  NodeListener(Canceler canceler, Logger this.logger) : super(canceler);

  void endClassDeclaration(int interfacesCount, Token beginToken,
                           Token extendsKeyword, Token implementsKeyword,
                           Token endToken) {
    NodeList body = popNode();
    NodeList interfaces =
        makeNodeList(interfacesCount, implementsKeyword, null, ",");
    TypeAnnotation supertype = popNode();
    // TODO(ahe): Type variables.
    Identifier name = popNode();
    pushNode(new ClassNode(name, supertype, interfaces, beginToken,
                           extendsKeyword, endToken));
  }

  void endClassBody(int memberCount, Token beginToken, Token endToken) {
    pushNode(makeNodeList(memberCount, beginToken, endToken, null));
  }

  void endFormalParameter(Token token) {
    NodeList name = new NodeList.singleton(popNode());
    TypeAnnotation type = popNode();
    pushNode(new VariableDefinitions(type, null, name, token));
  }

  void endFormalParameters(int count, Token beginToken, Token endToken) {
    pushNode(makeNodeList(count, beginToken, endToken, ","));
  }

  void endArguments(int count, Token beginToken, Token endToken) {
    pushNode(makeNodeList(count, beginToken, endToken, ","));
  }

  void handleNoArguments(Token token) {
    pushNode(null);
  }

  void endReturnStatement(bool hasExpression,
                          Token beginToken, Token endToken) {
    Expression expression = hasExpression ? popNode() : null;
    pushNode(new Return(beginToken, endToken, expression));
  }

  void endExpressionStatement(Token token) {
    pushNode(new ExpressionStatement(popNode(), token));
  }

  void handleOnError(Token token, var error) {
    canceler.cancel("internal error: '${token.value}': ${error}", token: token);
  }

  void handleLiteralInt(Token token) {
    pushNode(new LiteralInt(token, (t, e) => handleOnError(t, e)));
  }

  void handleLiteralDouble(Token token) {
    pushNode(new LiteralDouble(token, (t, e) => handleOnError(t, e)));
  }

  void handleLiteralBool(Token token) {
    pushNode(new LiteralBool(token, (t, e) => handleOnError(t, e)));
  }

  void handleLiteralString(Token token) {
    pushNode(new LiteralString(token));
  }

  void handleLiteralNull(Token token) {
    pushNode(new LiteralNull(token));
  }

  void handleBinaryExpression(Token token) {
    Node argument = popNode();
    Node receiver = popNode();
    if ((token.stringValue === '.') &&
        (argument is Send) && (argument.asSend().receiver === null)) {
      pushNode(argument.asSend().copyWithReceiver(receiver));
    } else {
      // TODO(ahe): If token.stringValue === '.', the resolver should
      // reject this.
      NodeList arguments = new NodeList.singleton(argument);
      pushNode(new Send(receiver, new Operator(token), arguments));
    }
  }

  void handleAssignmentExpression(Token token) {
    Node arg = popNode();
    Node node = popNode();
    Send send = node.asSend();
    if (send === null) {
      canceler.cancel('not assignable: $node', node: node);
    }
    if (!(send.isPropertyAccess || send.isIndex)) {
      canceler.cancel('not assignable: $send', node: send);
    }
    if (send.asSendSet() !== null) {
      canceler.cancel('chained assignment', node: send);
    }
    NodeList arguments;
    if (send.isIndex) {
      Link<Node> link = new Link<Node>(arg);
      link = link.prepend(send.arguments.head);
      arguments = new NodeList(null, link);
    } else {
      arguments = new NodeList.singleton(arg);
    }
    Operator op = new Operator(token);
    pushNode(new SendSet(send.receiver, send.selector, op, arguments));
  }

  void handleConditionalExpression(Token question, Token colon) {
    Node elseExpression = popNode();
    Node thenExpression = popNode();
    Node condition = popNode();
    // TODO(ahe): Create an AST node.
    pushNode(null);
    canceler.cancel('conditional expression not implemented yet',
                    token: question);
  }

  void endSend(Token token) {
    NodeList arguments = popNode();
    Node selector = popNode();
    // TODO(ahe): Handle receiver.
    pushNode(new Send(null, selector, arguments));
  }

  void handleVoidKeyword(Token token) {
    pushNode(new TypeAnnotation(new Identifier(token)));
  }

  void endFunctionBody(int count, Token beginToken, Token endToken) {
    pushNode(new Block(makeNodeList(count, beginToken, endToken, null)));
  }

  void handleNoFunctionBody(Token token) {
    pushNode(null);
  }

  void endFunction(Token token) {
    Statement body = popNode();
    NodeList formals = popNode();
    Identifier name = popNode();
    // TODO(ahe): Return types are optional.
    TypeAnnotation type = popNode();
    pushNode(new FunctionExpression(name, formals, body, type));
  }

  void handleVarKeyword(Token token) {
    pushNode(new Identifier(token));
  }

  void handleFinalKeyword(Token token) {
    pushNode(new Identifier(token));
  }

  void endVariablesDeclaration(int count, Token endToken) {
    // TODO(ahe): Pick one name for this concept, either
    // VariablesDeclaration or VariableDefinitions.
    NodeList variables = makeNodeList(count, null, null, ",");
    TypeAnnotation type = popNode();
    pushNode(new VariableDefinitions(type, null, variables, endToken));
  }

  void endInitializer(Token assignmentOperator) {
    Expression initializer = popNode();
    NodeList arguments = new NodeList.singleton(initializer);
    Expression name = popNode();
    Operator op = new Operator(assignmentOperator);
    pushNode(new SendSet(null, name, op, arguments));
  }

  void endIfStatement(Token ifToken, Token elseToken) {
    Statement elsePart = (elseToken === null) ? null : popNode();
    Statement thenPart = popNode();
    ParenthesizedExpression condition = popNode();
    pushNode(new If(condition, thenPart, elsePart, ifToken, elseToken));
  }

  void endForStatement(Token beginToken, Token endToken) {
    Statement body = popNode();
    Expression update = popNode();
    ExpressionStatement condition = popNode();
    VariableDefinitions initializer = popNode();
    pushNode(new For(initializer, condition, update, body, beginToken));
  }

  void endDoWhileStatement(Token doKeyword, Token whileKeyword,
                           Token endToken) {
    Expression condition = popNode();
    Statement body = popNode();
    pushNode(new DoWhile(body, condition, doKeyword, whileKeyword, endToken));
  }

  void endWhileStatement(Token whileKeyword, Token endToken) {
    Statement body = popNode();
    Expression condition = popNode();
    pushNode(new While(condition, body, whileKeyword));
  }

  void endBlock(int count, Token beginToken, Token endToken) {
    pushNode(new Block(makeNodeList(count, beginToken, endToken, null)));
  }

  void endType(int count, Token beginToken, Token endToken) {
    TypeAnnotation type = new TypeAnnotation(popNode());
    // TODO(ahe): Don't discard library prefixes.
    discardNodes(count - 1); // Discard library prefixes.
    pushNode(type);
  }

  void endThrowStatement(Token throwToken, Token endToken) {
    Expression expression = popNode();
    pushNode(new Throw(expression, throwToken, endToken));
  }

  void endRethrowStatement(Token throwToken, Token endToken) {
    pushNode(new Throw(null, throwToken, endToken));
  }

  void handleUnaryPrefixExpression(Token token) {
    pushNode(new Send.prefix(popNode(), new Operator(token)));
  }

  void handleSuperExpression(Token token) {
    pushNode(new Identifier(token));
  }

  void handleThisExpression(Token token) {
    pushNode(new Identifier(token));
  }

  void handleUnaryAssignmentExpression(Token token, bool isPrefix) {
    Node node = popNode();
    Send send = node.asSend();
    if (send === null) {
      canceler.cancel('not assignable: $node', node: node);
    }
    if (!send.isPropertyAccess) {
      canceler.cancel('not assignable: $send', node: send);
    }
    if (send.asSendSet() !== null) {
      canceler.cancel('chained assignment', node: send);
    }
    Operator op = new Operator(token);
    if (isPrefix) {
      pushNode(new SendSet.prefix(send.receiver, send.selector, op));
    } else {
      pushNode(new SendSet.postfix(send.receiver, send.selector, op));
    }
  }
  void handleUnaryPostfixAssignmentExpression(Token token) {
    handleUnaryAssignmentExpression(token, false);
  }

  void handleUnaryPrefixAssignmentExpression(Token token) {
    handleUnaryAssignmentExpression(token, true);
  }

  void endInitializers(int count, Token beginToken, Token endToken) {
    // TODO(ahe): Preserve initializers.
    discardNodes(count);
  }

  void handleNoInitializers() {
    // TODO(ahe): pushNode(null);
  }

  void handleNoFieldInitializer(Token token) {
    pushNode(null);
  }

  void endField(Token beginToken, Token endToken) {
    canceler.cancel("fields are not implemented yet", token: beginToken);
    Expression initializer = popNode();
    Identifier name = popNode();
    // TODO(ahe): implement this.
    pushNode(null);
  }

  void endMethod(Token beginToken, Token endToken) {
    Statement body = popNode();
    Identifier name = popNode(); // TODO(ahe): What about constructors?
    // TODO(ahe): Save modifiers.
    pushNode(new FunctionExpression(name, null, null, null));
    // TODO(ahe): Record enclosing element?
    Element methodElement =
        new PartialFunctionElement(name.source, beginToken, endToken);
    memberElements = memberElements.prepend(methodElement);
  }

  void endLiteralMapEntry(Token token) {
    Expression value = popNode();
    LiteralString key = popNode();
    pushNode(null); // TODO(ahe): Create AST node.
  }

  void handleLiteralMap(int count, Token beginToken, Token endToken) {
    NodeList entries = makeNodeList(count, beginToken, endToken, ',');
    // TODO(ahe): Type arguments are discarded.
    pushNode(null); // TODO(ahe): Create AST node.
    canceler.cancel('literal map not implemented', node: entries);
  }

  void handleLiteralList(int count, Token beginToken, Token endToken) {
    NodeList elements = makeNodeList(count, beginToken, endToken, ',');
    // TODO(ahe): Type arguments are discarded.
    pushNode(new LiteralList(null, elements));
  }

  void handleIndexedExpression(Token openSquareBracket,
                               Token closeSquareBracket) {
    NodeList arguments =
        makeNodeList(1, openSquareBracket, closeSquareBracket, null);
    Node receiver = popNode();
    Node selector = new Operator.synthetic('[]');
    pushNode(new Send(receiver, selector, arguments));
  }

  void handleNewExpression(Token token, bool named) {
    if (named) {
      canceler.cancel('named constructors are not implemented', token: token);
    }
    NodeList arguments = popNode();
    TypeAnnotation type = popNode();
    pushNode(new NewExpression(token, new Send(null, type, arguments)));
  }

  void handleConstExpression(Token token, bool named) {
    canceler.cancel('const expressions are not implemented', token: token);
  }

  void handleOperatorName(Token operatorKeyword, Token token) {
    canceler.cancel('user defined operators are not implemented', token: token);
  }

  void handleNamedArgument(Token colon) {
    canceler.cancel('named arguments are not implemented', token: colon);
  }

  void handleStringInterpolationPart(Token token) {
    canceler.cancel('string interpolation is not implemented', token: token);
  }

  void endOptionalFormalParameters(int count,
                                   Token beginToken, Token endToken) {
    canceler.cancel('optional formal paramters are not implemented',
                    token: beginToken);
  }

  void handleFunctionTypedFormalParameter(Token token) {
    canceler.cancel('function typed formal paramters are not implemented',
                    token: token);
  }

  void handleValuedFormalParameter(Token equals, Token token) {
    canceler.cancel('formal paramters are not implemented',
                    token: equals);
  }

  void beginTryStatement(Token token) {
    canceler.cancel('try statement is not implemented', token: token);
  }

  void endSwitchStatement(Token switchKeyword) {
    canceler.cancel('switch statement is not implemented',
                    token: switchKeyword);
  }

  void handleBreakStatement(bool hasTarget,
                            Token breakKeyword, Token endToken) {
    canceler.cancel('break statement is not implemented', token: breakKeyword);
  }

  void handleContinueStatement(bool hasTarget,
                               Token continueKeyword, Token endToken) {
    canceler.cancel('continue statement is not implemented',
                    token: continueKeyword);
  }

  void handleEmptyStatement(Token token) {
    canceler.cancel('empty statement is not implemented', token: token);
  }

  NodeList makeNodeList(int count, Token beginToken, Token endToken,
                        String delimiter) {
    Link<Node> nodes = const EmptyLink<Node>();
    for (; count > 0; --count) {
      // This effectively reverses the order of nodes so they end up
      // in correct (source) order.
      nodes = nodes.prepend(popNode());
    }
    SourceString sourceDelimiter =
        (delimiter === null) ? null : new SourceString(delimiter);
    return new NodeList(beginToken, nodes, endToken, sourceDelimiter);
  }

  void log(message) {
    logger.log(message);
  }
}

class PartialFunctionElement extends FunctionElement {
  final Token beginToken;
  final Token endToken;

  PartialFunctionElement(SourceString name,
                         Token this.beginToken, Token this.endToken)
    : super(name, ElementKind.FUNCTION, null);

  FunctionExpression parseNode(Canceler canceler, Logger logger) {
    if (node != null) return node;
    node = parse(canceler, logger, (p) => p.parseFunction(beginToken));
    return node;
  }
}

Node parse(Canceler canceler, Logger logger, doParse(Parser parser)) {
  NodeListener listener = new NodeListener(canceler, logger);
  doParse(new Parser(listener));
  Node node = listener.popNode();
  assert(listener.nodes.isEmpty());
  return node;
}
