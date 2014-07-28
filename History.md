
1.1.2 / 2014-07-28
==================

 * add appveyor.yml file for Windows testing
 * README: use svg for Travis badge
 * package: update all the dependency versions
 * package: bump "readable-stream" to v1
 * travis: don't test node v0.9, test node v0.10, v0.11
 * fixed to works on 0.8.x (#5, @arunoda)

1.1.1 / 2013-03-07
==================

 * Fix stupid global `done` variable leak

1.1.0 / 2013-03-07
==================

 * update for node v0.9.12 streams2 API changes
 * encoder: add `Encoder#use(stream)` function
 * decoder-stream: add once() and removeListener() support for "packet" event

1.0.0 / 2013-02-16
==================

 * `streams2` refactor
 * Encoder implemented
 * Decoder finished being implemented

0.0.1 / 2012-11-15
==================

 * Initial release
 * Only a partially implemented decoder so far...
