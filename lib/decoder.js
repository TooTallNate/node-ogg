
/**
 * Module dependencies.
 */

var debug = require('debug')('ogg:decoder');
var binding = require('./binding');
var asyncEmit = require('./asyncEmit');
var inherits = require('util').inherits;
var Writable = require('stream').Writable;
var OggStream = require('./stream');

// node v0.8.x compat
if (!Writable) Writable = require('readable-stream/writable');

/**
 * Module exports.
 */

module.exports = Decoder;

/**
 * The ogg `Decoder` class. Write an OGG file stream to it, and it'll emit
 * "stream" events for each embedded stream. The OggStream instances emit
 * "packet" events with the raw `ogg_packet` instance to send to a ogg stream
 * decoder (like Vorbis, Theora, etc.).
 *
 * @param {Object} opts Writable stream options
 * @api public
 */

function Decoder (opts) {
  if (!(this instanceof Decoder)) return new Decoder(opts);
  Writable.call(this, opts);

  this.oy = new Buffer(binding.sizeof_ogg_sync_state);
  var r = binding.ogg_sync_init(this.oy);
  if (0 !== r) {
    throw new Error('ogg_sync_init() failed: ' + r);
  }
}
inherits(Decoder, Writable);

/**
 * Writable stream base class `_write()` callback function.
 *
 * @param {Buffer} chunk
 * @param {Function} done
 * @api private
 */

Decoder.prototype._write = function (chunk, done) {
  debug('_write(%d bytes)', chunk.length);

  // allocate space for 1 `ogg_page` and 1 `ogg_packet`
  // XXX: we could do this at the per-decoder level, since only 1 ogg_page is
  // active (being processed by an ogg decoder) at a time
  var stream;
  var self = this;
  var packets = 0;
  var oy = this.oy;
  var page = new Buffer(binding.sizeof_ogg_page);
  var packet = new Buffer(binding.sizeof_ogg_packet);

  binding.ogg_sync_write(oy, chunk, chunk.length, afterWrite);
  function afterWrite (rtn) {
    debug('after _write(%d)', rtn);
    if (0 === rtn) {
      pageout();
    } else {
      done(new Error('ogg_sync_write() error: ' + rtn));
    }
  }

  function pageout (err) {
    debug('pageout(%j)', err);
    if (err) {
      done(err);
    } else {
      binding.ogg_sync_pageout(oy, page, afterPageout);
    }
  }

  function afterPageout (rtn, serialno, packets) {
    debug('afterPageout(%d, %d, %d)', rtn, serialno, packets);
    if (1 === rtn) {
      // got a page, now write it to the appropriate `ogg_stream`
      page.serialno = serialno;
      page.packets = packets;
      self.emit('page', page);
      stream = self._stream(serialno);
      pagein();
    } else if (0 === rtn) {
      // need more data
      done();
    } else {
      // something bad...
      done(new Error('ogg_sync_pageout() error: ' + rtn));
    }
  }

  // at this point, "page" has a valid `ogg_page`,
  // and "stream.os" is an `ogg_stream_state`
  function pagein () {
    debug('pagein()');
    binding.ogg_stream_pagein(stream.os, page, afterPagein);
  }

  function afterPagein (rtn) {
    debug('afterPagein(%d)', rtn);
    if (0 === rtn) {
      packets = page.packets;
      packetout();
    } else {
      done(new Error('ogg_stream_pagein() error: ' + rtn));
    }
  }

  function packetout () {
    debug('packetout(), %d packets left', packets);
    if (0 === packets) {
      // read out the next page from the Decoder
      pageout();
    } else {
      packet.packetno = null;
      binding.ogg_stream_packetout(stream.os, packet, afterPacketout);
    }
  }

  function afterPacketout (rtn) {
    debug('afterPacketout(%d)', rtn);
    if (1 === rtn) {
      // got a packet
      var info = binding.ogg_packet_info(packet);
      for (var i in info) {
        packet[i] = info[i];
      }
      if (packet.b_o_s) {
        stream.emit('bos');
      }
      asyncEmit(stream, 'packet', [ packet ], afterPacketEmit);
    } else {
      done(new Error('ogg_stream_packetout() error: ' + rtn));
    }
  }

  function afterPacketEmit (err) {
    debug('afterPacketEmit(%j)', err);
    if (err) return done(err);
    if (packet.e_o_s) {
      stream.emit('eos');
    }
    --packets;
    // read out the next packet from the stream
    packetout();
  }
};

/**
 * Gets an ogg Stream instance for the given "serialno".
 * Creates one if necessary, and then emits a "stream" event.
 *
 * @param {Number} serialno The serial number of the ogg_stream.
 * @return {OggStream} an ogg_stream for the given serial number.
 * @api private
 */

Decoder.prototype._stream = function (serialno) {
  debug('_stream(%d)', serialno);
  var stream = this[serialno];
  if (!stream) {
    stream = new OggStream(serialno);
    this[serialno] = stream;
    this.emit('stream', stream);
  }
  return stream;
};
