// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// WARNING: Do not edit - generated code.

class SVGFilterElementWrappingImplementation extends SVGElementWrappingImplementation implements SVGFilterElement {
  SVGFilterElementWrappingImplementation._wrap(ptr) : super._wrap(ptr) {}

  SVGAnimatedInteger get filterResX() { return LevelDom.wrapSVGAnimatedInteger(_ptr.filterResX); }

  SVGAnimatedInteger get filterResY() { return LevelDom.wrapSVGAnimatedInteger(_ptr.filterResY); }

  SVGAnimatedEnumeration get filterUnits() { return LevelDom.wrapSVGAnimatedEnumeration(_ptr.filterUnits); }

  SVGAnimatedLength get height() { return LevelDom.wrapSVGAnimatedLength(_ptr.height); }

  SVGAnimatedEnumeration get primitiveUnits() { return LevelDom.wrapSVGAnimatedEnumeration(_ptr.primitiveUnits); }

  SVGAnimatedLength get width() { return LevelDom.wrapSVGAnimatedLength(_ptr.width); }

  SVGAnimatedLength get x() { return LevelDom.wrapSVGAnimatedLength(_ptr.x); }

  SVGAnimatedLength get y() { return LevelDom.wrapSVGAnimatedLength(_ptr.y); }

  void setFilterRes(int filterResX, int filterResY) {
    _ptr.setFilterRes(filterResX, filterResY);
    return;
  }

  // From SVGURIReference

  SVGAnimatedString get href() { return LevelDom.wrapSVGAnimatedString(_ptr.href); }

  // From SVGLangSpace

  String get xmllang() { return _ptr.xmllang; }

  void set xmllang(String value) { _ptr.xmllang = value; }

  String get xmlspace() { return _ptr.xmlspace; }

  void set xmlspace(String value) { _ptr.xmlspace = value; }

  // From SVGExternalResourcesRequired

  SVGAnimatedBoolean get externalResourcesRequired() { return LevelDom.wrapSVGAnimatedBoolean(_ptr.externalResourcesRequired); }

  // From SVGStylable

  SVGAnimatedString get className() { return LevelDom.wrapSVGAnimatedString(_ptr.className); }

  CSSStyleDeclaration get style() { return LevelDom.wrapCSSStyleDeclaration(_ptr.style); }

  CSSValue getPresentationAttribute(String name) {
    return LevelDom.wrapCSSValue(_ptr.getPresentationAttribute(name));
  }
}
