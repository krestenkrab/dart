/*
 * Copyright (c) 2011, the Dart project authors.
 * 
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.tools.core.internal.completion.ast;

import com.google.dart.compiler.ast.DartExpression;
import com.google.dart.compiler.ast.DartParameter;
import com.google.dart.compiler.ast.DartTypeNode;
import com.google.dart.compiler.ast.Modifiers;
import com.google.dart.tools.core.internal.completion.Mark;

import java.util.List;
import java.util.Stack;

public class ParameterCompleter extends DartParameter implements CompletionNode {
  private static final long serialVersionUID = 1L;

  public static ParameterCompleter from(DartParameter p) {
    return CompletionUtil.init(
        new ParameterCompleter(p.getName(), p.getTypeNode(), p.getFunctionParameters(),
            p.getDefaultExpr(), p.getModifiers()), p);
  }

  private Stack<Mark> stack;

  public ParameterCompleter(DartExpression name, DartTypeNode typeNode,
      List<DartParameter> functionParameters, DartExpression defaultExpr, Modifiers modifiers) {
    super(name, typeNode, functionParameters, defaultExpr, modifiers);
  }

  @Override
  public Stack<Mark> getCompletionParsingContext() {
    return stack;
  }

  @Override
  public void setCompletionParsingContext(Stack<Mark> stack) {
    this.stack = stack;
  }
}
