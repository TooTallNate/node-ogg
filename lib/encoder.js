
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
 * Welds one or more EncoderStream instances into a single bitstream.
 */

function Encoder (opts) {
  if (!(this instanceof Encoder)) return new Encoder(opts);
  debug('creating new ogg "Encoder" instance');
  Readable.call(this, opts);
  this.streams = Object.create(null);

  // a queue of `ogg_page` instances flattened into Buffer instnces. The _read()
  // function should deplete this queue, or wait til the "_page" event to read
  // more
  this._queue = [];
}
inherits(Encoder, Readable);

/**
 * Creates a new EncoderStream instance and returns it for the user to begin
 * submitting `ogg_packet` instances to it.
 *
 * @param {Number} serialno The serial number of the stream, null/undefined means random.
 * @return {EncoderStream} the newly created EncoderStream instance. Call `.packetin()` on it.
 * @api public
 */

Encoder.prototype.stream = function (serialno) {
  debug('stream(%d)', serialno);
  var s = this.streams[serialno];
  if (!s) {
    s = new EncoderStream(serialno, this);
    var self = this;
    s.on('page', function (page, hlen, blen) {
      self._onpage(page, hlen, blen);
    });
    this.streams[s.serialno] = s;
  }
  return s;
};

Encoder.prototype._onpage = function (page, header_len, body_len) {
  debug('_onpage()');
  // got a page!
  var data = new Buffer(header_len + body_len);
  binding.ogg_page_to_buffer(page, data, onBuffer);
  var self = this;
  function onBuffer () {
    debug('onBuffer(%d bytes)', data.length);
    self._queue.push(data);
    self.emit('_page');
  }
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
  if (this._queue.length) {
    process();
  } else {
    debug('need to wait for ogg_page Buffer');
    this.once('_page', process);
  }
  function process () {
    var buf = Buffer.concat(this._queue);
    this._queue.splice(0); // empty queue
    done(null, buf);
  }
  /*
  // "bytes" is purely advisory... as long as we respond with *some* data in the
  // done() callback, then the world keeps on spinning...
  var self = this;
  var item;
  var page;
  var data;

  // processes the next "stream" in the _queue
  function process () {
    if (0 == self._queue.length) {
      // no more streams with ogg_packet's submitted that need to be processed.
      debug('no "stream"s to process, waiting for pageout()/flush()...');
      self.once('needRead', process);
      return;
    }
    item = self._queue[0];
    page = new Buffer(binding.sizeof_ogg_page);

    // call the pageout()/flush() binding
    item.fn(item.stream.os, page, after);
  }

  function after (r, header_len, body_len) {
    debug('after(%d, %d, %d)', r, header_len, body_len);
    if (0 === r) {
      // need more
      debug('need more...');
      // well we're done with this piece now...
      self._queue.shift();
      var item_done = item.done;
      if (item.stream.eos()) {
        debug('got stream "eos" (serialno %s)', item.stream.serialno);
        delete self.streams[item.stream.serialno];
        if (0 == Object.keys(self.streams).length) {
          debug('no more streams to process... sending EOF');
          done(null, null);
        } else {
          process();
        }
      } else {
        process();
      }
      // notify the EncoderStream instance that we're done...
      item_done();
    } else {
      // got a page!
      data = new Buffer(header_len + body_len);
      binding.ogg_page_to_buffer(page, data, onBuffer);
    }
  }

  function onBuffer () {
    debug('onBuffer(%d bytes)', data.length);
    done(null, data);
  }

  process();
  */
};
