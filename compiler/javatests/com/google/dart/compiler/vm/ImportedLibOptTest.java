// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.vm;

public class ImportedLibOptTest extends ImportedLibTests {
  @Override
  protected void runNegativeTest(String testName, String... commandArray) {
    super.runNegativeTest(testName, addOptimizeOption(commandArray));
  }

  @Override
  protected void runPositiveTest(String testName, String... commandArray) throws Throwable {
    super.runPositiveTest(testName, addOptimizeOption(commandArray));
  }
}
