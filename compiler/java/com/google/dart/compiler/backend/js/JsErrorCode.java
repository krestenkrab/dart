// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
package com.google.dart.compiler.backend.js;

import com.google.dart.compiler.ErrorCode;
import com.google.dart.compiler.ErrorSeverity;
import com.google.dart.compiler.SubSystem;

/**
 * {@link ErrorCode}s for JavaScript backend.
 */
public enum JsErrorCode implements ErrorCode {
  INTERNAL_ERROR("internal error: %s");
  private final ErrorSeverity severity;
  private final String message;

  /**
   * Initialize a newly created error code to have the given message and ERROR severity.
   */
  private JsErrorCode(String message) {
    this(ErrorSeverity.ERROR, message);
  }

  /**
   * Initialize a newly created error code to have the given severity and message.
   */
  private JsErrorCode(ErrorSeverity severity, String message) {
    this.severity = severity;
    this.message = message;
  }

  public String getMessage() {
    return message;
  }

  public ErrorSeverity getErrorSeverity() {
    return severity;
  }

  public SubSystem getSubSystem() {
    return SubSystem.JS_BACKEND;
  }
}