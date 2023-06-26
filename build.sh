set -x

NAPI_HEADER_DIR=`realpath "$(dirname $(which node))/../include/node"`

if [[ -n "$1" ]]
then
    TARGET_FLAG="-target $1"
else
    TARGET_FLAG=""
fi

# zig cc -print-supported-cpus
# zig cc --print-effective-triple
# zig cc -### -O3 -undefined dynamic_lookup -shared -isystem $NAPI_HEADER_DIR $TARGET_FLAG -o uvc.node src/main.c
# cc -O1 -undefined dynamic_lookup -shared -isystem $NAPI_HEADER_DIR $TARGET_FLAG -o uvc.node src/main.c
cc -O1 -fPIC -shared -isystem $NAPI_HEADER_DIR $TARGET_FLAG -o uvc.node src/main.c

zig build-lib -fallow-shlib-undefined -dynamic -isystem $NAPI_HEADER_DIR -femit-bin=uvz.node -O ReleaseSmall src/main.zig

