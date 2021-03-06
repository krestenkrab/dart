// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// DO NOT EDIT
// Auto-generated Dart DOM library.

#library("dom");
#native("frog_dom.js");

#source('generated/src/frog/AbstractWorker.dart');
#source('generated/src/frog/ArrayBuffer.dart');
#source('generated/src/frog/ArrayBufferView.dart');
#source('generated/src/frog/Attr.dart');
#source('generated/src/frog/AudioBuffer.dart');
#source('generated/src/frog/AudioBufferSourceNode.dart');
#source('generated/src/frog/AudioChannelMerger.dart');
#source('generated/src/frog/AudioChannelSplitter.dart');
#source('generated/src/frog/AudioContext.dart');
#source('generated/src/frog/AudioDestinationNode.dart');
#source('generated/src/frog/AudioGain.dart');
#source('generated/src/frog/AudioGainNode.dart');
#source('generated/src/frog/AudioListener.dart');
#source('generated/src/frog/AudioNode.dart');
#source('generated/src/frog/AudioPannerNode.dart');
#source('generated/src/frog/AudioParam.dart');
#source('generated/src/frog/AudioProcessingEvent.dart');
#source('generated/src/frog/AudioSourceNode.dart');
#source('generated/src/frog/BarInfo.dart');
#source('generated/src/frog/BeforeLoadEvent.dart');
#source('generated/src/frog/BiquadFilterNode.dart');
#source('generated/src/frog/Blob.dart');
#source('generated/src/frog/CDATASection.dart');
#source('generated/src/frog/CSSCharsetRule.dart');
#source('generated/src/frog/CSSFontFaceRule.dart');
#source('generated/src/frog/CSSImportRule.dart');
#source('generated/src/frog/CSSMediaRule.dart');
#source('generated/src/frog/CSSPageRule.dart');
#source('generated/src/frog/CSSPrimitiveValue.dart');
#source('generated/src/frog/CSSRule.dart');
#source('generated/src/frog/CSSRuleList.dart');
#source('generated/src/frog/CSSStyleDeclaration.dart');
#source('generated/src/frog/CSSStyleRule.dart');
#source('generated/src/frog/CSSStyleSheet.dart');
#source('generated/src/frog/CSSUnknownRule.dart');
#source('generated/src/frog/CSSValue.dart');
#source('generated/src/frog/CSSValueList.dart');
#source('generated/src/frog/CanvasGradient.dart');
#source('generated/src/frog/CanvasPattern.dart');
#source('generated/src/frog/CanvasPixelArray.dart');
#source('generated/src/frog/CanvasRenderingContext.dart');
#source('generated/src/frog/CanvasRenderingContext2D.dart');
#source('generated/src/frog/CharacterData.dart');
#source('generated/src/frog/ClientRect.dart');
#source('generated/src/frog/ClientRectList.dart');
#source('generated/src/frog/Clipboard.dart');
#source('generated/src/frog/CloseEvent.dart');
#source('generated/src/frog/Comment.dart');
#source('generated/src/frog/CompositionEvent.dart');
#source('generated/src/frog/Console.dart');
#source('generated/src/frog/ConvolverNode.dart');
#source('generated/src/frog/Coordinates.dart');
#source('generated/src/frog/Counter.dart');
#source('generated/src/frog/Crypto.dart');
#source('generated/src/frog/CustomEvent.dart');
#source('generated/src/frog/DOMApplicationCache.dart');
#source('generated/src/frog/DOMException.dart');
#source('generated/src/frog/DOMFileSystem.dart');
#source('generated/src/frog/DOMFileSystemSync.dart');
#source('generated/src/frog/DOMFormData.dart');
#source('generated/src/frog/DOMImplementation.dart');
#source('generated/src/frog/DOMMimeType.dart');
#source('generated/src/frog/DOMMimeTypeArray.dart');
#source('generated/src/frog/DOMParser.dart');
#source('generated/src/frog/DOMPlugin.dart');
#source('generated/src/frog/DOMPluginArray.dart');
#source('generated/src/frog/DOMSelection.dart');
#source('generated/src/frog/DOMSettableTokenList.dart');
#source('generated/src/frog/DOMTokenList.dart');
#source('generated/src/frog/DOMURL.dart');
#source('generated/src/frog/DOMWindow.dart');
#source('generated/src/frog/DataTransferItem.dart');
#source('generated/src/frog/DataTransferItemList.dart');
#source('generated/src/frog/DataView.dart');
#source('generated/src/frog/Database.dart');
#source('generated/src/frog/DatabaseSync.dart');
#source('generated/src/frog/DedicatedWorkerContext.dart');
#source('generated/src/frog/DelayNode.dart');
#source('generated/src/frog/DeviceMotionEvent.dart');
#source('generated/src/frog/DeviceOrientationEvent.dart');
#source('generated/src/frog/DirectoryEntry.dart');
#source('generated/src/frog/DirectoryEntrySync.dart');
#source('generated/src/frog/DirectoryReader.dart');
#source('generated/src/frog/DirectoryReaderSync.dart');
#source('generated/src/frog/Document.dart');
#source('generated/src/frog/DocumentFragment.dart');
#source('generated/src/frog/DocumentType.dart');
#source('generated/src/frog/DynamicsCompressorNode.dart');
#source('generated/src/frog/Element.dart');
#source('generated/src/frog/ElementTimeControl.dart');
#source('generated/src/frog/ElementTraversal.dart');
#source('generated/src/frog/Entity.dart');
#source('generated/src/frog/EntityReference.dart');
#source('generated/src/frog/Entry.dart');
#source('generated/src/frog/EntryArray.dart');
#source('generated/src/frog/EntryArraySync.dart');
#source('generated/src/frog/EntrySync.dart');
#source('generated/src/frog/ErrorEvent.dart');
#source('generated/src/frog/Event.dart');
#source('generated/src/frog/EventException.dart');
#source('generated/src/frog/EventSource.dart');
#source('generated/src/frog/EventTarget.dart');
#source('generated/src/frog/File.dart');
#source('generated/src/frog/FileEntry.dart');
#source('generated/src/frog/FileEntrySync.dart');
#source('generated/src/frog/FileError.dart');
#source('generated/src/frog/FileException.dart');
#source('generated/src/frog/FileList.dart');
#source('generated/src/frog/FileReader.dart');
#source('generated/src/frog/FileReaderSync.dart');
#source('generated/src/frog/FileWriter.dart');
#source('generated/src/frog/FileWriterSync.dart');
#source('generated/src/frog/Float32Array.dart');
#source('generated/src/frog/Float64Array.dart');
#source('generated/src/frog/Geolocation.dart');
#source('generated/src/frog/Geoposition.dart');
#source('generated/src/frog/HTMLAllCollection.dart');
#source('generated/src/frog/HTMLAnchorElement.dart');
#source('generated/src/frog/HTMLAppletElement.dart');
#source('generated/src/frog/HTMLAreaElement.dart');
#source('generated/src/frog/HTMLAudioElement.dart');
#source('generated/src/frog/HTMLBRElement.dart');
#source('generated/src/frog/HTMLBaseElement.dart');
#source('generated/src/frog/HTMLBaseFontElement.dart');
#source('generated/src/frog/HTMLBodyElement.dart');
#source('generated/src/frog/HTMLButtonElement.dart');
#source('generated/src/frog/HTMLCanvasElement.dart');
#source('generated/src/frog/HTMLCollection.dart');
#source('generated/src/frog/HTMLDListElement.dart');
#source('generated/src/frog/HTMLDataListElement.dart');
#source('generated/src/frog/HTMLDetailsElement.dart');
#source('generated/src/frog/HTMLDirectoryElement.dart');
#source('generated/src/frog/HTMLDivElement.dart');
#source('generated/src/frog/HTMLDocument.dart');
#source('generated/src/frog/HTMLElement.dart');
#source('generated/src/frog/HTMLEmbedElement.dart');
#source('generated/src/frog/HTMLFieldSetElement.dart');
#source('generated/src/frog/HTMLFontElement.dart');
#source('generated/src/frog/HTMLFormElement.dart');
#source('generated/src/frog/HTMLFrameElement.dart');
#source('generated/src/frog/HTMLFrameSetElement.dart');
#source('generated/src/frog/HTMLHRElement.dart');
#source('generated/src/frog/HTMLHeadElement.dart');
#source('generated/src/frog/HTMLHeadingElement.dart');
#source('generated/src/frog/HTMLHtmlElement.dart');
#source('generated/src/frog/HTMLIFrameElement.dart');
#source('generated/src/frog/HTMLImageElement.dart');
#source('generated/src/frog/HTMLInputElement.dart');
#source('generated/src/frog/HTMLIsIndexElement.dart');
#source('generated/src/frog/HTMLKeygenElement.dart');
#source('generated/src/frog/HTMLLIElement.dart');
#source('generated/src/frog/HTMLLabelElement.dart');
#source('generated/src/frog/HTMLLegendElement.dart');
#source('generated/src/frog/HTMLLinkElement.dart');
#source('generated/src/frog/HTMLMapElement.dart');
#source('generated/src/frog/HTMLMarqueeElement.dart');
#source('generated/src/frog/HTMLMediaElement.dart');
#source('generated/src/frog/HTMLMenuElement.dart');
#source('generated/src/frog/HTMLMetaElement.dart');
#source('generated/src/frog/HTMLMeterElement.dart');
#source('generated/src/frog/HTMLModElement.dart');
#source('generated/src/frog/HTMLOListElement.dart');
#source('generated/src/frog/HTMLObjectElement.dart');
#source('generated/src/frog/HTMLOptGroupElement.dart');
#source('generated/src/frog/HTMLOptionElement.dart');
#source('generated/src/frog/HTMLOptionsCollection.dart');
#source('generated/src/frog/HTMLOutputElement.dart');
#source('generated/src/frog/HTMLParagraphElement.dart');
#source('generated/src/frog/HTMLParamElement.dart');
#source('generated/src/frog/HTMLPreElement.dart');
#source('generated/src/frog/HTMLProgressElement.dart');
#source('generated/src/frog/HTMLQuoteElement.dart');
#source('generated/src/frog/HTMLScriptElement.dart');
#source('generated/src/frog/HTMLSelectElement.dart');
#source('generated/src/frog/HTMLSourceElement.dart');
#source('generated/src/frog/HTMLSpanElement.dart');
#source('generated/src/frog/HTMLStyleElement.dart');
#source('generated/src/frog/HTMLTableCaptionElement.dart');
#source('generated/src/frog/HTMLTableCellElement.dart');
#source('generated/src/frog/HTMLTableColElement.dart');
#source('generated/src/frog/HTMLTableElement.dart');
#source('generated/src/frog/HTMLTableRowElement.dart');
#source('generated/src/frog/HTMLTableSectionElement.dart');
#source('generated/src/frog/HTMLTextAreaElement.dart');
#source('generated/src/frog/HTMLTitleElement.dart');
#source('generated/src/frog/HTMLTrackElement.dart');
#source('generated/src/frog/HTMLUListElement.dart');
#source('generated/src/frog/HTMLUnknownElement.dart');
#source('generated/src/frog/HTMLVideoElement.dart');
#source('generated/src/frog/HashChangeEvent.dart');
#source('generated/src/frog/HighPass2FilterNode.dart');
#source('generated/src/frog/History.dart');
#source('generated/src/frog/IDBAny.dart');
#source('generated/src/frog/IDBCursor.dart');
#source('generated/src/frog/IDBCursorWithValue.dart');
#source('generated/src/frog/IDBDatabase.dart');
#source('generated/src/frog/IDBDatabaseError.dart');
#source('generated/src/frog/IDBDatabaseException.dart');
#source('generated/src/frog/IDBFactory.dart');
#source('generated/src/frog/IDBIndex.dart');
#source('generated/src/frog/IDBKey.dart');
#source('generated/src/frog/IDBKeyRange.dart');
#source('generated/src/frog/IDBObjectStore.dart');
#source('generated/src/frog/IDBRequest.dart');
#source('generated/src/frog/IDBTransaction.dart');
#source('generated/src/frog/IDBVersionChangeEvent.dart');
#source('generated/src/frog/IDBVersionChangeRequest.dart');
#source('generated/src/frog/ImageData.dart');
#source('generated/src/frog/InjectedScriptHost.dart');
#source('generated/src/frog/InspectorFrontendHost.dart');
#source('generated/src/frog/Int16Array.dart');
#source('generated/src/frog/Int32Array.dart');
#source('generated/src/frog/Int8Array.dart');
#source('generated/src/frog/JavaScriptAudioNode.dart');
#source('generated/src/frog/JavaScriptCallFrame.dart');
#source('generated/src/frog/KeyboardEvent.dart');
#source('generated/src/frog/Location.dart');
#source('generated/src/frog/LowPass2FilterNode.dart');
#source('generated/src/frog/MediaElementAudioSourceNode.dart');
#source('generated/src/frog/MediaError.dart');
#source('generated/src/frog/MediaList.dart');
#source('generated/src/frog/MediaQueryList.dart');
#source('generated/src/frog/MediaQueryListListener.dart');
#source('generated/src/frog/MemoryInfo.dart');
#source('generated/src/frog/MessageChannel.dart');
#source('generated/src/frog/MessageEvent.dart');
#source('generated/src/frog/MessagePort.dart');
#source('generated/src/frog/Metadata.dart');
#source('generated/src/frog/MouseEvent.dart');
#source('generated/src/frog/MutationCallback.dart');
#source('generated/src/frog/MutationEvent.dart');
#source('generated/src/frog/MutationRecord.dart');
#source('generated/src/frog/NamedNodeMap.dart');
#source('generated/src/frog/Navigator.dart');
#source('generated/src/frog/NavigatorUserMediaError.dart');
#source('generated/src/frog/NavigatorUserMediaSuccessCallback.dart');
#source('generated/src/frog/Node.dart');
#source('generated/src/frog/NodeFilter.dart');
#source('generated/src/frog/NodeIterator.dart');
#source('generated/src/frog/NodeList.dart');
#source('generated/src/frog/NodeSelector.dart');
#source('generated/src/frog/Notation.dart');
#source('generated/src/frog/Notification.dart');
#source('generated/src/frog/NotificationCenter.dart');
#source('generated/src/frog/OESStandardDerivatives.dart');
#source('generated/src/frog/OESTextureFloat.dart');
#source('generated/src/frog/OESVertexArrayObject.dart');
#source('generated/src/frog/OfflineAudioCompletionEvent.dart');
#source('generated/src/frog/OperationNotAllowedException.dart');
#source('generated/src/frog/OverflowEvent.dart');
#source('generated/src/frog/PageTransitionEvent.dart');
#source('generated/src/frog/Performance.dart');
#source('generated/src/frog/PerformanceNavigation.dart');
#source('generated/src/frog/PerformanceTiming.dart');
#source('generated/src/frog/PopStateEvent.dart');
#source('generated/src/frog/PositionError.dart');
#source('generated/src/frog/ProcessingInstruction.dart');
#source('generated/src/frog/ProgressEvent.dart');
#source('generated/src/frog/RGBColor.dart');
#source('generated/src/frog/Range.dart');
#source('generated/src/frog/RangeException.dart');
#source('generated/src/frog/RealtimeAnalyserNode.dart');
#source('generated/src/frog/Rect.dart');
#source('generated/src/frog/SQLError.dart');
#source('generated/src/frog/SQLException.dart');
#source('generated/src/frog/SQLResultSet.dart');
#source('generated/src/frog/SQLResultSetRowList.dart');
#source('generated/src/frog/SQLTransaction.dart');
#source('generated/src/frog/SQLTransactionSync.dart');
#source('generated/src/frog/SVGAElement.dart');
#source('generated/src/frog/SVGAltGlyphDefElement.dart');
#source('generated/src/frog/SVGAltGlyphElement.dart');
#source('generated/src/frog/SVGAltGlyphItemElement.dart');
#source('generated/src/frog/SVGAngle.dart');
#source('generated/src/frog/SVGAnimateColorElement.dart');
#source('generated/src/frog/SVGAnimateElement.dart');
#source('generated/src/frog/SVGAnimateMotionElement.dart');
#source('generated/src/frog/SVGAnimateTransformElement.dart');
#source('generated/src/frog/SVGAnimatedAngle.dart');
#source('generated/src/frog/SVGAnimatedBoolean.dart');
#source('generated/src/frog/SVGAnimatedEnumeration.dart');
#source('generated/src/frog/SVGAnimatedInteger.dart');
#source('generated/src/frog/SVGAnimatedLength.dart');
#source('generated/src/frog/SVGAnimatedLengthList.dart');
#source('generated/src/frog/SVGAnimatedNumber.dart');
#source('generated/src/frog/SVGAnimatedNumberList.dart');
#source('generated/src/frog/SVGAnimatedPreserveAspectRatio.dart');
#source('generated/src/frog/SVGAnimatedRect.dart');
#source('generated/src/frog/SVGAnimatedString.dart');
#source('generated/src/frog/SVGAnimatedTransformList.dart');
#source('generated/src/frog/SVGAnimationElement.dart');
#source('generated/src/frog/SVGCircleElement.dart');
#source('generated/src/frog/SVGClipPathElement.dart');
#source('generated/src/frog/SVGColor.dart');
#source('generated/src/frog/SVGComponentTransferFunctionElement.dart');
#source('generated/src/frog/SVGCursorElement.dart');
#source('generated/src/frog/SVGDefsElement.dart');
#source('generated/src/frog/SVGDescElement.dart');
#source('generated/src/frog/SVGDocument.dart');
#source('generated/src/frog/SVGElement.dart');
#source('generated/src/frog/SVGElementInstance.dart');
#source('generated/src/frog/SVGElementInstanceList.dart');
#source('generated/src/frog/SVGEllipseElement.dart');
#source('generated/src/frog/SVGException.dart');
#source('generated/src/frog/SVGExternalResourcesRequired.dart');
#source('generated/src/frog/SVGFEBlendElement.dart');
#source('generated/src/frog/SVGFEColorMatrixElement.dart');
#source('generated/src/frog/SVGFEComponentTransferElement.dart');
#source('generated/src/frog/SVGFECompositeElement.dart');
#source('generated/src/frog/SVGFEConvolveMatrixElement.dart');
#source('generated/src/frog/SVGFEDiffuseLightingElement.dart');
#source('generated/src/frog/SVGFEDisplacementMapElement.dart');
#source('generated/src/frog/SVGFEDistantLightElement.dart');
#source('generated/src/frog/SVGFEDropShadowElement.dart');
#source('generated/src/frog/SVGFEFloodElement.dart');
#source('generated/src/frog/SVGFEFuncAElement.dart');
#source('generated/src/frog/SVGFEFuncBElement.dart');
#source('generated/src/frog/SVGFEFuncGElement.dart');
#source('generated/src/frog/SVGFEFuncRElement.dart');
#source('generated/src/frog/SVGFEGaussianBlurElement.dart');
#source('generated/src/frog/SVGFEImageElement.dart');
#source('generated/src/frog/SVGFEMergeElement.dart');
#source('generated/src/frog/SVGFEMergeNodeElement.dart');
#source('generated/src/frog/SVGFEMorphologyElement.dart');
#source('generated/src/frog/SVGFEOffsetElement.dart');
#source('generated/src/frog/SVGFEPointLightElement.dart');
#source('generated/src/frog/SVGFESpecularLightingElement.dart');
#source('generated/src/frog/SVGFESpotLightElement.dart');
#source('generated/src/frog/SVGFETileElement.dart');
#source('generated/src/frog/SVGFETurbulenceElement.dart');
#source('generated/src/frog/SVGFilterElement.dart');
#source('generated/src/frog/SVGFilterPrimitiveStandardAttributes.dart');
#source('generated/src/frog/SVGFitToViewBox.dart');
#source('generated/src/frog/SVGFontElement.dart');
#source('generated/src/frog/SVGFontFaceElement.dart');
#source('generated/src/frog/SVGFontFaceFormatElement.dart');
#source('generated/src/frog/SVGFontFaceNameElement.dart');
#source('generated/src/frog/SVGFontFaceSrcElement.dart');
#source('generated/src/frog/SVGFontFaceUriElement.dart');
#source('generated/src/frog/SVGForeignObjectElement.dart');
#source('generated/src/frog/SVGGElement.dart');
#source('generated/src/frog/SVGGlyphElement.dart');
#source('generated/src/frog/SVGGlyphRefElement.dart');
#source('generated/src/frog/SVGGradientElement.dart');
#source('generated/src/frog/SVGHKernElement.dart');
#source('generated/src/frog/SVGImageElement.dart');
#source('generated/src/frog/SVGLangSpace.dart');
#source('generated/src/frog/SVGLength.dart');
#source('generated/src/frog/SVGLengthList.dart');
#source('generated/src/frog/SVGLineElement.dart');
#source('generated/src/frog/SVGLinearGradientElement.dart');
#source('generated/src/frog/SVGLocatable.dart');
#source('generated/src/frog/SVGMPathElement.dart');
#source('generated/src/frog/SVGMarkerElement.dart');
#source('generated/src/frog/SVGMaskElement.dart');
#source('generated/src/frog/SVGMatrix.dart');
#source('generated/src/frog/SVGMetadataElement.dart');
#source('generated/src/frog/SVGMissingGlyphElement.dart');
#source('generated/src/frog/SVGNumber.dart');
#source('generated/src/frog/SVGNumberList.dart');
#source('generated/src/frog/SVGPaint.dart');
#source('generated/src/frog/SVGPathElement.dart');
#source('generated/src/frog/SVGPathSeg.dart');
#source('generated/src/frog/SVGPathSegArcAbs.dart');
#source('generated/src/frog/SVGPathSegArcRel.dart');
#source('generated/src/frog/SVGPathSegClosePath.dart');
#source('generated/src/frog/SVGPathSegCurvetoCubicAbs.dart');
#source('generated/src/frog/SVGPathSegCurvetoCubicRel.dart');
#source('generated/src/frog/SVGPathSegCurvetoCubicSmoothAbs.dart');
#source('generated/src/frog/SVGPathSegCurvetoCubicSmoothRel.dart');
#source('generated/src/frog/SVGPathSegCurvetoQuadraticAbs.dart');
#source('generated/src/frog/SVGPathSegCurvetoQuadraticRel.dart');
#source('generated/src/frog/SVGPathSegCurvetoQuadraticSmoothAbs.dart');
#source('generated/src/frog/SVGPathSegCurvetoQuadraticSmoothRel.dart');
#source('generated/src/frog/SVGPathSegLinetoAbs.dart');
#source('generated/src/frog/SVGPathSegLinetoHorizontalAbs.dart');
#source('generated/src/frog/SVGPathSegLinetoHorizontalRel.dart');
#source('generated/src/frog/SVGPathSegLinetoRel.dart');
#source('generated/src/frog/SVGPathSegLinetoVerticalAbs.dart');
#source('generated/src/frog/SVGPathSegLinetoVerticalRel.dart');
#source('generated/src/frog/SVGPathSegList.dart');
#source('generated/src/frog/SVGPathSegMovetoAbs.dart');
#source('generated/src/frog/SVGPathSegMovetoRel.dart');
#source('generated/src/frog/SVGPatternElement.dart');
#source('generated/src/frog/SVGPoint.dart');
#source('generated/src/frog/SVGPointList.dart');
#source('generated/src/frog/SVGPolygonElement.dart');
#source('generated/src/frog/SVGPolylineElement.dart');
#source('generated/src/frog/SVGPreserveAspectRatio.dart');
#source('generated/src/frog/SVGRadialGradientElement.dart');
#source('generated/src/frog/SVGRect.dart');
#source('generated/src/frog/SVGRectElement.dart');
#source('generated/src/frog/SVGRenderingIntent.dart');
#source('generated/src/frog/SVGSVGElement.dart');
#source('generated/src/frog/SVGScriptElement.dart');
#source('generated/src/frog/SVGSetElement.dart');
#source('generated/src/frog/SVGStopElement.dart');
#source('generated/src/frog/SVGStringList.dart');
#source('generated/src/frog/SVGStylable.dart');
#source('generated/src/frog/SVGStyleElement.dart');
#source('generated/src/frog/SVGSwitchElement.dart');
#source('generated/src/frog/SVGSymbolElement.dart');
#source('generated/src/frog/SVGTRefElement.dart');
#source('generated/src/frog/SVGTSpanElement.dart');
#source('generated/src/frog/SVGTests.dart');
#source('generated/src/frog/SVGTextContentElement.dart');
#source('generated/src/frog/SVGTextElement.dart');
#source('generated/src/frog/SVGTextPathElement.dart');
#source('generated/src/frog/SVGTextPositioningElement.dart');
#source('generated/src/frog/SVGTitleElement.dart');
#source('generated/src/frog/SVGTransform.dart');
#source('generated/src/frog/SVGTransformList.dart');
#source('generated/src/frog/SVGTransformable.dart');
#source('generated/src/frog/SVGURIReference.dart');
#source('generated/src/frog/SVGUnitTypes.dart');
#source('generated/src/frog/SVGUseElement.dart');
#source('generated/src/frog/SVGVKernElement.dart');
#source('generated/src/frog/SVGViewElement.dart');
#source('generated/src/frog/SVGViewSpec.dart');
#source('generated/src/frog/SVGZoomAndPan.dart');
#source('generated/src/frog/SVGZoomEvent.dart');
#source('generated/src/frog/Screen.dart');
#source('generated/src/frog/ScriptProfile.dart');
#source('generated/src/frog/ScriptProfileNode.dart');
#source('generated/src/frog/SharedWorker.dart');
#source('generated/src/frog/SharedWorkercontext.dart');
#source('generated/src/frog/SpeechInputEvent.dart');
#source('generated/src/frog/SpeechInputResult.dart');
#source('generated/src/frog/SpeechInputResultList.dart');
#source('generated/src/frog/Storage.dart');
#source('generated/src/frog/StorageEvent.dart');
#source('generated/src/frog/StorageInfo.dart');
#source('generated/src/frog/StyleMedia.dart');
#source('generated/src/frog/StyleSheet.dart');
#source('generated/src/frog/StyleSheetList.dart');
#source('generated/src/frog/Text.dart');
#source('generated/src/frog/TextEvent.dart');
#source('generated/src/frog/TextMetrics.dart');
#source('generated/src/frog/TextTrack.dart');
#source('generated/src/frog/TextTrackCue.dart');
#source('generated/src/frog/TextTrackCueList.dart');
#source('generated/src/frog/TimeRanges.dart');
#source('generated/src/frog/Touch.dart');
#source('generated/src/frog/TouchEvent.dart');
#source('generated/src/frog/TouchList.dart');
#source('generated/src/frog/TreeWalker.dart');
#source('generated/src/frog/UIEvent.dart');
#source('generated/src/frog/Uint16Array.dart');
#source('generated/src/frog/Uint32Array.dart');
#source('generated/src/frog/Uint8Array.dart');
#source('generated/src/frog/ValidityState.dart');
#source('generated/src/frog/VoidCallback.dart');
#source('generated/src/frog/WaveShaperNode.dart');
#source('generated/src/frog/WebGLActiveInfo.dart');
#source('generated/src/frog/WebGLBuffer.dart');
#source('generated/src/frog/WebGLContextAttributes.dart');
#source('generated/src/frog/WebGLContextEvent.dart');
#source('generated/src/frog/WebGLDebugRendererInfo.dart');
#source('generated/src/frog/WebGLDebugShaders.dart');
#source('generated/src/frog/WebGLFramebuffer.dart');
#source('generated/src/frog/WebGLProgram.dart');
#source('generated/src/frog/WebGLRenderbuffer.dart');
#source('generated/src/frog/WebGLRenderingContext.dart');
#source('generated/src/frog/WebGLShader.dart');
#source('generated/src/frog/WebGLTexture.dart');
#source('generated/src/frog/WebGLUniformLocation.dart');
#source('generated/src/frog/WebGLVertexArrayObjectOES.dart');
#source('generated/src/frog/WebKitAnimation.dart');
#source('generated/src/frog/WebKitAnimationEvent.dart');
#source('generated/src/frog/WebKitAnimationList.dart');
#source('generated/src/frog/WebKitBlobBuilder.dart');
#source('generated/src/frog/WebKitCSSFilterValue.dart');
#source('generated/src/frog/WebKitCSSKeyframeRule.dart');
#source('generated/src/frog/WebKitCSSKeyframesRule.dart');
#source('generated/src/frog/WebKitCSSMatrix.dart');
#source('generated/src/frog/WebKitCSSTransformValue.dart');
#source('generated/src/frog/WebKitFlags.dart');
#source('generated/src/frog/WebKitLoseContext.dart');
#source('generated/src/frog/WebKitMutationObserver.dart');
#source('generated/src/frog/WebKitPoint.dart');
#source('generated/src/frog/WebKitTransitionEvent.dart');
#source('generated/src/frog/WebSocket.dart');
#source('generated/src/frog/WheelEvent.dart');
#source('generated/src/frog/Worker.dart');
#source('generated/src/frog/WorkerContext.dart');
#source('generated/src/frog/WorkerLocation.dart');
#source('generated/src/frog/WorkerNavigator.dart');
#source('generated/src/frog/XMLHttpRequest.dart');
#source('generated/src/frog/XMLHttpRequestException.dart');
#source('generated/src/frog/XMLHttpRequestProgressEvent.dart');
#source('generated/src/frog/XMLHttpRequestUpload.dart');
#source('generated/src/frog/XMLSerializer.dart');
#source('generated/src/frog/XPathEvaluator.dart');
#source('generated/src/frog/XPathException.dart');
#source('generated/src/frog/XPathExpression.dart');
#source('generated/src/frog/XPathNSResolver.dart');
#source('generated/src/frog/XPathResult.dart');
#source('generated/src/frog/XSLTProcessor.dart');
#source('generated/src/interface/AudioBufferCallback.dart');
#source('generated/src/interface/DatabaseCallback.dart');
#source('generated/src/interface/EntriesCallback.dart');
#source('generated/src/interface/EntryCallback.dart');
#source('generated/src/interface/ErrorCallback.dart');
#source('generated/src/interface/FileCallback.dart');
#source('generated/src/interface/FileSystemCallback.dart');
#source('generated/src/interface/FileWriterCallback.dart');
#source('generated/src/interface/MetadataCallback.dart');
#source('generated/src/interface/NavigatorUserMediaErrorCallback.dart');
#source('generated/src/interface/PositionCallback.dart');
#source('generated/src/interface/PositionErrorCallback.dart');
#source('generated/src/interface/SQLStatementCallback.dart');
#source('generated/src/interface/SQLStatementErrorCallback.dart');
#source('generated/src/interface/SQLTransactionCallback.dart');
#source('generated/src/interface/SQLTransactionErrorCallback.dart');
#source('generated/src/interface/SQLTransactionSyncCallback.dart');
#source('generated/src/interface/StorageInfoErrorCallback.dart');
#source('generated/src/interface/StorageInfoQuotaCallback.dart');
#source('generated/src/interface/StorageInfoUsageCallback.dart');
#source('generated/src/interface/StringCallback.dart');

#source('src/EventListener.dart');
#source('src/RequestAnimationFrameCallback.dart');
#source('src/TimeoutHandler.dart');
#source('src/_Collections.dart');
// #source('src/_FactoryProviders.dart');
#source('src/_ListIterators.dart');
#source('src/_Lists.dart');

class Window extends DOMWindow {}
DOMWindow get window() native "return window;";
// TODO(vsm): Revert to Dart method when 508 is fixed.
HTMLDocument get document() native "return window.document;";
