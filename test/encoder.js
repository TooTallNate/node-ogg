
var fs = require('fs');
var path = require('path');
var ogg = require('../');
var Encoder = ogg.Encoder;
var Packet = require('../lib/binding').ogg_packet_create;
var assert = require('assert');
var fixtures = path.resolve(__dirname, 'fixtures');

describe('Encoder', function () {

  it('should return an OggStream instance for .stream()', function () {
    var e = new Encoder();
    var s = e.stream();
    assert(s instanceof ogg.OggStream);
  });

  it('should emit an "end" event after the "e_o_s" packet', function (done) {
    var e = new Encoder();
    // flow...
    e.resume();

    e.on('end', done);
    var s = e.stream();
    s.packetin(Packet({
      data: Buffer('test'),
      packetno: 0,
      e_o_s: 1
    }), function (err) {
      if (err) return done(err);
      s.pageout(function (err) {
        if (err) return done(err);
        // wait for "end" event...
      });
    });
  });

  it('should emit an "end" event after *all* "e_o_s" packets', function (done) {
    var e = new Encoder();
    // flow...
    e.resume();

    e.on('end', done);
    [ 'foo', 'bar', 'baz' ].forEach(function (data) {
      var s = e.stream();
      s.packetin(Packet({
        data: Buffer(data),
        packetno: 0,
        e_o_s: 1
      }), function (err) {
        if (err) return done(err);
        s.pageout(function (err) {
          if (err) return done(err);
          // wait for "end" event...
        });
      });
    });
  });

});
