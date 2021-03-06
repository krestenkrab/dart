// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.vm;

import junit.extensions.TestSetup;
import junit.framework.Test;
import junit.framework.TestSuite;

/**
 * @author floitsch@google.com (Florian Loitsch)
 */
public class DartTests extends TestSetup {

  public DartTests(TestSuite test) {
    super(test);
  }

  public static Test suite() {
    TestSuite suite = new TestSuite("Dart imported vm test suite.");

    suite.addTestSuite(ImportedDartTests.class);
    return new DartTests(suite);
  }
}
