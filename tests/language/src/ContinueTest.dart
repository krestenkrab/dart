// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Dart test for continue in for, do/while and while loops.

class ContinueTest {
  static testMain() {
    int i;
    int forCounter = 0;
    for (i = 0; i < 10; i++) {
      if (i > 3) continue;
      forCounter++;
    }
    Expect.equals(4, forCounter);
    Expect.equals(10, i);

    i = 0;
    int doWhileCounter = 0;
    do {
      i++;
      if (i > 3) continue;
      doWhileCounter++;
    } while (i < 10);
    Expect.equals(3, doWhileCounter);
    Expect.equals(10, i);

    i = 0;
    int whileCounter = 0;
    while (i < 10) {
      i++;
      if (i > 3) continue;
      whileCounter++;
    }
    Expect.equals(3, whileCounter);
    Expect.equals(10, i);

    // Use a label to continue to the outer loop.
    i = 0;
    L: while (i < 50) {
      i += 3;
      while (i < 30) {
        i += 2;
        if (i < 10) {
          continue L;
        } else {
          i++;
          break;
        }
      }
      break;
    }
    Expect.equals(11, i);
  }
}

main() {
  ContinueTest.testMain();
}
