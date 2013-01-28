
/**
 * Module dependencies.
 */

var debug = require('debug')('ogg:encoder-stream');
var binding = require('./binding');
var inherits = require('util').inherits;
var Writable = require('stream').Writable;

// node v0.8.x compat
if (!Writable) Writable = require('readable-stream/writable');

/**
 * Module exports.
 */

module.exports = EncoderStream;

/**
 *
 */

function EncoderStream (serialno, encoder) {
  if (!(this instanceof EncoderStream)) return new EncoderStream(serialno);
  Writable.call(this, { objectMode: true, highWaterMark: 0 });

  if (null == serialno) {
    // TODO: better random serial number algo
    serialno = Math.random() * 1000000 | 0;
    debug('generated random serial number: %d', serialno);
  }
  this.serialno = serialno;
  this.encoder = encoder;
  this.os = new Buffer(binding.sizeof_ogg_stream_state);
  var r = binding.ogg_stream_init(this.os, serialno);
  if (0 !== r) {
    throw new Error('ogg_stream_init() failed: ' + r);
  }
}
inherits(EncoderStream, Writable);

/**
 * Writable stream _write() callback function.
 * Takes the given `ogg_packet` and calls `ogg_stream_packetin()` on it.
 * If a "flush" or "pageout" command was given, then that function will be called
 * in an attempt to output any possible `ogg_page` instances.
 * it into an `ogg_page` instance.
 *
 * @api private
 */

EncoderStream.prototype._write = function (packet, fn) {
  debug('_write()');
  var self = this;
  if (Buffer.isBuffer(packet)) {
    // assumed to be an `ogg_packet` Buffer instance
    this._packetin(packet, checkCommand);
  } else {
    checkCommand();
  }
  function checkCommand (err) {
    if (err) return fn(err);
    if (packet.flush) {
      self._flush(fn);
    } else if (packet.pageout) {
      self._pageout(fn);
    } else {
      // no command
      fn();
    }
  }
};

/**
 * Calls `ogg_stream_packetin()`.
 *
 * @api private
 */

EncoderStream.prototype._packetin = function (packet, fn) {
  debug('_packetin()');
  binding.ogg_stream_packetin(this.os, packet, function (rtn) {
    if (0 === rtn) {
      fn();
    } else {
      fn(new Error(rtn));
    }
  });
};

/**
 * Calls `ogg_stream_pageout()` until it returns 0.
 *
 * @api private
 */

var Struct = require('ref-struct');
var ogg_page = Struct({
  header: 'uchar*',
  header_len: 'long',
  body: 'uchar*',
  body_len: 'long'
});

EncoderStream.prototype._pageout = function (fn) {
  debug('_pageout()');
  var os = this.os;
  var og = new Buffer(binding.sizeof_ogg_page);
  var self = this;
  binding.ogg_stream_pageout(os, og, function (rtn, hlen, blen) {
    debug('ogg_stream_pageout() return = %d', rtn);
    if (0 === rtn) {
      fn();
    } else {
      self.emit('page', og, hlen, blen);
      self._pageout(fn);
    }
  });
};

/**
 * Calls `ogg_stream_flush()` until it returns 0.
 *
 * @api private
 */

EncoderStream.prototype._flush = function (fn) {
  debug('_flush()');
  var os = this.os;
  var og = new Buffer(binding.sizeof_ogg_page);
  var self = this;
  binding.ogg_stream_flush(os, og, function (rtn, hlen, blen) {
    debug('ogg_stream_flush() return = %d', rtn);
    if (0 === rtn) {
      fn();
    } else {
      self.emit('page', og, hlen, blen);
      self._flush(fn);
    }
  });
};
