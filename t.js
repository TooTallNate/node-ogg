
var ogg = require('./');
var decoder = new ogg.Decoder();
process.stdin.pipe(decoder);

decoder.on('stream', function (stream) {
  console.error('got "stream"', stream);

});
