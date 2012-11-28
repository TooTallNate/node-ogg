
/**
 * Quick little example that simply keeps track of the
 * number of "packet" events received per "stream". I use
 * to help in debugging sometimes.
 *
 * Pipe an OGG file to stdin.
 */

var ogg = require('../');
var stats = {};

var decoder = new ogg.Decoder();
decoder.on('stream', function (stream) {
  if (null == stats[stream.serialno]) stats[stream.serialno] = 0;
  stream.on('packet', function () {
    stats[stream.serialno]++;
  });
});
decoder.on('finish', function () {
  console.log('number of packets:');
  console.log(stats);
});

process.stdin.pipe(decoder);
