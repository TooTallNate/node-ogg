
/**
 * Module dependencies.
 */

var debug = require('debug')('ogg:encoder');
var binding = require('./binding');
var OggStream = require('./stream');
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
 * Welds one or more OggStream instances into a single bitstream.
 */

function Encoder (opts) {
  if (!(this instanceof Encoder)) return new Encoder(opts);
  debug('creating new ogg "Encoder" instance');
  Readable.call(this, opts);
  this.streams = Object.create(null);

  // a queue of OggStreams that is the order in which pageout()/flush() should be
  // called in the _read() callback function
  this._queue = [];
}
inherits(Encoder, Readable);

/**
 * Creates a new OggStream instance and returns it for the user to begin
 * submitting `ogg_packet` instances to it.
 *
 * @param {Number} serialno The serial number of the stream, null/undefined means random.
 * @return {OggStream} the newly created OggStream instance. Call `.packetin()` on it.
 * @api public
 */

Encoder.prototype.stream = function (serialno) {
  debug('stream(%d)', serialno);
  var s = this.streams[serialno];
  if (!s) {
    s = new OggStream(serialno);
    s.on('needPageout', pageout(this));
    s.on('needFlush', flush(this));
    this.streams[s.serialno] = s;
  }
  return s;
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
      // notify the OggStream instance that we're done...
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
};

/**
 * Queues a "pageout()" to happen on the next _read() call.
 *
 * @api private
 */

Encoder.prototype._pageout = function (stream, done) {
  debug('_pageout(serialno %d)', stream.serialno);
  this._queue.push({
    stream: stream,
    fn: binding.ogg_stream_pageout,
    done: done
  });
  this.emit('needRead');
};

/**
 * Queues a "flush()" to happen on the next _read() call.
 *
 * @api private
 */

Encoder.prototype._flush = function (stream, done) {
  debug('_flush(serialno %d)', stream.serialno);
  this._queue.push({
    stream: stream,
    fn: binding.ogg_stream_flush,
    done: done
  });
  this.emit('needRead');
};

/**
 * Returns an event handler for the "needsPageout" event.
 *
 * @param {Encoder} encoder
 * @api private
 */

function pageout (encoder) {
  return function (done) {
    encoder._pageout(this, done);
  };
};

/**
 * Returns an event handler for the "needsFlush" event.
 *
 * @param {Encoder} encoder
 * @api private
 */

function flush (encoder) {
  return function (done) {
    encoder._flush(this, done);
  };
};
