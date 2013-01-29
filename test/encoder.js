
var fs = require('fs');
var ogg = require('../');
var path = require('path');
var Encoder = ogg.Encoder;
var assert = require('assert');
var ogg_packet = require('ogg-packet');
var fixtures = path.resolve(__dirname, 'fixtures');

describe('Encoder', function () {

  it('should return an EncoderStream instance for .stream()', function () {
    var e = new Encoder();
    var s = e.stream();
    assert.equal('function', typeof s.write);
    assert.equal('function', typeof s.packetin);
    assert.equal('function', typeof s.pageout);
    assert.equal('function', typeof s.flush);
  });

  describe('with one .stream()', function () {

    it('should emit an "end" event after one "e_o_s" packet', function (done) {
      var e = new Encoder();
      // flow...
      e.resume();

      e.on('end', done);
      var s = e.stream();

      // create `ogg_packet`
      var data = new Buffer('test');
      var packet = new ogg_packet();
      packet.packet = data;
      packet.bytes = data.length;
      packet.b_o_s = 1;
      packet.e_o_s = 1;
      packet.granulepos = 0;
      packet.packetno = 0;

      s.packetin(packet, function (err) {
        if (err) return done(err);
        s.pageout(function (err) {
          if (err) return done(err);
          // wait for "end" event...
        });
      });
    });

    it('should support passing in an `ogg_struct` instance directly', function (done) {
      var e = new Encoder();
      // flow...
      e.resume();

      e.on('end', done);
      var s = e.stream();

      // create an `ogg_packet` struct
      var data = new Buffer('test');
      var packet = new ogg_packet();
      packet.packet = data;
      packet.bytes = data.length;
      packet.b_o_s = 1;
      packet.e_o_s = 1;
      packet.granulepos = 0;
      packet.packetno = 0;

      // pass in the `ogg_packet` instance directly
      s.packetin(packet, function (err) {
        if (err) return done(err);
        s.pageout(function (err) {
          if (err) return done(err);
          // wait for "end" event...
        });
      });
    });

  });

  describe('with three .stream()s', function () {

    it('should emit an "end" event after three "e_o_s" packets', function (done) {
      var e = new Encoder();
      // flow...
      e.resume();

      e.on('end', done);
      [ 'foo', 'bar', 'baz' ].forEach(function (str) {
        var s = e.stream();

        // create `ogg_packet`
        var data = new Buffer(str);
        var packet = new ogg_packet();
        packet.packet = data;
        packet.bytes = data.length;
        packet.b_o_s = 1;
        packet.e_o_s = 1;
        packet.granulepos = 0;
        packet.packetno = 0;

        s.packetin(packet, function (err) {
          if (err) return done(err);
          s.pageout(function (err) {
            if (err) return done(err);
            // wait for "end" event...
          });
        });
      });
    });

  });

});
