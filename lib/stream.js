
/**
 * Module dependencies.
 */

var binding = require('./binding');
var asyncEmit = require('./asyncEmit');
var inherits = require('util').inherits;
var debug = require('debug')('ogg:stream');
var EventEmitter = require('events').EventEmitter;

/**
 * Module exports.
 */

module.exports = OggStream;

/**
 * The `OggStream` class isn't a regular node stream, because it doesn't operate
 * on raw data. Instead it operates on `ogg_packet` and/or `ogg_page` instances,
 * and therefore is an EventEmitter subclass.
 *
 * @param {Number} serialno The integer serial number of the ogg_stream. If
 *                 null/undefined then a random serial number is assigned.
 * @api public
 */

function OggStream (serialno) {
  if (!(this instanceof OggStream)) return new OggStream(serialno);
  if (null == serialno) {
    // TODO: better random serial number algo
    serialno = Math.random() * 1000000 | 0;
  }
  debug('creating new OggStream instance (serialno %d)', serialno);
  EventEmitter.call(this);

  this.serialno = serialno;
  this.os = new Buffer(binding.sizeof_ogg_stream_state);

  var r = binding.ogg_stream_init(this.os, serialno);
  if (0 !== r) {
    throw new Error('ogg_stream_init() failed: ' + r);
  }
}
inherits(OggStream, EventEmitter);

/**
 * Calls `ogg_stream_eos()` on this OggStream to query whether or no the stream
 * has received the packet with the "e_o_s" flag set.
 *
 * @return {Boolean} true if the stream has ended, false otherwise
 * @api public
 */

OggStream.prototype.eos = function () {
  return binding.ogg_stream_eos(this.os) == 1;
};

/****************************************
 *              Decoding                *
 ****************************************/

/**
 * Calls `ogg_stream_pagein()` on this OggStream.
 * Internal function used by the `Decoder` class.
 *
 * @param {Buffer} page `ogg_page` instance
 * @param {Function} fn callback function
 * @api private
 */

OggStream.prototype.pagein  = function (page, fn) {
  debug('pagein()');

  var os = this.os;
  var self = this;
  var packets = page.packets;
  var packet = new Buffer(binding.sizeof_ogg_packet);

  binding.ogg_stream_pagein(os, page, afterPagein);
  function afterPagein (r) {
    if (0 === r) {
      // `ogg_page` has been submitted, now read out the
      // packets and emit "packet" event on this stream
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
      asyncEmit(self, 'packet', [ packet ], afterPacketEmit);
    } else {
      fn(new Error('ogg_stream_packetout() error: ' + rtn));
    }
  }

  function afterPacketEmit (err) {
    debug('afterPacketEmit(%j)', err);
    if (err) return fn(err);
    if (packet.e_o_s) {
      self.emit('eos');
    }
    --packets;
    // read out the next packet from the stream
    packetout();
  }
};

/****************************************
 *              Encoding                *
 ****************************************/

/**
 * Calls `ogg_stream_packetin()` on this OggStream.
 * This is the main function called when encoding an ogg file.
 *
 * @param {Buffer} packet `ogg_packet` instance
 * @param {Function} fn callback function
 * @api public
 */

OggStream.prototype.packetin = function (packet, fn) {

};
