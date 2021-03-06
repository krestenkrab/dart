// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// WARNING: Do not edit - generated code.

class _HTMLOptionsCollectionWrappingImplementation extends _HTMLCollectionWrappingImplementation implements HTMLOptionsCollection {
  _HTMLOptionsCollectionWrappingImplementation() : super() {}

  static create__HTMLOptionsCollectionWrappingImplementation() native {
    return new _HTMLOptionsCollectionWrappingImplementation();
  }

  int get length() { return _get_length_HTMLOptionsCollection(this); }
  static int _get_length_HTMLOptionsCollection(var _this) native;

  void set length(int value) { _set_length_HTMLOptionsCollection(this, value); }
  static void _set_length_HTMLOptionsCollection(var _this, int value) native;

  int get selectedIndex() { return _get_selectedIndex(this); }
  static int _get_selectedIndex(var _this) native;

  void set selectedIndex(int value) { _set_selectedIndex(this, value); }
  static void _set_selectedIndex(var _this, int value) native;

  void remove(int index) {
    _remove(this, index);
    return;
  }
  static void _remove(receiver, index) native;

  String get typeName() { return "HTMLOptionsCollection"; }
}
