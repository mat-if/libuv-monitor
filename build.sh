set -x

NAPI_HEADER_DIR=`realpath "$(dirname $(which node))/../include/node"`

zig cc --print-effective-triple
zig cc -O3 -undefined dynamic_lookup -shared -isystem $NAPI_HEADER_DIR -o uvc.node src/main.c
