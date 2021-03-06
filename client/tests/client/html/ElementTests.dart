// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.


expectLargeRect(ClientRect rect) {
  Expect.equals(rect.top, 0);
  Expect.equals(rect.left, 0);
  Expect.isTrue(rect.width > 100);
  Expect.isTrue(rect.height > 100);
  Expect.equals(rect.bottom, rect.top + rect.height);
  Expect.equals(rect.right, rect.left + rect.width);
}

void testEventHelper(EventListenerList listenerList, String type,
    [Function registerOnEventListener = null]) {
  bool firedWhenAddedToListenerList = false;
  bool firedOnEvent = false;
  listenerList.add((e) {
    firedWhenAddedToListenerList = true;
  });
  if (registerOnEventListener != null) {
    registerOnEventListener((e) {
      firedOnEvent = true;
    });
  }
  final event = new Event(type);
  listenerList.dispatch(event);

  Expect.isTrue(firedWhenAddedToListenerList);
  if (registerOnEventListener != null) {
    Expect.isTrue(firedOnEvent);
  }
}

void testConstructorHelper(String tag, String htmlSnippet,
    String expectedText, Function isExpectedClass) {
  Expect.isTrue(isExpectedClass(new Element.tag(tag)));
  final elementFromSnippet = new Element.html(htmlSnippet);
  Expect.isTrue(isExpectedClass(elementFromSnippet));
  Expect.equals(elementFromSnippet.text, expectedText);
}

void testElement() { 
  asyncTest('computedStyle', 1, () {
    final element = document.body;
    element.computedStyle.then((style) {
      Expect.equals(style.getPropertyValue('left'), 'auto');
      callbackDone();
    });
  });

  asyncTest('rect', 1, () {
    final element = document.body;
    element.rect.then((rect) {
      expectLargeRect(rect.client);
      expectLargeRect(rect.offset);
      expectLargeRect(rect.scroll);
      Expect.equals(rect.bounding.left, 8);
      Expect.equals(rect.bounding.top, 8);
      Expect.isTrue(rect.clientRects.length > 0);
      callbackDone();
    });
  });

  group('constructors', () {
    test('a', () => testConstructorHelper('a', '<a>foo</a>', 'foo',
        (element) => element is AnchorElement));
    test('area', () => testConstructorHelper('area', '<area>foo</area>', '',
        (element) => element is AreaElement));
    // TODO(jacobr): audio tags cause tests to segfault when using dartium.
    // b/5522106.
    // test('audio', () => testConstructorHelper('audio',
    //     '<audio>foo</audio>', 'foo',
    //     (element) => element is AudioElement));
    test('br', () => testConstructorHelper('br', '<br>', '',
        (element) => element is BRElement));
    test('base', () => testConstructorHelper('base', '<base>foo</base>', '',
        (element) => element is BaseElement));          
    test('blockquote', () => testConstructorHelper('blockquote',
        '<blockquote>foo</blockquote>', 'foo',
        (element) => element is QuoteElement));
    test('body', () => testConstructorHelper('body',
        '<body><div>foo</div></body>', 'foo',
        (element) => element is BodyElement));
    test('button', () => testConstructorHelper('button',
        '<button>foo</button>', 'foo',
        (element) => element is ButtonElement));
    test('canvas', () => testConstructorHelper('canvas',
        '<canvas>foo</canvas>', 'foo',
        (element) => element is CanvasElement));
    test('dl', () => testConstructorHelper('dl', '<dl>foo</dl>', 'foo',
        (element) => element is DListElement));
    // TODO(jacobr): WebKit doesn't yet support the DataList class.
    // test('datalist', () => testConstructorHelper('datalist',
    //     '<datalist>foo</datalist>', 'foo',
    //     (element) => element is DataListElement));
    test('details', () => testConstructorHelper('details',
        '<details>foo</details>', 'foo',
        (element) => element is DetailsElement));
    test('div', () => testConstructorHelper('div', '<div>foo</div>', 'foo',
        (element) => element is DivElement));
    test('embed', () => testConstructorHelper('embed',
        '<embed>foo</embed>', '',
        (element) => element is EmbedElement));
    test('fieldset', () => testConstructorHelper('fieldset',
        '<fieldset>foo</fieldset>', 'foo',
        (element) => element is FieldSetElement));
    test('font', () => testConstructorHelper('font', '<font>foo</font>', 'foo',
        (element) => element is FontElement));
    test('form', () => testConstructorHelper('form', '<form>foo</form>', 'foo',
        (element) => element is FormElement));
    test('hr', () => testConstructorHelper('hr', '<hr>', '',
        (element) => element is HRElement));
    test('head', () => testConstructorHelper('head',
        '<head><script>foo;</script></head>', 'foo;',
        (element) => element is HeadElement));
    test('h1', () => testConstructorHelper('h1', '<h1>foo</h1>', 'foo',
        (element) => element is HeadingElement));
    test('h2', () => testConstructorHelper('h2', '<h2>foo</h2>', 'foo',
        (element) => element is HeadingElement));
    test('h3', () => testConstructorHelper('h3', '<h3>foo</h3>', 'foo',
        (element) => element is HeadingElement));
    test('h4', () => testConstructorHelper('h4', '<h4>foo</h4>', 'foo',
        (element) => element is HeadingElement));
    test('h5', () => testConstructorHelper('h5', '<h5>foo</h5>', 'foo',
        (element) => element is HeadingElement));
    test('h6', () => testConstructorHelper('h6', '<h6>foo</h6>', 'foo',
        (element) => element is HeadingElement));
    test('iframe', () => testConstructorHelper('iframe',
        '<iframe>foo</iframe>', 'foo',
        (element) => element is IFrameElement));
    test('img', () => testConstructorHelper('img', '<img>', '',
        (element) => element is ImageElement));
    test('input', () => testConstructorHelper('input', '<input/>', '',
        (element) => element is InputElement));
    test('keygen', () => testConstructorHelper('keygen', '<keygen>', '',
        (element) => element is KeygenElement));
    test('li', () => testConstructorHelper('li', '<li>foo</li>', 'foo',
        (element) => element is LIElement));
    test('label', () => testConstructorHelper('label',
        '<label>foo</label>', 'foo',
        (element) => element is LabelElement));
    test('legend', () => testConstructorHelper('legend',
        '<legend>foo</legend>', 'foo',
        (element) => element is LegendElement));
    test('link', () => testConstructorHelper('link', '<link>', '',
        (element) => element is LinkElement));
    test('map', () => testConstructorHelper('map', '<map>foo</map>', 'foo',
        (element) => element is MapElement));
    test('marquee', () => testConstructorHelper('marquee',
        '<marquee>foo</marquee>', 'foo',
        (element) => element is MarqueeElement));
    test('menu', () => testConstructorHelper('menu', '<menu>foo</menu>', 'foo',
        (element) => element is MenuElement));
    test('meta', () => testConstructorHelper('meta', '<meta>', '',
        (element) => element is MetaElement));
    test('meter', () => testConstructorHelper('meter',
        '<meter>foo</meter>', 'foo',
        (element) => element is MeterElement));
    test('del', () => testConstructorHelper('del', '<del>foo</del>', 'foo',
        (element) => element is ModElement));
    test('ins', () => testConstructorHelper('ins', '<ins>foo</ins>', 'foo',
        (element) => element is ModElement));
    test('ol', () => testConstructorHelper('ol', '<ol>foo</ol>', 'foo',
        (element) => element is OListElement));
    test('object', () => testConstructorHelper('object',
        '<object>foo</object>', 'foo',
        (element) => element is ObjectElement));
    test('optgroup', () => testConstructorHelper('optgroup',
        '<optgroup>foo</optgroup>', 'foo',
        (element) => element is OptGroupElement));
    test('option', () => testConstructorHelper('option',
        '<option>foo</option>', 'foo',
        (element) => element is OptionElement));
    test('output', () => testConstructorHelper('output',
        '<output>foo</output>', 'foo',
        (element) => element is OutputElement));
    test('p', () => testConstructorHelper('p', '<p>foo</p>', 'foo',
        (element) => element is ParagraphElement));
    test('param', () => testConstructorHelper('param', '<param>', '',
        (element) => element is ParamElement));
    test('pre', () => testConstructorHelper('pre', '<pre>foo</pre>', 'foo',
        (element) => element is PreElement));
    test('progress', () => testConstructorHelper('progress',
        '<progress>foo</progress>', 'foo',
        (element) => element is ProgressElement));
    test('q', () => testConstructorHelper('q', '<q>foo</q>', 'foo',
        (element) => element is QuoteElement));
    test('script', () => testConstructorHelper('script',
        '<script>foo</script>', 'foo',
        (element) => element is ScriptElement));
    test('select', () => testConstructorHelper('select',
        '<select>foo</select>', 'foo',
        (element) => element is SelectElement));
    // TODO(jacobr): audio tags cause tests to segfault when using dartium.
    // b/5522106.
    // test('source', () => testConstructorHelper('source', '<source>', '',
    //                      (element) => element is SourceElement));
    test('span', () => testConstructorHelper('span', '<span>foo</span>', 'foo',
        (element) => element is SpanElement));
    test('style', () => testConstructorHelper('style',
        '<style>foo</style>', 'foo',
        (element) => element is StyleElement));
    test('caption', () => testConstructorHelper('caption',
        '<caption>foo</caption>', 'foo',
        (element) => element is TableCaptionElement));
    test('td', () => testConstructorHelper('td', '<td>foo</td>', 'foo',
        (element) => element is TableCellElement));
    test('colgroup', () => testConstructorHelper('colgroup',
        '<colgroup></colgroup>', '',
        (element) => element is TableColElement));
    test('col', () => testConstructorHelper('col', '<col></col>', '',
        (element) => element is TableColElement));
    test('table', () => testConstructorHelper('table',
        '<table><caption>foo</caption></table>', 'foo',
        (element) => element is TableElement));
    test('tr', () => testConstructorHelper('tr',
        '<tr><td>foo</td></tr>', 'foo',
        (element) => element is TableRowElement));
    test('tbody', () => testConstructorHelper('tbody',
        '<tbody><tr><td>foo</td></tr></tbody>', 'foo',
        (element) => element is TableSectionElement));
    test('tfoot', () => testConstructorHelper('tfoot',
        '<tfoot><tr><td>foo</td></tr></tfoot>', 'foo',
        (element) => element is TableSectionElement));
    test('thead', () => testConstructorHelper('thead',
        '<thead><tr><td>foo</td></tr></thead>', 'foo',
        (element) => element is TableSectionElement));
    test('textarea', () => testConstructorHelper('textarea',
        '<textarea>foo</textarea>', 'foo',
        (element) => element is TextAreaElement));
    test('title', () => testConstructorHelper('title',
        '<title>foo</title>', 'foo',
        (element) => element is TitleElement));
    // TODO(jacobr): audio tags cause tests to segfault when using dartium.
    // b/5522106.
    // test('track', () => testConstructorHelper('track', '<track>', '',
    //                      (element) => element is TrackElement));
    test('ul', () => testConstructorHelper('ul', '<ul>foo</ul>', 'foo',
        (element) => element is UListElement));
    // TODO(jacobr): video tags cause tests to segfault when using dartium.
    // b/5522106.
    // test('video', () => testConstructorHelper('video',
    //     '<video>foo</video>', 'foo',
    //     (element) => element is VideoElement));
    // TODO(jacobr): this test is broken until Dartium fixes b/5521083
    // test('someunknown', () => testConstructorHelper('someunknown',
    //     '<someunknown>foo</someunknown>', 'foo',
    //     (element) => element is UnknownElement));
  });

  test('eventListeners', () {
    final element = new Element.tag('div');
    final on = element.on;
    final rawElement = unwrapDomObject(element);

    testEventHelper(on.abort, 'abort',
        (listener) => rawElement.addEventListener('abort', listener, true));
    testEventHelper(on.beforeCopy, 'beforecopy',
        (listener) => rawElement.addEventListener('beforecopy', listener, true));
    testEventHelper(on.beforeCut, 'beforecut',
        (listener) => rawElement.addEventListener('beforecut', listener, true));
    testEventHelper(on.beforePaste, 'beforepaste',
        (listener) => rawElement.addEventListener('beforepaste', listener, true));
    testEventHelper(on.blur, 'blur',
        (listener) => rawElement.addEventListener('blur', listener, true));
    testEventHelper(on.change, 'change',
        (listener) => rawElement.addEventListener('change', listener, true));
    testEventHelper(on.click, 'click',
        (listener) => rawElement.addEventListener('click', listener, true));
    testEventHelper(on.contextMenu, 'contextmenu',
        (listener) => rawElement.addEventListener('contextmenu', listener, true));
    testEventHelper(on.copy, 'copy',
        (listener) => rawElement.addEventListener('copy', listener, true));
    testEventHelper(on.cut, 'cut',
        (listener) => rawElement.addEventListener('cut', listener, true));
    testEventHelper(on.dblClick, 'dblclick',
        (listener) => rawElement.addEventListener('dblclick', listener, true));
    testEventHelper(on.drag, 'drag',
        (listener) => rawElement.addEventListener('drag', listener, true));
    testEventHelper(on.dragEnd, 'dragend',
        (listener) => rawElement.addEventListener('dragend', listener, true));
    testEventHelper(on.dragEnter, 'dragenter',
        (listener) => rawElement.addEventListener('dragenter', listener, true));
    testEventHelper(on.dragLeave, 'dragleave',
        (listener) => rawElement.addEventListener('dragleave', listener, true));
    testEventHelper(on.dragOver, 'dragover',
        (listener) => rawElement.addEventListener('dragover', listener, true));
    testEventHelper(on.dragStart, 'dragstart',
        (listener) => rawElement.addEventListener('dragstart', listener, true));
    testEventHelper(on.drop, 'drop',
        (listener) => rawElement.addEventListener('drop', listener, true));
    testEventHelper(on.error, 'error',
        (listener) => rawElement.addEventListener('error', listener, true));
    testEventHelper(on.focus, 'focus',
        (listener) => rawElement.addEventListener('focus', listener, true));
    testEventHelper(on.input, 'input',
        (listener) => rawElement.addEventListener('input', listener, true));
    testEventHelper(on.invalid, 'invalid',
        (listener) => rawElement.addEventListener('invalid', listener, true));
    testEventHelper(on.keyDown, 'keydown',
        (listener) => rawElement.addEventListener('keydown', listener, true));
    testEventHelper(on.keyPress, 'keypress',
        (listener) => rawElement.addEventListener('keypress', listener, true));
    testEventHelper(on.keyUp, 'keyup',
        (listener) => rawElement.addEventListener('keyup', listener, true));
    testEventHelper(on.load, 'load',
        (listener) => rawElement.addEventListener('load', listener, true));
    testEventHelper(on.mouseDown, 'mousedown',
        (listener) => rawElement.addEventListener('mousedown', listener, true));
    testEventHelper(on.mouseMove, 'mousemove',
        (listener) => rawElement.addEventListener('mousemove', listener, true));
    testEventHelper(on.mouseOut, 'mouseout',
        (listener) => rawElement.addEventListener('mouseout', listener, true));
    testEventHelper(on.mouseOver, 'mouseover',
        (listener) => rawElement.addEventListener('mouseover', listener, true));
    testEventHelper(on.mouseUp, 'mouseup',
        (listener) => rawElement.addEventListener('mouseup', listener, true));
    testEventHelper(on.mouseWheel, 'mousewheel',
        (listener) => rawElement.addEventListener('mousewheel', listener, true));
    testEventHelper(on.paste, 'paste',
        (listener) => rawElement.addEventListener('paste', listener, true));
    testEventHelper(on.reset, 'reset',
        (listener) => rawElement.addEventListener('reset', listener, true));
    testEventHelper(on.scroll, 'scroll',
        (listener) => rawElement.addEventListener('scroll', listener, true));
    testEventHelper(on.search, 'search',
        (listener) => rawElement.addEventListener('search', listener, true));
    testEventHelper(on.select, 'select',
        (listener) => rawElement.addEventListener('select', listener, true));
    testEventHelper(on.selectStart, 'selectstart',
        (listener) => rawElement.addEventListener('selectstart', listener, true));
    testEventHelper(on.submit, 'submit',
        (listener) => rawElement.addEventListener('submit', listener, true));
    testEventHelper(on.touchCancel, 'touchcancel',
        (listener) => rawElement.addEventListener('touchcancel', listener, true));
    testEventHelper(on.touchEnd, 'touchend',
        (listener) => rawElement.addEventListener('touchend', listener, true));
    testEventHelper(on.touchLeave, 'touchleave');
    testEventHelper(on.touchMove, 'touchmove',
        (listener) => rawElement.addEventListener('touchmove', listener, true));
    testEventHelper(on.touchStart, 'touchstart',
        (listener) => rawElement.addEventListener('touchstart', listener, true));
    testEventHelper(on.transitionEnd, 'webkitTransitionEnd');
    testEventHelper(on.fullscreenChange, 'webkitfullscreenchange',
        (listener) => rawElement.addEventListener('webkitfullscreenchange', listener, true));
  });

  test('attributes', () {
    final element = new Element.html(
        '''<div class="foo" style="overflow: hidden" data-foo="bar"
               data-foo2="bar2" dir="rtl">
           </div>''');
    final attributes = element.attributes;
    Expect.equals(attributes['class'], 'foo');
    Expect.equals(attributes['style'], 'overflow: hidden');
    Expect.equals(attributes['data-foo'], 'bar');
    Expect.equals(attributes['data-foo2'], 'bar2');
    Expect.equals(attributes.length, 5);
    Expect.equals(element.dataAttributes.length, 2);
    element.dataAttributes['foo'] = 'baz';
    Expect.equals(element.dataAttributes['foo'], 'baz');
    Expect.equals(attributes['data-foo'], 'baz');
    attributes['data-foo2'] = 'baz2';
    Expect.equals(attributes['data-foo2'], 'baz2');
    Expect.equals(element.dataAttributes['foo2'], 'baz2');
    Expect.equals(attributes['dir'], 'rtl');

    // TODO(jacobr): determine why these tests are causing segfaults in dartium
    // element.dataAttributes.remove('foo2');
    // Expect.equals(attributes.length, 4);
    // Expect.equals(dataAttributes.length, 1);
    // attributes.remove('style');
    // Expect.equals(attributes.length, 3);
    // element.dataAttributes['foo3'] = 'baz3';
    // Expect.equals(dataAttributes.length, 2);
    // Expect.equals(attributes.length, 4);
    // attributes['style'] = 'width: 300px;';
    // Expect.equals(attributes.length, 5);*/
  });
}
