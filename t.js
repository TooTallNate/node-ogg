
var ogg = require('./');
var decoder = new ogg.Decoder();
process.stdin.pipe(decoder);

process.stdin.on('data', function (b) {
  console.error('  STDIN "data" event (%d bytes)', b.length);
});

decoder.on('page', function (page) {
  console.error('"page" event (serialno %d) (num packets %d)', page.serialno, page.packets);
});

decoder.on('stream', function (stream) {
  console.error('"stream" event (serialno %d)', stream.serialno);
  stream.on('packet', function (packet, fn) {
    console.error('"packet" event, (serialno %d) (packetno %s) (bytes %d)', stream.serialno, packet.packetno, packet.bytes);
    setTimeout(fn, 500);
  });
});
