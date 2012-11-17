
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
