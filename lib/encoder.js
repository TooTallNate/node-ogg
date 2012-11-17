
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
    s.on('packetin', packetin(this));
    this.streams[s.serialno] = s;
  }
  return s;
};

/**
 * Readable stream base class `_read()` callback function.
 *
 * @param {Number} bytes
 * @param {Function} done
 * @api private
 */

Encoder.prototype._read = function (bytes, done) {
  debug('_read(%d bytes)', bytes);
};

Encoder.prototype._packetin = function (stream, packet, done) {
  debug('_packetin()');
};

/**
 *
 */

function packetin (encoder) {
  return function (packet, done) {
    encoder._packetin(this, packet, done);
  };
};
