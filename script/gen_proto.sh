#!/bin/sh
# Regenerate include/wlms_header.pb.h and src/wlms_header.pb.c from proto/wlms_header.proto.
# Needs the nanopb generator of the vendored runtime version: `uv tool install nanopb==0.4.9.1`
# (or `uv pip install nanopb==0.4.9.1 grpcio-tools` into a venv) so that `nanopb_generator` is on PATH.
set -e
cd "$(dirname "$0")/.."
nanopb_generator -I proto -D proto wlms_header.proto
mv proto/wlms_header.pb.h include/
mv proto/wlms_header.pb.c src/
echo "generated include/wlms_header.pb.h and src/wlms_header.pb.c"
