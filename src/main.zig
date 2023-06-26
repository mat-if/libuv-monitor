const std = @import("std");

const c = @cImport({
    @cInclude("node_api.h");
    // @cInclude("uv.h");
});

// TODO: Can remove these and use the cInclude when https://github.com/ziglang/zig/issues/12325 is fixed
const uv_loop_s = extern struct {
    data: ?*anyopaque,
    active_handles: c_uint,
    handle_queue: [2]?*anyopaque,
    active_reqs: extern union {
        unused: ?*anyopaque,
        count: c_uint,
    },
};

extern fn uv_default_loop() ?*uv_loop_s;
// End temporary UV definitions

fn get_active_reqs(env: c.napi_env, _: c.napi_callback_info) callconv(.C) c.napi_value {
    var result: c.napi_value = undefined;

    const loop = uv_default_loop() orelse {
        _ = c.napi_throw_error(env, null, "Failed to fetch uv event loop");
        return null;
    };

    if (c.napi_create_uint32(env, loop.active_reqs.count, &result) != 0) {
        _ = c.napi_throw_error(env, null, "Failed to create return value");
        return null;
    }

    return result;
}

export fn napi_register_module_v1(env: c.napi_env, exports: c.napi_value) c.napi_value {
    var function: c.napi_value = undefined;

    if (c.napi_create_function(env, null, 0, get_active_reqs, null, &function) != 0)  {
        _ = c.napi_throw_error(env, null, "Failed to create function");
        return null;
    }

    if (c.napi_set_named_property(env, exports, "getActiveReqs", function) != 0) {
        _ = c.napi_throw_error(env, null, "Failed to add function to exports");
        return null;
    }

    return exports;
}

