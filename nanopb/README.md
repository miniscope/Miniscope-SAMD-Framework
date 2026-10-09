# nanopb runtime (vendored)

Encoder half of [nanopb](https://github.com/nanopb/nanopb) 0.4.9.1 (zlib license, see LICENSE.txt):
`pb.h`, `pb_common.[ch]`, `pb_encode.[ch]`. The device only encodes, so `pb_decode.*` is left out.

The generated `wlms_header.pb.[ch]` in `include/` and `src/` must come from the same nanopb version;
regenerate them with `script/gen_proto.sh` after editing `proto/wlms_header.proto`.
