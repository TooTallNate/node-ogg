
/**
 * Module dependencies.
 */

var debug = require('debug')('ogg:encoder');
var binding = require('./binding');
var EncoderStream = require('./encoder-stream');
var inherits = require('util').inherits;
var Readable = require('stream').Readable;

// node v0.8.x compat
if (!Readable) Readable = require('readable-stream');

/**
 * Module exports.
 */

module.exports = Encoder;

/**
 * The `Encoder` class.
 * Welds one or more `EncoderStream` instances into a single bitstream.
 */

function Encoder (opts) {
  if (!(this instanceof Encoder)) return new Encoder(opts);
  debug('creating new ogg "Encoder" instance');
  Readable.call(this, opts);

  // map of `EncoderStream` instances keyed by their serial number
  this.streams = Object.create(null);

  // a queue of `ogg_page` instances flattened into Buffer instnces. The _read()
  // function should deplete this queue, or wait til the "_page" event to read
  // more
  this._queue = [];

  // binded _onpage() call so that we can use it as an event
  // callback function on EncoderStream instances
  this._onpage = this._onpage.bind(this);
}
inherits(Encoder, Readable);

/**
 * Creates a new EncoderStream instance and returns it for the user to begin
 * submitting `ogg_packet` instances to it.
 *
 * @param {Number} serialno The serial number of the stream, null/undefined means random.
 * @return {EncoderStream} The newly created EncoderStream instance. Call `.packetin()` on it.
 * @api public
 */

Encoder.prototype.stream = function (serialno) {
  debug('stream(%d)', serialno);
  var s = this.streams[serialno];
  if (!s) {
    s = new EncoderStream(serialno);
    s.on('page', this._onpage);
    this.streams[s.serialno] = s;
  }
  return s;
};

/**
 * Called for each "page" event from every substream EncoderStream instance.
 * Flattens the given `ogg_page` buffer into a regular node.js Buffer.
 *
 * @api private
 */

Encoder.prototype._onpage = function (stream, page, header_len, body_len, e_o_s) {
  debug('_onpage()');

  if (e_o_s) {
    // stream is done...
    delete this.streams[stream.serialno];
  }

  // got a page!
  var data = new Buffer(header_len + body_len);
  binding.ogg_page_to_buffer(page, data);
  this._queue.push(data);
  this.emit('_page');
};

/**
 * Readable stream base class `_read()` callback function.
 * Processes the _queue array and attempts to read out any available
 * `ogg_page` instances, converted to raw Buffers.
 *
 * @param {Number} bytes
 * @param {Function} done
 * @api private
 */

Encoder.prototype._read = function (bytes, done) {
  debug('_read(%d bytes)', bytes);

  if (this._needsEnd) {
    done(null, null); // emit "end"
  } else if (this._queue.length) {
    output.call(this);
  } else {
    debug('need to wait for ogg_page Buffer');
    this.once('_page', output);
  }

  function output () {
    debug('flushing "_queue" (%d entries)', this._queue.length);
    var buf = Buffer.concat(this._queue);
    this._queue.splice(0); // empty queue

    // check if there's any more streams being processed
    var n = Object.keys(this.streams).length;
    if (n === 0) {
      this._needsEnd = true;
    }

    done(null, buf);
  }
};
