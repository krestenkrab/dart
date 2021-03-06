// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Third dart test program.

class A {
  var a;
  static var s;

  static foo() {
    return s;
  }

  A(x, y) : a = x + y + 1 { }

  value() {
    return a + foo();
  }
}


class B {
  var b;
  static var s;

  static foo(x) {
    return x + s;
  }

  value() {
    return b + foo(s) + A.foo();
  }

  B(x) : b = x {
    b = b + 1;
  }
}


class ThirdTest {
  static testMain() {
    var a = new A(1, 2);
    var b = new B(3);
    A.s = 4;
    B.s = 5;
    Expect.equals(26, a.value() + b.value());
 }
}

main() {
  ThirdTest.testMain();
}
