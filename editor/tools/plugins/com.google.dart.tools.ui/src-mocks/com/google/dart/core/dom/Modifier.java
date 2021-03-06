/*
 * Copyright (c) 2011, the Dart project authors.
 *
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.core.dom;

/**
 * TODO(brianwilkerson): This is a temporary interface, used to resolve compilation errors.
 */
public class Modifier {
  public static final int NONE = 0;
  public static final int PUBLIC = 1;
  public static final int PROTECTED = 2;
  public static final int PRIVATE = 4;
  public static final int FINAL = 8;
  public static final int SYNCHRONIZED = 16;

  public static boolean isAbstract(int modifiers) {
    return false;
  }

  public static boolean isFinal(int modifiers) {
    return false;
  }

  public static boolean isPrivate(int modifiers) {
    return false;
  }

  public static boolean isProtected(int modifiers) {
    return false;
  }

  public static boolean isPublic(int modifiers) {
    return false;
  }

  public static boolean isStatic(int modifiers) {
    return false;
  }

  public static boolean isSynchronized(int modifiers) {
    return false;
  }

  public static boolean isTransient(int modifiers) {
    return false;
  }

  public static boolean isVolatile(int modifiers) {
    return false;
  }
}
