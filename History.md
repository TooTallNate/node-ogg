1.2.5 / 2016-07-07
==================

* [[`1590ebaab0`](https://github.com/TooTallNate/ogg/commit/1590ebaab0)] - Upgrade to nan v2.3.5 (lammas)
* [[`76abd9c7db`](https://github.com/TooTallNate/ogg/commit/76abd9c7db)] - **travis**: don't test node v3 (Nathan Rajlich)
* [[`57b77af6e5`](https://github.com/TooTallNate/ogg/commit/57b77af6e5)] - **README**: update Node.js name (Nathan Rajlich)

1.2.4 / 2016-01-20
==================

  * fix node v0.8

1.2.3 / 2016-01-20
==================

  * use Local instead of Handle
  * travis: test node v5

1.2.2 / 2015-12-12
==================

  * travis: test io.js and node v4
  * package: add "license" field
  * package: update Node.js text
  * package: update "nan" to v2.1.0
  * package: update "debug" to v2

1.2.1 / 2015-05-24
==================

  * binding: add "-undefined dynamic_lookup" flag on OS X
  * Readme: add node-opus to encoders/decoders list
  * config: add linux arm (raspberry pi) config

1.2.0 / 2015-05-07
==================

  * appveyor: don't test node v0.8 or v0.11
  * travis: don't test node v0.11
  * add node 0.12 to the CI files
  * add io.js/node 0.12 support via "nan" (#7, @Xenoveritas)

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
