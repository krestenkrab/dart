// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Test that a timeout handler can cancel itself.
class TimerCancel2Test {
  static void testSelfCancel() {
    var cancelTimer;

    void cancelHandler(Timer timer) {
      cancelTimer.cancel();
    }

    cancelTimer = new Timer(cancelHandler, 1, true);
  }

  static void testMain() {
    testSelfCancel();
  }
}

main() {
  TimerCancel2Test.testMain();
}
