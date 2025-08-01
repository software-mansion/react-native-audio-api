# Opusfile

[![GitLab Pipeline Status](https://gitlab.xiph.org/xiph/opusfile/badges/master/pipeline.svg)](https://gitlab.xiph.org/xiph/opusfile/commits/master)
[![GitHub CI](https://github.com/xiph/opusfile/actions/workflows/build.yml/badge.svg)](https://github.com/xiph/opusfile/actions/workflows/build.yml)

The opusfile and opusurl libraries provide a high-level API for
decoding and seeking within .opus files on disk or over http(s).

opusfile depends on libopus and libogg.
opusurl depends on opusfile and openssl.

The library is functional, but there are likely issues
we didn't find in our own testing. Please give feedback
in #opus on irc.libera.chat or at opus@xiph.org.

Programming documentation is available in tree and online at
https://opus-codec.org/docs/
