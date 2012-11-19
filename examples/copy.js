
/**
 * This example accepts an ogg filename as its argument and
 * creates a page-by-page copy of the ogg stream. It's not a byte-for-byte
 * copy of the file, though I'm not yet sure why that is...
 */

var fs = require('fs');
var ogg = require('../');
var path = require('path');
var file = process.argv[2];

if (!file) {
  console.error('error: must specify an OGG file!');
  process.exit(1);
}

var out = path.resolve(path.dirname(file), 'copy of ' + path.basename(file));
var encoder = new ogg.Encoder();
var decoder = new ogg.Decoder();

// for each "stream" event in the decoded file, we need to call
// .stream() on the encoder to create a matching output stream
decoder.on('stream', function (stream) {

  // we match the serial numbers, though we could also get random ones instead
  var outStream = encoder.stream(stream.serialno);

  // for each "page" event, force the output stream to flush a page of its own.
  // the first time this is emitted there won't have been any "packets" queued
  // yet, but it's nothing to worry about...
  stream.on('page', function (page, done) {
    outStream.flush(done);
  });

  // for each "packet" event, send the packet to the output stream untouched
  stream.on('packet', function (packet, done) {
    outStream.packetin(packet, done);
  });

  // at the end of each stream, force one last page flush, for any remaining
  // packets in the stream. this ensures the "end" event gets fired properly.
  stream.on('eos', function () {
    outStream.flush(function (err) {
      if (err) throw err;
    });
  });
});

// the input file gets piped to the Decoder instance
fs.createReadStream(file).pipe(decoder);

// the OGG file output from the Encoder instance gets piped to the output file
encoder.pipe(fs.createWriteStream(out));
encoder.on('end', function () {
  console.error('created copy of %j as %j', file, out);
});
