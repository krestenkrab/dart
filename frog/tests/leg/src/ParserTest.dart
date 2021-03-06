// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#import('parser_helper.dart');

void testStatement(String statement) {
  var node = parseStatement(statement);
  print("$statement: $node");
}

void main() {
  testStatement('List<T> t;');
  testStatement('List<List<T>> t;');
  testStatement('List<List<List<T>>> t;');
  testStatement('List<List<List<List<T>>>> t;');
  testStatement('List<List<List<List<List<T>>>>> t;');

  testStatement('List<List<T> > t;');
  testStatement('List<List<List<T> >> t;');
  testStatement('List<List<List<List<T> >>> t;');
  testStatement('List<List<List<List<List<T> >>>> t;');

  testStatement('List<List<List<T> > > t;');
  testStatement('List<List<List<List<T> > >> t;');
  testStatement('List<List<List<List<List<T> > >>> t;');

  testStatement('List<List<List<List<T> > > > t;');
  testStatement('List<List<List<List<List<T> > > >> t;');

  testStatement('List<List<List<List<List<T> > > > > t;');

  testStatement('List<List<List<List<List<T> >> >> t;');

  testStatement('List<List<List<List<List<T> >>> > t;');

  testStatement('List<List<List<List<List<T >>> >> t;');

  testStatement('List<T> t;');
  testStatement('List<List<T>> t;');
  testStatement('List<List<List<T>>> t;');
  testStatement('List<List<List<List<T>>>> t;');
  testStatement('List<List<List<List<List<T>>>>> t;');

  testStatement('lib.List<List<T> > t;');
  testStatement('lib.List<List<List<T> >> t;');
  testStatement('lib.List<List<List<List<T> >>> t;');
  testStatement('lib.List<List<List<List<List<T> >>>> t;');

  testStatement('lib.List<List<List<T> > > t;');
  testStatement('lib.List<List<List<List<T> > >> t;');
  testStatement('lib.List<List<List<List<List<T> > >>> t;');

  testStatement('lib.List<List<List<List<T> > > > t;');
  testStatement('lib.List<List<List<List<List<T> > > >> t;');

  testStatement('lib.List<List<List<List<List<T> > > > > t;');

  testStatement('lib.List<List<List<List<List<T> >> >> t;');

  testStatement('lib.List<List<List<List<List<T> >>> > t;');

  testStatement('lib.List<List<List<List<List<T >>> >> t;');

  testStatement('x++;');
  // TODO(ahe): reenable following test.
  // testStatement('++x++;');
  testStatement('++x;');
  testStatement('print(x++);');
  // TODO(ahe): reenable following test.
  // testStatement('print(++x++);'); // Accepted by parser, rejected later.
  testStatement('print(++x);');

  testStatement('MyClass.foo().bar().baz();');
  testStatement('MyClass.foo().-x;'); // Accepted by parser, rejected later.
  testStatement('a.b.c.d();');

  testStatement('int f() {}');
  testStatement('void f() {}');

  testStatement('do fisk(); while (hest());');
  testStatement('do { fisk(); } while (hest());');

  testStatement('while (fisk()) hest();');
  testStatement('while (fisk()) { hest(); }');
}
