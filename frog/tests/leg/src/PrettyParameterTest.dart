// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Test that parameters keep their names in the output.

#import("compiler_helper.dart");

final String FOO = @"""
void foo(var a, var b) {
}
""";


final String BAR = @"""
void bar(var eval, var $eval) {
}
""";


final String PARAMETER_AND_TEMP = @"""
void print(var a) {}
void bar(var t0, var b) {
  {
    var t0 = 2;
    if (b) {
      t0 = 4;
    } else {
      t0 = 3;
    }
    print(t0);
  }
  print(t0);
}
""";

final String NO_LOCAL = @"""
foo(bar, baz) {
  if (bar) {
    baz = 2;
  } else {
    baz = 3;
  }
  return baz;
}
""";

final String MULTIPLE_PHIS_ONE_LOCAL = @"""
foo(param1, param2, param3) {
  var a = 2;
  if (param1) {
    if (param2) {
      if (param3) {
        a = 42;
      }
    }
  }
  return a;
}
""";

main() {
  String generated = compile(FOO, 'foo');
  // TODO(ngeoffray): Use 'contains' when frog supports it.
  RegExp regexp = const RegExp("function foo\\(a, b\\) {");
  Expect.isTrue(regexp.hasMatch(generated));

  generated = compile(BAR, 'bar');
  regexp = const RegExp("function bar\\(\\\$eval, _\\\$eval\\) {");
  Expect.isTrue(regexp.hasMatch(generated));

  generated = compile(PARAMETER_AND_TEMP, 'bar');
  regexp = const RegExp("print\\(t0\\)");
  Expect.isTrue(regexp.hasMatch(generated));
  // Check that the second 't0' got another name.
  regexp = const RegExp("print\\(t0_0\\)");
  Expect.isTrue(regexp.hasMatch(generated));

  generated = compile(NO_LOCAL, 'foo');
  regexp = const RegExp("return baz");
  Expect.isTrue(regexp.hasMatch(generated));
  regexp = const RegExp("baz = 2");
  Expect.isTrue(regexp.hasMatch(generated));
  regexp = const RegExp("baz = 3");
  Expect.isTrue(regexp.hasMatch(generated));
  regexp = const RegExp("bar === true");
  Expect.isTrue(regexp.hasMatch(generated));

  generated = compile(MULTIPLE_PHIS_ONE_LOCAL, 'foo');
  regexp = const RegExp("var a = 2;");
  Expect.isTrue(regexp.hasMatch(generated));

  regexp = const RegExp("a = 2;");
  Iterator matches = regexp.allMatches(generated).iterator();
  Expect.isTrue(matches.hasNext());
  matches.next();
  Expect.isFalse(matches.hasNext());
}
