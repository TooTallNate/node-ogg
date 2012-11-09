
var ogg = require('./');
var decoder = new ogg.Decoder();
process.stdin.pipe(decoder);

/*
stream.on('page', function (page) {
  console.error('page', page);
});
*/

decoder.on('stream', function (stream) {
  console.error('stream', decoder);
  stream.on('packet', function (packet) {
    console.error('"packet" event, (serialno %d) (packetno %s)', stream.serialno, packet.packetno);
  });
});
