node-ogg
========
### NodeJS native binding to libogg

This module provides a Writable stream interface for decoding `ogg` files, and a
Readable stream for encoding `ogg`files. `libogg` only provides the interfaces for
multiplexing the various streams embedding into an ogg file (and vice versa),
therefore this module must be used in conjunction with a `node-ogg` compatible
stream module, like `node-vorbis`, `node-theora`, or `node-flac`.


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

Here's an example of using the `Decoder` class with a dummy OGG `StreamDecoder`
that simply `console.log()`s information about each "page" emitted from each OGG
stream:

``` javascript
var ogg = require('ogg');
var file = __dirname + '/Hydrate-Kenny_Beltrey.ogg';

var decoder = new ogg.Decoder();
decoder.on('stream', function (stream) {
  console.log(stream.serialno);
  stream.on('page', function (page, done) {
    console.log(page.bytes);
    done();
  });
});
```

See the `examples` directory for some more example code.

API
---

### Decoder class

The `Decoder` class is a `Writable` stream that accepts an OGG file written to
it, and emits "stream" events when a new stream is encountered, which you _must_
listen for and attach an OGG `StreamDecoder` instance to, like `node-vorbis` for
a stream containing Vorbis audio data.

### Encoder class

The `Encoder` class is a `Readable` stream where you attach OGG `StreamEncoder`
instances to the stream, and it outputs a valid OGG file.
