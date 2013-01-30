
/**
 * Module dependencies.
 */

var binding = require('./binding');
var inherits = require('util').inherits;

/**
 * Module exports.
 */

module.exports = ogg_packet;

/**
 * Encapsulates an `ogg_packet` C struct instance. The `ogg_packet`
 * class is a node.js Buffer subclass.
 *
 * @api public
 */

function ogg_packet (buffer) {
  if (!Buffer.isBuffer(buffer)) {
    buffer = new Buffer(binding.sizeof_ogg_packet);
  }
  if (buffer.length != binding.sizeof_ogg_packet) {
    throw new Error('"buffer.length" = ' + buffer.length + ', expected ' + binding.sizeof_ogg_packet);
  }
  buffer.__proto__ = ogg_packet.prototype;
  return buffer;
}
inherits(ogg_packet, Buffer);

/**
 * packet->packet
 */

Object.defineProperty(ogg_packet.prototype, 'packet', {
  get: function () {
    return binding.ogg_packet_get_packet(this);
  },
  set: function (packet) {
    return binding.ogg_packet_set_packet(this, packet);
  },
  enumerable: true,
  configurable: true
});

/**
 * packet->bytes
 */

Object.defineProperty(ogg_packet.prototype, 'bytes', {
  get: function () {
    return binding.ogg_packet_bytes(this);
  },
  enumerable: true,
  configurable: true
});

/**
 * packet->e_o_s
 */

Object.defineProperty(ogg_packet.prototype, 'e_o_s', {
  get: function () {
    return binding.ogg_packet_e_o_s(this);
  },
  enumerable: true,
  configurable: true
});

/**
 * packet->b_o_s
 */

Object.defineProperty(ogg_packet.prototype, 'b_o_s', {
  get: function () {
    return binding.ogg_packet_b_o_s(this);
  },
  enumerable: true,
  configurable: true
});

/**
 * packet->granulepos
 */

Object.defineProperty(ogg_packet.prototype, 'granulepos', {
  get: function () {
    return binding.ogg_packet_granulepos(this);
  },
  enumerable: true,
  configurable: true
});

/**
 * packet->packetno
 */

Object.defineProperty(ogg_packet.prototype, 'packetno', {
  get: function () {
    return binding.ogg_packet_packetno(this);
  },
  enumerable: true,
  configurable: true
});

/**
 * Creates a new Buffer instance to back this `ogg_packet` instance.
 * Typically this function is used to take control over the bytes backing the
 * `ogg_packet` instance when the library that filled the packet reuses the
 * backing memory store for each `ogg_packet` instance.
 *
 * @api public
 */

ogg_packet.prototype.replace = function () {
  var buf = new Buffer(this.bytes);
  binding.ogg_packet_replace_buffer(this, buf);

  // keep a reference to "buf" so it doesn't get GC'd
  this._packet = buf;
};
