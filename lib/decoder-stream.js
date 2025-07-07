
/**
 * Module dependencies.
 */

var debug = require('debug')('ogg:decoder-stream');
var binding = require('./binding');
var ogg_packet = require('./packet');
var inherits = require('util').inherits;
var Readable = require('stream').Readable;

/**
 * Module exports.
 */

module.exports = DecoderStream;

/**
 * The `DecoderStream` class is what gets passed in for the `Decoder` class'
 * "stream" event. You should not need to create instances of `DecoderStream`
 * manually.
 *
 * @api private
 */

function DecoderStream (serialno) {
  if (!(this instanceof DecoderStream)) return new DecoderStream(serialno);
  Readable.call(this, { objectMode: true, highWaterMark: 0 });

  // array of `ogg_packet` instances to output for the _read() function
  this.packets = [];

  this.serialno = serialno;

  this.os = new Buffer(binding.sizeof_ogg_stream_state);
  var r = binding.ogg_stream_init(this.os, serialno);
  if (0 !== r) {
    throw new Error('ogg_stream_init() failed: ' + r);
  }
}
inherits(DecoderStream, Readable);

/**
 * We have to overwrite the "on()" function to reinterpret "packet" event names as
 * "data" event names. Attaching a "packet" event listener will put the stream
 * into streams2 "old-mode".
 *
 * @api public
 */

DecoderStream.prototype.on = function (ev, fn) {
  if ('packet' == ev) {
    debug('on(): remapping "packet" event listener as "data" event listener');
    ev = 'data';
  }
  return Readable.prototype.on.call(this, ev, fn);
};
DecoderStream.prototype.addListener = DecoderStream.prototype.on;

DecoderStream.prototype.once = function (ev, fn) {
  if ('packet' == ev) {
    debug('once(): remapping "packet" event listener as "data" event listener');
    ev = 'data';
  }
  return Readable.prototype.once.call(this, ev, fn);
};

DecoderStream.prototype.removeListener = function (ev, fn) {
  if ('packet' == ev) {
    debug('removeListener(): remapping "packet" event listener as "data" event listener');
    ev = 'data';
  }
  return Readable.prototype.removeListener.call(this, ev, fn);
};

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
  var packet;

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
      packet = new ogg_packet();
      binding.ogg_stream_packetout(os, packet, afterPacketout);
    }
  }

  function afterPacketout (rtn, bytes, b_o_s, e_o_s, granulepos, packetno) {
    debug('afterPacketout(%d, %d, %d, %d, %d, %d)', rtn, bytes, b_o_s, e_o_s, granulepos, packetno);
    if (1 === rtn) {
      // got a packet...

      // since libogg takes control of the `packet`s "packet" data field, we must
      // copy it over to a Node.js buffer and change the pointer over. That way,
      // the `packet` Buffer is *completely* managed by the JS garbage collector
      packet.replace();

      if (b_o_s) {
        self.emit('bos');
      }
      packet._callback = afterPacketRead;
      self.packets.push(packet);
      self.emit('_packet');
    } else if (-1 === rtn) {
      // libogg issued a sync warning, usually recoverable, try it again.
      // http://xiph.org/ogg/doc/libogg/ogg_stream_packetout.html
      packetout();
    } else { // libogg returned an unrecoverable error
      fn(new Error('ogg_stream_packetout() error: ' + rtn));
    }
  }

  function afterPacketRead (err) {
    debug('afterPacketRead(%s)', err);
    if (err) return fn(err);
    if (packet.e_o_s) {
      self.emit('eos');
      self.push(null); // emit "end"
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

    if (this.push) this.push(packet);
    else fn(null, packet); // XXX: old Readable API, remove at some point

    if (callback) process.nextTick(callback);
  }
  if (this.packets.length > 0) {
    onpacket.call(this);
  } else {
    this.once('_packet', onpacket);
  }
};
