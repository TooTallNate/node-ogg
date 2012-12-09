
var assert = require('assert');
var OggStream = require('../').OggStream;

describe('OggStream', function () {

  it('should create an OggStream instance with the given serial number', function () {
    var serialno = 11111;
    var stream = new OggStream(serialno);
    assert(stream instanceof OggStream);
    assert.equal(serialno, stream.serialno);
  });

  it('should create an OggStream instance with a random serial number', function () {
    var stream = new OggStream();
    assert(stream instanceof OggStream);
    assert(isFinite(stream.serialno));
    assert.equal('number', typeof stream.serialno);
  });

});
