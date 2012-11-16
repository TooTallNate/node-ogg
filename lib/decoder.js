
/**
 * Module dependencies.
 */

var debug = require('debug')('ogg:decoder');
var binding = require('bindings')('ogg');
var inherits = require('util').inherits;
var Writable = require('stream').Writable;
var EventEmitter = require('events').EventEmitter;
var asyncEmit = require('./asyncEmit');

// node v0.8.x compat
if (!Writable) Writable = require('readable-stream/writable');

/**
 * Module exports.
 */

module.exports = Decoder;

/**
 * The ogg Decoder class. Write an OGG file stream to it, and it'll emit "stream"
 * events that themselves emit "page" events.
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
 * 
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
      asyncEmit(stream, 'packet', [ packet ], afterPacket);
    } else {
      done(new Error('ogg_stream_packetout() error: ' + rtn));
    }
  }

  function afterPacket (err) {
    debug('afterPacket(%j)', err);
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
 * Gets an OGG stream EventEmitter for the given "serialno".
 * Creates one if necessary, and then emits a "stream" event.
 */

Decoder.prototype._stream = function (serialno) {
  debug('_stream(%d)', serialno);
  var stream = this[serialno];
  if (!stream) {
    debug('creating EventEmitter for OGG stream "%d"', serialno);
    // XXX: move to its own constructor?
    stream = new EventEmitter();
    stream.os = new Buffer(binding.sizeof_ogg_stream_state);
    var r = binding.ogg_stream_init(stream.os, serialno);
    if (0 !== r) {
      throw new Error('ogg_stream_init() error: ' + r);
    }
    stream.serialno = serialno;
    this[serialno] = stream;
    this.emit('stream', stream);
  }
  return stream;
};
