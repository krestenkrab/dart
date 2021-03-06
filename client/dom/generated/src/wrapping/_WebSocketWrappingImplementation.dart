// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// WARNING: Do not edit - generated code.

class _WebSocketWrappingImplementation extends DOMWrapperBase implements WebSocket {
  _WebSocketWrappingImplementation() : super() {}

  static create__WebSocketWrappingImplementation() native {
    return new _WebSocketWrappingImplementation();
  }

  String get URL() { return _get_URL(this); }
  static String _get_URL(var _this) native;

  String get binaryType() { return _get_binaryType(this); }
  static String _get_binaryType(var _this) native;

  void set binaryType(String value) { _set_binaryType(this, value); }
  static void _set_binaryType(var _this, String value) native;

  int get bufferedAmount() { return _get_bufferedAmount(this); }
  static int _get_bufferedAmount(var _this) native;

  String get extensions() { return _get_extensions(this); }
  static String _get_extensions(var _this) native;

  String get protocol() { return _get_protocol(this); }
  static String _get_protocol(var _this) native;

  int get readyState() { return _get_readyState(this); }
  static int _get_readyState(var _this) native;

  void addEventListener(String type, EventListener listener, [bool useCapture = null]) {
    if (useCapture === null) {
      _addEventListener_WebSocket(this, type, listener);
      return;
    } else {
      _addEventListener_WebSocket_2(this, type, listener, useCapture);
      return;
    }
  }
  static void _addEventListener_WebSocket(receiver, type, listener) native;
  static void _addEventListener_WebSocket_2(receiver, type, listener, useCapture) native;

  void close([int code = null, String reason = null]) {
    if (code === null) {
      if (reason === null) {
        _close(this);
        return;
      }
    } else {
      if (reason === null) {
        _close_2(this, code);
        return;
      } else {
        _close_3(this, code, reason);
        return;
      }
    }
    throw "Incorrect number or type of arguments";
  }
  static void _close(receiver) native;
  static void _close_2(receiver, code) native;
  static void _close_3(receiver, code, reason) native;

  bool dispatchEvent(Event evt) {
    return _dispatchEvent_WebSocket(this, evt);
  }
  static bool _dispatchEvent_WebSocket(receiver, evt) native;

  void removeEventListener(String type, EventListener listener, [bool useCapture = null]) {
    if (useCapture === null) {
      _removeEventListener_WebSocket(this, type, listener);
      return;
    } else {
      _removeEventListener_WebSocket_2(this, type, listener, useCapture);
      return;
    }
  }
  static void _removeEventListener_WebSocket(receiver, type, listener) native;
  static void _removeEventListener_WebSocket_2(receiver, type, listener, useCapture) native;

  bool send(String data) {
    return _send(this, data);
  }
  static bool _send(receiver, data) native;

  String get typeName() { return "WebSocket"; }
}
