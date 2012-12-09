
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
 * Calls `ogg_stream_eos()` on this OggStream to check whether or not the
 * stream has received the packet with the "e_o_s" flag set.
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
      // `ogg_page` has been submitted, now emit an async "page" event
      asyncEmit(self, 'page', [ page ], afterPageEmit);
    } else {
      fn(new Error('ogg_stream_pagein() error: ' + r));
    }
  }

  function afterPageEmit (err) {
    if (err) return fn(err);
    // now read out the packets and emit async "packet" events on this stream
    packetout();
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
 * Submits a raw packet to the streaming layer, so that it
 * can be formed into a page.
 *
 * @param {Buffer|ogg_packet} packet `ogg_packet` instance
 * @param {Function} fn callback function
 * @api public
 */

OggStream.prototype.packetin = function (packet, fn) {
  debug('packetin()');

  // support passing in an `ogg_packet` struct instance directly
  if (packet.buffer) packet = packet.buffer;

  if (this._writingPacket) {
    debug('need to wait for "packetin" event before writing');
    this.once('packetin', this.packetin.bind(this, packet, fn));
    return;
  }

  var os = this.os;
  var self = this;
  this._writingPacket = true;

  binding.ogg_stream_packetin(os, packet, afterPacketin);

  function afterPacketin (r) {
    self._writingPacket = false;
    if (0 === r) {
      fn();
    } else {
      fn(new Error('ogg_stream_packetin() error: ' + r));
    }
  }
};

/**
 * Flags that this stream needs `ogg_stream_flush()` called on it before another
 * `ogg_packet` is allowed to be submitted.
 *
 * @param {Function} fn callback function to invoke when page has been flushed
 * @api public
 */

OggStream.prototype.flush = function (fn) {
  debug('flush()');
  // TODO: wait for packetin queue to be drained first
  asyncEmit(this, 'needFlush', fn);
};

/**
 * Calls `ogg_stream_pageout()` on this OggStream.
 * Outputs a completed page if the stream contains enough packets to form a full
 * page. Emits an *asynchronous* "needPageout" event, which the Encoder uses to convert
 * the page to a Buffer instance and output in the readable stream.
 *
 * @param {Function} fn callback function to invoke when page has been flushed
 * @api public
 */

OggStream.prototype.pageout = function (fn) {
  debug('pageout()');
  // TODO: wait for packetin queue to be drained first
  asyncEmit(this, 'needPageout', fn);
};
