// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// WARNING: Do not edit - generated code.

class _CanvasPixelArrayWrappingImplementation extends DOMWrapperBase implements CanvasPixelArray {
  _CanvasPixelArrayWrappingImplementation() : super() {}

  static create__CanvasPixelArrayWrappingImplementation() native {
    return new _CanvasPixelArrayWrappingImplementation();
  }

  int get length() { return _get_length(this); }
  static int _get_length(var _this) native;

  int operator[](int index) { return _index(this, index); }
  static int _index(var _this, int index) native;

  void operator[]=(int index, int value) {
    return _set_index(this, index, value);
  }
  static _set_index(_this, index, value) native;

  void add(int value) {
    throw new UnsupportedOperationException("Cannot add to immutable List.");
  }

  void addLast(int value) {
    throw new UnsupportedOperationException("Cannot add to immutable List.");
  }

  void addAll(Collection<int> collection) {
    throw new UnsupportedOperationException("Cannot add to immutable List.");
  }

  void sort(int compare(int a, int b)) {
    throw new UnsupportedOperationException("Cannot sort immutable List.");
  }

  void copyFrom(List<Object> src, int srcStart, int dstStart, int count) {
    throw new UnsupportedOperationException("This object is immutable.");
  }

  int indexOf(int element, [int start = 0]) {
    return _Lists.indexOf(this, element, start, this.length);
  }

  int lastIndexOf(int element, [int start = null]) {
    if (start === null) start = length - 1;
    return _Lists.lastIndexOf(this, element, start);
  }

  int clear() {
    throw new UnsupportedOperationException("Cannot clear immutable List.");
  }

  int removeLast() {
    throw new UnsupportedOperationException("Cannot removeLast on immutable List.");
  }

  int last() {
    return this[length - 1];
  }

  void forEach(void f(int element)) {
    _Collections.forEach(this, f);
  }

  Collection<int> filter(bool f(int element)) {
    return _Collections.filter(this, new List<int>(), f);
  }

  bool every(bool f(int element)) {
    return _Collections.every(this, f);
  }

  bool some(bool f(int element)) {
    return _Collections.some(this, f);
  }

  void setRange(int start, int length, List<int> from, [int startFrom]) {
    throw new UnsupportedOperationException("Cannot setRange on immutable List.");
  }

  void removeRange(int start, int length) {
    throw new UnsupportedOperationException("Cannot removeRange on immutable List.");
  }

  void insertRange(int start, int length, [int initialValue]) {
    throw new UnsupportedOperationException("Cannot insertRange on immutable List.");
  }

  List<int> getRange(int start, int length) {
    throw new NotImplementedException();
  }

  bool isEmpty() {
    return length == 0;
  }

  Iterator<int> iterator() {
    return new _FixedSizeListIterator<int>(this);
  }

  String get typeName() { return "CanvasPixelArray"; }
}
