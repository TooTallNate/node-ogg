
var fs = require('fs');
var path = require('path');
var assert = require('assert');
var Decoder = require('../').Decoder;
var fixtures = path.resolve(__dirname, 'fixtures');

describe('Decoder', function () {

  describe('"320x240.ogv" fixture file', function () {

    it('should get 2 stream events', function (done) {
      var decoder = new Decoder();
      var input = fs.createReadStream(path.resolve(fixtures, '320x240.ogv'));
      var count = 0;
      var expected = 2;
      decoder.on('stream', function () {
        count++;
      });
      decoder.on('finish', function () {
        assert.equal(expected, count);
        done();
      });
      input.pipe(decoder);
    });

  });

});
