if (self.importScripts) {
  importScripts('../resources/fetch-test-helpers.js');
}

promise_test(function() {
    var response = new Response;
    return response.text()
      .then(function(text) {
          assert_equals(text, '',
                        'response.text() must return an empty string' +
                        'if body is null');
        });
  }, 'Behavior of Response with no constructor arguments.');

promise_test(function() {
    var response = new Response('test string');
    assert_equals(
      response.headers.get('Content-Type'),
      'text/plain;charset=UTF-8',
      'A Response constructed with a string should have a Content-Type.');
    return response.text()
      .then(function(text) {
          assert_equals(text, 'test string',
                        'Response body text should match the string on ' +
                        'construction.');
        });
  }, 'Behavior of Response with string content.');

promise_test(function() {
    var intView = new Int32Array([0, 1, 2, 3, 4, 55, 6, 7, 8, 9]);
    var buffer = intView.buffer;

    var response = new Response(buffer);
    assert_false(response.headers.has('Content-Type'),
                 'A Response constructed with ArrayBuffer should not have a ' +
                 'content type.');
    return response.arrayBuffer()
      .then(function(buffer) {
          var resultIntView = new Int32Array(buffer);
          assert_array_equals(
            resultIntView, [0, 1, 2, 3, 4, 55, 6, 7, 8, 9],
            'Response body ArrayBuffer should match ArrayBuffer ' +
            'it was constructed with.');
        });
  }, 'Behavior of Response with ArrayBuffer content.');

promise_test(function() {
    var intView = new Int32Array([0, 1, 2, 3, 4, 55, 6, 7, 8, 9]);

    var response = new Response(intView);
    assert_false(response.headers.has('Content-Type'),
                 'A Response constructed with ArrayBufferView ' +
                 'should not have a content type.');
    return response.arrayBuffer()
      .then(function(buffer) {
          var resultIntView = new Int32Array(buffer);
          assert_array_equals(
            resultIntView, [0, 1, 2, 3, 4, 55, 6, 7, 8, 9],
            'Response body ArrayBuffer should match ArrayBufferView ' +
            'it was constructed with.');
        });
  }, 'Behavior of Response with ArrayBufferView content without a slice.');

promise_test(function() {
    var intView = new Int32Array([0, 1, 2, 3, 4, 55, 6, 7, 8, 9]);
    var slice = intView.subarray(1, 4);  // Should be [1, 2, 3]
    var response = new Response(slice);
    assert_false(response.headers.has('Content-Type'),
                 'A Response constructed with ArrayBufferView ' +
                 'should not have a content type.');
    return response.arrayBuffer()
      .then(function(buffer) {
          var resultIntView = new Int32Array(buffer);
          assert_array_equals(
            resultIntView, [1, 2, 3],
            'Response body ArrayBuffer should match ArrayBufferView ' +
            'slice it was constructed with.');
        });
  }, 'Behavior of Response with ArrayBufferView content with a slice.');

promise_test(function() {
    var formData = new FormData();
    formData.append('sample string', '1234567890');
    formData.append('sample blob', new Blob(['blob content']));
    formData.append('sample file',
                    new File(['file content'], 'file.dat'));
    var response = new Response(formData);
    return response.text()
      .then(function(result) {
          var reg = new RegExp('multipart\/form-data; boundary=(.*)');
          var regResult = reg.exec(getContentType(response.headers));
          var boundary = regResult[1];
          var expected_body =
            '--' + boundary + '\r\n' +
            'Content-Disposition: form-data; name="sample string"\r\n' +
            '\r\n' +
            '1234567890\r\n' +
            '--' + boundary + '\r\n' +
            'Content-Disposition: form-data; name="sample blob"; ' +
            'filename="blob"\r\n' +
            'Content-Type: application/octet-stream\r\n' +
            '\r\n' +
            'blob content\r\n' +
            '--' + boundary + '\r\n' +
            'Content-Disposition: form-data; name="sample file"; ' +
            'filename="file.dat"\r\n' +
            'Content-Type: application/octet-stream\r\n' +
            '\r\n' +
            'file content\r\n' +
            '--' + boundary + '--\r\n';
          assert_equals(
            result, expected_body,
            'Creating a Response with FormData body must succeed.');
        });
  }, 'Behavior of Response with FormData content');

promise_test(function() {
    var headers = new Headers;
    headers.set('Content-Language', 'ja');
    var response = new Response(
      'test string', {method: 'GET', headers: headers});
    assert_false(response.bodyUsed);
    var response2 = response.clone();
    assert_false(response.bodyUsed, 'bodyUsed is not set by clone().');
    assert_false(response2.bodyUsed, 'bodyUsed is not set by clone().');
    response.headers.set('Content-Language', 'en');
    var response3;
    assert_equals(
      response2.headers.get('Content-Language'), 'ja', 'Headers of cloned ' +
      'response should not change when original response headers are changed.');

    var p = response.text();
    assert_true(response.bodyUsed, 'bodyUsed should be true when locked.');
    assert_false(response2.bodyUsed,
                 'Cloned bodies should not share bodyUsed.');
    assert_throws({name: 'TypeError'},
                  function() { response3 = response.clone(); },
                  'Response.clone() should throw if the body was used.');
    return p.then(function(text) {
        assert_false(response.bodyUsed,
                     'bodyUsed becomes false when text() is done.');
        assert_false(response2.bodyUsed);
        response3 = response.clone();
        return response2.text();
      }).then(function(text) {
        assert_equals(text, 'test string',
                      'Response clone response body text should match.');
        return Promise.all([response.text(), response2.text(),
                            response3.text()]);
      }).then(function(texts) {
        assert_equals(texts[0], '', 'Cloned after consumed.');
        assert_equals(texts[1], '', 'Already consumed.');
        assert_equals(texts[2], '', 'Cloned after consumed.');
      });
  }, 'Behavior of bodyUsed in Response and clone behavior.');

promise_test(function() {
    var response = new Response(null);
    assert_equals(
      response.headers.get('Content-Type'),
      'text/plain;charset=UTF-8',
      'A Response constructed with a value coerced to string should have a ' +
      'text Content-Type.');
    return response.text()
      .then(function(text) {
          assert_equals(text, 'null',
                        'A null value passed to Response constructor should ' +
                        'be coerced to the string "null".');
        });
  }, 'Behavior of Response passed null for body.');

promise_test(function() {
    var response = new Response();
    assert_equals(
      response.headers.get('Content-Type'),
      null,
      'A Response constructed with no body should have no Content-Type.');
    return response.text()
      .then(function(text) {
          assert_equals(text, '',
                        'Response with no body accessed as text should ' +
                        'resolve to the empty string.');
        });
  }, 'Behavior of Response with no body.');

done();
