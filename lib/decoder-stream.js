
/**
 * Module dependencies.
 */

var debug = require('debug')('ogg:decoder-stream');
var binding = require('./binding');
var inherits = require('util').inherits;
var Readable = require('stream').Readable;

// node v0.8.x compat
if (!Readable) Readable = require('readable-stream');

/**
 * Module exports.
 */

module.exports = DecoderStream;

/**
 *
 */

function DecoderStream (serialno) {
  if (!(this instanceof DecoderStream)) return new DecoderStream(serialno);
  Readable.call(this, { objectMode: true });

  this.serialno = serialno;
  this.os = new Buffer(binding.sizeof_ogg_stream_state);
  this.packets = [];

  // every time "packets.push()" is called, emit a "_packet" event so that the
  // _read() function know about it...
  var push = this.packets.push;
  this.packets.push = function () {
    push.apply(this.packets, arguments);
    this.emit('_packet');
  }.bind(this);

  var r = binding.ogg_stream_init(this.os, serialno);
  if (0 !== r) {
    throw new Error('ogg_stream_init() failed: ' + r);
  }
}
inherits(DecoderStream, Readable);

/**
 * Calls `ogg_stream_pagein()` on this OggStream.
 * Internal function used by the `Decoder` class.
 *
 * @param {Buffer} page `ogg_page` instance
 * @param {Number} packets the number of `ogg_packet` instances in the page
 * @param {Function} fn callback function
 * @api private
 */

DecoderStream.prototype.pagein = function (page, packets, fn) {
  debug('pagein(%d packets)', packets);

  var os = this.os;
  var self = this;
  var packet = new Buffer(binding.sizeof_ogg_packet);
  //process.nextTick(fn);

  binding.ogg_stream_pagein(os, page, afterPagein);
  function afterPagein (r) {
    if (0 === r) {
      // `ogg_page` has been submitted, now emit a "page" event
      self.emit('page', page);

      // now read out the packets and push them onto this Readable stream
      packetout();
    } else {
      fn(new Error('ogg_stream_pagein() error: ' + r));
    }
  }

  function packetout () {
    debug('packetout(), %d packets left', packets);
    if (0 === packets) {
      // no more packets to read out, we're done...
      fn();
    } else {
      packet.bytes = null;
      packet.b_o_s = null;
      packet.e_o_s = null;
      packet.granulepos = null;
      packet.packetno = null;
      binding.ogg_stream_packetout(os, packet, afterPacketout);
    }
  }

  function afterPacketout (rtn, bytes, b_o_s, e_o_s, granulepos, packetno) {
    debug('afterPacketout(%d, %d, %d, %d, %d, %d)', rtn, bytes, b_o_s, e_o_s, granulepos, packetno);
    if (1 === rtn) {
      // got a packet
      packet.bytes = bytes;
      packet.b_o_s = b_o_s;
      packet.e_o_s = e_o_s;
      packet.granulepos = granulepos;
      packet.packetno = packetno;
      if (b_o_s) {
        self.emit('bos');
      }
      packet._callback = afterPacketRead;
      self.packets.push(packet);
    } else {
      fn(new Error('ogg_stream_packetout() error: ' + rtn));
    }
  }

  function afterPacketRead (err) {
    debug('afterPacketRead(%s)', err);
    if (err) return fn(err);
    if (packet.e_o_s) {
      self.emit('eos');
    }
    --packets;
    // read out the next packet from the stream
    packetout();
  }
};

/**
 * Pushes the next "packet" from the "packets" array, otherwise waits for an
 * "_packet" event.
 *
 * @api private
 */

DecoderStream.prototype._read = function (n, fn) {
  debug('_read(%d packets)', n);
  function onpacket () {
    var packet = this.packets.shift();
    var callback = packet._callback;
    packet._callback = null;
    fn(null, packet);
    if (callback) callback();
  }
  if (this.packets.length > 0) {
    onpacket.call(this);
  } else {
    this.once('_packet', onpacket);
  }
};
