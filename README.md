node-ogg
========
### NodeJS native binding to libogg

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

  // emitted upon the first packet of the stream
  stream.on('bof', function () {
    console.log('got "bof":', stream.serialno);
  });

  // emitted for each `ogg_packet` instance in the stream.
  // note that this is an *asynchronous* event!
  stream.on('packet', function (packet, done) {
    console.log('got "packet":', packet.packetno);
    done();
  });

  // emitted after the last packet of the stream
  stream.on('eof', function () {
    console.log('got "eof":', stream.serialno);
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
it, and emits "stream" events when a new stream is encountered. The `OggStream`
isntance received emits "packet" events for each `ogg_packet` encountered, which
you are then expected to pass along to a ogg stream decoder.

### Encoder class

The `Encoder` class is a `Readable` stream where you are given `OggStream`
instances and are required to write `ogg_packet`s received from an ogg stream
encoder to them in order to create a valid ogg file.
