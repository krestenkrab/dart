// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Separate entrypoint for the frog compiler with experimental support for the
// 'await' keyword.

#import('../lang.dart');
#import('../frog.dart', prefix:'frog');

// TODO(sigmund): add here a phase that performs the async transformations.
_awaitCompilationPhase() {
}

void main() {
  experimentalAwaitPhase = _awaitCompilationPhase;
  frog.main();
}
