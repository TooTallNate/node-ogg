node-ogg
========
### NodeJS native binding to libogg
[![Build Status](https://travis-ci.org/TooTallNate/node-ogg.png?branch=master)](https://travis-ci.org/TooTallNate/node-ogg)

This module provides a Writable stream interface for decoding `ogg` files, and a
Readable stream for encoding `ogg` files. `libogg` only provides the interfaces
for multiplexing the various streams embedding into an ogg file (and vice versa),
therefore this module is intended to be used in conjunction with a
`node-ogg`-compatible stream module, like `node-vorbis` and `node-theora`.


Installation
------------

`node-ogg` comes bundled with its own copy of `libogg`, so
there's no need to have the library pre-installed on your system.

Simply compile and install `node-ogg` using `npm`:

``` bash
$ npm install ogg
```

__NOTE:__ `node-ogg` requires to be built using `node-gyp` v0.8.0 or newer!


Example
-------

Here's an example of using the `Decoder` class and simply listening for the raw
events and `console.log()`s information about each "packet" emitted from each ogg
stream:

``` javascript
var fs = require('fs');
var ogg = require('ogg');
var file = __dirname + '/Hydrate-Kenny_Beltrey.ogg';

var decoder = new ogg.Decoder();

decoder.on('stream', function (stream) {
  console.log('new "stream":', stream.serialno);

  // emitted for each `ogg_packet` instance in the stream.
  stream.on('data', function (packet) {
    console.log('got "packet":', packet.packetno);
  });

  // emitted after the last packet of the stream
  stream.on('end', function () {
    console.log('got "end":', stream.serialno);
  });
});

// pipe the ogg file to the Decoder
fs.createReadStream(file).pipe(decoder);
```

See the `examples` directory for some more example code.


API
---

### Decoder class

The `Decoder` class is a `Writable` stream that accepts an ogg file written to
it, and emits "stream" events when a new stream is encountered. The
`DecoderStream` instance is a readable stream that outputs `ogg_packet` Buffer
instances.encountered, which
you are then expected to pass along to a ogg stream decoder.

### Encoder class

The `Encoder` class is a `Readable` stream where you are given `EncoderStream`
instances and are required to write `ogg_packet`s received from an ogg stream
encoder to them in order to create a valid ogg file.


OGG Stream Decoders/Encoders
----------------------------

Here's a list of known ogg stream decoders and encoders that are compatible with / depend on `node-ogg`.
Please send pull requests for additional modules if you write one.

| **Module**                       | **Decoder?** | **Encoder?**
|:--------------------------------:|:------------:|:------------:
|   [`node-vorbis`][node-vorbis]   |      ✓       |      ✓

[node-vorbis]: https://github.com/TooTallNate/node-vorbis
