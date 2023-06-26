#include <node_api.h>
#include <stdio.h>
#include <uv.h>

#define NAPI_METHOD(NAME, FUNC) \
        { NAME, 0, FUNC, 0, 0, 0, napi_default, 0 }

// TODO: These probably don't need to be macros
#define DEFINE_NAPI_METHOD(NAME, FUNC) \
    napi_property_descriptor d_##FUNC = NAPI_METHOD(NAME, FUNC); \
    if (napi_define_properties(env, exports, 1, &d_##FUNC) != napi_ok) { \
        napi_throw_error(env, 0, "Failed to create function"); \
    }

#define OBJECT_PROPERTY(OBJECT, TYPE, NAME, VALUE) \
    napi_value js_##VALUE; \
    if (napi_create_##TYPE(env, VALUE, &js_##VALUE) != napi_ok) { \
        napi_throw_error(env, 0, "Failed to create #VALUE value"); \
        return 0; \
    } \
    if (napi_set_named_property(env, OBJECT, NAME, js_##VALUE) != napi_ok) { \
        napi_throw_error(env, 0, "Failed to add #NAME property"); \
        return 0; \
    }

uv_check_t check_handle;

uint32_t min = UINT32_MAX;
uint32_t max = 0;
uint64_t sum = 0;
uint32_t count = 0;
uint64_t last_check = 0;
uint64_t starting_idle_time = 0;
// TODO: Add since_last metrics also
uint32_t since_last_min = UINT32_MAX;
uint32_t since_last_max = 0;
uint64_t since_last_sum = 0;
uint32_t since_last_count = 0;
uint64_t since_last_info_call = 0;
uint64_t since_last_idle_time = 0;

void reset_since_last() {
    since_last_min = UINT32_MAX;
    since_last_max = 0;
    since_last_sum = 0;
    since_last_count = 0;
}

void check_callback(uv_check_t *handle) {
    uint32_t loop_elapsed;
    uint64_t now = uv_hrtime();

    // Handle first run
    if (last_check == 0) {
        last_check = now;
        starting_idle_time = uv_metrics_idle_time(uv_default_loop());
        return;
    }

    // TODO: This is in nanoseconds
    loop_elapsed = (now - last_check);

    if (loop_elapsed < min) {
        min = loop_elapsed;
    }

    if (loop_elapsed > max) {
        max = loop_elapsed;
    }

    sum += loop_elapsed;

    count += 1;

    last_check = now;

    // TODO: Since last hackery
    if (loop_elapsed < since_last_min) {
        since_last_min = loop_elapsed;
    }

    if (loop_elapsed > since_last_max) {
        since_last_max = loop_elapsed;
    }

    since_last_sum += loop_elapsed;

    since_last_count += 1;
}

void sum_active_handles(uv_handle_t *handle, void *total_count) {
    if (uv_is_active(handle)) {
        // printf("\t%p: type: %d..%s\n", handle, handle->type, uv_handle_type_name(handle->type));
        *(uint32_t*)total_count += 1;
    }
}

static napi_value init_uv_check(napi_env env, napi_callback_info info) {
    uv_check_init(uv_default_loop(), &check_handle);

    if (uv_check_start(&check_handle, &check_callback) != 0) {
        napi_throw_error(env, 0, "Failed to start uv check callback");
        return 0;
    }

    return 0;
}

static napi_value stop_uv_check(napi_env env, napi_callback_info info) {
    uv_check_stop(&check_handle);

    return 0;
}

static napi_value create_uv_info_object(napi_env env, napi_callback_info info, double idle_percent, uint32_t active_handles, uint32_t active_reqs, double since_last_idle_percent) {
    napi_value object;

    if (napi_create_object(env, &object) != napi_ok) {
        napi_throw_error(env, 0, "Failed to create object");
        return 0;
    }

    OBJECT_PROPERTY(object, uint32, "lifetimeMin", min);
    OBJECT_PROPERTY(object, uint32, "lifetimeMax", max);
    OBJECT_PROPERTY(object, uint32, "lifetimeSum", sum);
    OBJECT_PROPERTY(object, uint32, "lifetimeCount", count);
    OBJECT_PROPERTY(object, double, "lifetimeIdlePercent", idle_percent);
    OBJECT_PROPERTY(object, uint32, "sinceLastMin", since_last_min);
    OBJECT_PROPERTY(object, uint32, "sinceLastMax", since_last_max);
    OBJECT_PROPERTY(object, uint32, "sinceLastSum", since_last_sum);
    OBJECT_PROPERTY(object, uint32, "sinceLastCount", since_last_count);
    OBJECT_PROPERTY(object, double, "sinceLastIdlePercent", since_last_idle_percent);
    OBJECT_PROPERTY(object, uint32, "activeHandles", active_handles);
    OBJECT_PROPERTY(object, uint32, "activeReqs", active_reqs);

    return object;
}

static napi_value get_uv_check_info(napi_env env, napi_callback_info info) {
    uv_loop_t* event_loop = uv_default_loop();

    uint64_t idle_time = uv_metrics_idle_time(event_loop);

    // TODO: Should this divided by the actual diff in hrtime from start to
    // now, instead of the sum? Is sum completely accurate?
    double idle_percent = 100.0;
    if (sum > 0) {
        idle_percent = ((idle_time - starting_idle_time) * 100.0) / sum;
    }

    // TODO: What is the difference between this and event_loop->active_handles?
    uint32_t active_handles = 0;
    uv_walk(event_loop, &sum_active_handles, &active_handles);

    // TODO: since last hackery
    uint64_t since_last_i = idle_time - since_last_idle_time;
    since_last_idle_time = idle_time;
    double since_last_idle_percent = 100.0;
    if (since_last_sum > 0) {
        since_last_idle_percent = (since_last_i * 100.0) / since_last_sum;
    }

    napi_value info_object = create_uv_info_object(env, info, idle_percent, active_handles, event_loop->active_reqs.count, since_last_idle_percent);

    reset_since_last();

    return info_object;
}

static napi_value uv_check(napi_env env, napi_callback_info info) {
    napi_status status;
    uv_loop_t* napi_event_loop;
    uv_loop_t* uv_d_event_loop;


    status = napi_get_uv_event_loop(env, &napi_event_loop);
    if (status != napi_ok) {
        printf("C: Error getting UV event loop: %u\n", status);
        return 0;
    }
    printf("C: NAPI Loop   : %p\n", &napi_event_loop);

    uv_d_event_loop = uv_default_loop();
    printf("C: Default Loop: %p\n", &uv_d_event_loop);

    const char* version = uv_version_string();
    printf("C: UV Version: %s\n", version);

    return 0;
}

static napi_value init(napi_env env, napi_value exports) {
    DEFINE_NAPI_METHOD("initUvCheck", init_uv_check);
    DEFINE_NAPI_METHOD("stopUvCheck", stop_uv_check);
    DEFINE_NAPI_METHOD("getUvCheckInfo", get_uv_check_info);

    // Test functions, not really useful
    DEFINE_NAPI_METHOD("uvCheck", uv_check);

    return exports;
}

NAPI_MODULE(uvc, init)
