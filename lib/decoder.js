
/**
 * Module dependencies.
 */

var debug = require('debug')('ogg:decoder');
var binding = require('./binding');
var inherits = require('util').inherits;
var Writable = require('stream').Writable;
var DecoderStream = require('./decoder-stream');

// node v0.8.x compat
if (!Writable) Writable = require('readable-stream/writable');

/**
 * Module exports.
 */

module.exports = Decoder;

/**
 * The ogg `Decoder` class. Write an OGG file stream to it, and it'll emit
 * "stream" events for each embedded stream. The DecoderStream instances emit
 * "packet" events with the raw `ogg_packet` instance to send to an ogg stream
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

  // allocate space for 1 `ogg_page`
  // XXX: we could do this at the per-decoder level, since only 1 ogg_page is
  // active (being processed by an ogg decoder) at a time
  var stream;
  var self = this;
  var oy = this.oy;
  var page = new Buffer(binding.sizeof_ogg_page);

  binding.ogg_sync_write(oy, chunk, chunk.length, afterWrite);
  function afterWrite (rtn) {
    debug('after _write(%d)', rtn);
    if (0 === rtn) {
      pageout();
    } else {
      done(new Error('ogg_sync_write() error: ' + rtn));
    }
  }

  function pageout () {
    debug('pageout()');
    page.serialno = null;
    page.packets = null;
    binding.ogg_sync_pageout(oy, page, afterPageout);
  }

  function afterPageout (rtn, serialno, packets) {
    debug('afterPageout(%d, %d, %d)', rtn, serialno, packets);
    if (1 === rtn) {
      // got a page, now write it to the appropriate DecoderStream
      page.serialno = serialno;
      page.packets = packets;
      self.emit('page', page);
      stream = self._stream(serialno);
      stream.pagein(page, packets, afterPagein);
    } else if (0 === rtn) {
      // need more data
      done();
    } else {
      // something bad...
      done(new Error('ogg_sync_pageout() error: ' + rtn));
    }
  }

  function afterPagein (err) {
    debug('afterPagein(%s)', err);
    if (err) return done(err);
    // attempt to read out the next page from the `ogg_sync_state`
    pageout();
  }
};

/**
 * Gets an DecoderStream instance for the given "serialno".
 * Creates one if necessary, and then emits a "stream" event.
 *
 * @param {Number} serialno The serial number of the ogg_stream.
 * @return {DecoderStream} an DecoderStream for the given serial number.
 * @api private
 */

Decoder.prototype._stream = function (serialno) {
  debug('_stream(%d)', serialno);
  var stream = this[serialno];
  if (!stream) {
    stream = new DecoderStream(serialno);
    this[serialno] = stream;
    this.emit('stream', stream);
  }
  return stream;
};
