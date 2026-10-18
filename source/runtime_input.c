#define _GNU_SOURCE 1
#include "runtime.h"


#if defined(__SWITCH__)
/* input */
#include <math.h>
#include <string.h>
#include <switch.h>
#include <stdatomic.h>
#include <time.h>
#include <pthread.h>
#include "port_controls.h"

static PaleteriaCursor paleteria_cursor={.x=640,.y=360};
static _Atomic uint32_t paleteria_cursor_frame;
static uint64_t paleteria_cursor_previous_ns;
static pthread_mutex_t paleteria_cursor_lock=PTHREAD_MUTEX_INITIALIZER;
static float input_normalize_stick(s32 value);
static uint64_t input_map_buttons(u64 raw);
static float paleteria_cursor_dt(void) {
    struct timespec now;
    if(clock_gettime(CLOCK_MONOTONIC,&now)!=0)return 0;
    uint64_t ns=(uint64_t)now.tv_sec*1000000000u+now.tv_nsec;
    float dt=paleteria_cursor_previous_ns&&ns>=paleteria_cursor_previous_ns?(ns-paleteria_cursor_previous_ns)/1e9f:0;
    paleteria_cursor_previous_ns=ns;return dt;
}
static void paleteria_cursor_publish(bool pressed) {
    atomic_store(&paleteria_cursor_frame,(uint32_t)paleteria_cursor.x|
        ((uint32_t)paleteria_cursor.y<<11)|(paleteria_cursor.enabled?1u<<30:0)|
        (pressed&&!paleteria_cursor.blocked?1u<<31:0));
}
uint32_t paleteria_cursor_snapshot(void) {
    if(!(atomic_load(&paleteria_cursor_frame)&(1u<<30)))return 0;
    /* Sample immediately before presentation, independently of guest Java work.
     * A separate PadState never consumes the main thread's button edges. */
    static _Thread_local PadState render_pad;
    static _Thread_local bool ready;
    if(!ready){padInitializeDefault(&render_pad);ready=true;}
    padUpdate(&render_pad);
    HidAnalogStickState stick=padGetStickPos(&render_pad,0);
    pthread_mutex_lock(&paleteria_cursor_lock);
    if(padIsConnected(&render_pad))
        paleteria_cursor_move_buttons(&paleteria_cursor,input_normalize_stick(stick.x),input_normalize_stick(stick.y),
            input_map_buttons(padGetButtons(&render_pad)),paleteria_cursor_dt());
    paleteria_cursor_publish((atomic_load(&paleteria_cursor_frame)&(1u<<31))!=0);
    uint32_t frame=atomic_load(&paleteria_cursor_frame);
    pthread_mutex_unlock(&paleteria_cursor_lock);
    return frame;
}



static PadState input_g_pad;
static bool input_g_initialized;
static bool input_g_sensor_started;
static HidSixAxisSensorHandle input_g_sensor_handles[APK64_INPUT_MAX_SENSORS];
static int input_g_sensor_count;
static HidNpadIdType input_g_sensor_id;
static HidNpadStyleTag input_g_sensor_style;
static Apk64InputSessionStats input_g_stats;


static uint64_t input_map_buttons(u64 raw) {
    uint64_t out = 0;
    if (raw & HidNpadButton_A) out |= APK64_BTN_A;
    if (raw & HidNpadButton_B) out |= APK64_BTN_B;
    if (raw & HidNpadButton_X) out |= APK64_BTN_X;
    if (raw & HidNpadButton_Y) out |= APK64_BTN_Y;
    if (raw & HidNpadButton_StickL) out |= APK64_BTN_LSTICK;
    if (raw & HidNpadButton_StickR) out |= APK64_BTN_RSTICK;
    if (raw & HidNpadButton_L) out |= APK64_BTN_L;
    if (raw & HidNpadButton_R) out |= APK64_BTN_R;
    if (raw & HidNpadButton_ZL) out |= APK64_BTN_ZL;
    if (raw & HidNpadButton_ZR) out |= APK64_BTN_ZR;
    if (raw & HidNpadButton_Plus) out |= APK64_BTN_PLUS;
    if (raw & HidNpadButton_Minus) out |= APK64_BTN_MINUS;
    if (raw & HidNpadButton_Left) out |= APK64_BTN_DLEFT;
    if (raw & HidNpadButton_Up) out |= APK64_BTN_DUP;
    if (raw & HidNpadButton_Right) out |= APK64_BTN_DRIGHT;
    if (raw & HidNpadButton_Down) out |= APK64_BTN_DDOWN;
    if (raw & HidNpadButton_AnySL) out |= APK64_BTN_SL;
    if (raw & HidNpadButton_AnySR) out |= APK64_BTN_SR;
    return out;
}

static float input_normalize_stick(s32 value) {
    float out = (float)value / (float)JOYSTICK_MAX;
    if (out > 1.0f) out = 1.0f;
    if (out < -1.0f) out = -1.0f;
    return out;
}

static HidNpadStyleTag input_choose_sensor_style(u32 styles) {
    if (styles & HidNpadStyleTag_NpadHandheld) return HidNpadStyleTag_NpadHandheld;
    if (styles & HidNpadStyleTag_NpadFullKey)  return HidNpadStyleTag_NpadFullKey;
    if (styles & HidNpadStyleTag_NpadJoyDual)  return HidNpadStyleTag_NpadJoyDual;
    if (styles & HidNpadStyleTag_NpadJoyLeft)  return HidNpadStyleTag_NpadJoyLeft;
    if (styles & HidNpadStyleTag_NpadJoyRight) return HidNpadStyleTag_NpadJoyRight;
    return (HidNpadStyleTag)0;
}

static int input_sensor_handle_count_for_style(HidNpadStyleTag style) {
    return style == HidNpadStyleTag_NpadJoyDual ? 2 : 1;
}

static void input_stop_sensors(void) {
    if (!input_g_sensor_started) return;
    for (int i = 0; i < input_g_sensor_count; ++i)
        (void)hidStopSixAxisSensor(input_g_sensor_handles[i]);
    input_g_sensor_started = false;
    input_g_sensor_count = 0;
}

static void input_refresh_sensors(bool handheld, u32 styles) {
    HidNpadIdType id = handheld ? HidNpadIdType_Handheld : HidNpadIdType_No1;
    HidNpadStyleTag style = input_choose_sensor_style(styles);
    if (!style) {
        input_stop_sensors();
        return;
    }

    if (input_g_sensor_started && id == input_g_sensor_id && style == input_g_sensor_style)
        return;

    input_stop_sensors();
    const int count = input_sensor_handle_count_for_style(style);
    Result rc = hidGetSixAxisSensorHandles(input_g_sensor_handles, count, id, style);
    if (R_FAILED(rc)) {
        APK64_LOGCW("INPUT", "sixaxis handles unavailable id=%d style=0x%x rc=0x%08x",
                    (int)id, (unsigned)style, rc);
        return;
    }

    int started = 0;
    for (int i = 0; i < count; ++i) {
        rc = hidStartSixAxisSensor(input_g_sensor_handles[i]);
        if (R_FAILED(rc)) {
            APK64_LOGCW("INPUT", "sixaxis start failed index=%d rc=0x%08x", i, rc);
            break;
        }
        started++;
    }
    if (started != count) {
        for (int i = 0; i < started; ++i)
            (void)hidStopSixAxisSensor(input_g_sensor_handles[i]);
        return;
    }

    input_g_sensor_id = id;
    input_g_sensor_style = style;
    input_g_sensor_count = count;
    input_g_sensor_started = true;
    APK64_LOGCI("INPUT", "sixaxis started id=%d style=0x%x handles=%d",
                (int)id, (unsigned)style, count);
}

bool apk64_input_init(void) {
    if (input_g_initialized) return true;
    memset(&input_g_stats, 0, sizeof(input_g_stats));

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&input_g_pad);
    hidInitializeTouchScreen();

    input_g_sensor_started = false;
    input_g_sensor_count = 0;
    input_g_initialized = true;
    APK64_LOGCI("INPUT", "input backend initialized: pad + touchscreen + sixaxis probe");
    return true;
}

void apk64_input_update(Apk64InputSnapshot *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!input_g_initialized) return;

    padUpdate(&input_g_pad);
    out->controller_connected = padIsConnected(&input_g_pad);
    out->handheld = padIsHandheld(&input_g_pad);
    out->style_set = padGetStyleSet(&input_g_pad);
    out->buttons = input_map_buttons(padGetButtons(&input_g_pad));
    out->buttons_down = input_map_buttons(padGetButtonsDown(&input_g_pad));
    out->buttons_up = input_map_buttons(padGetButtonsUp(&input_g_pad));

    HidAnalogStickState left = padGetStickPos(&input_g_pad, 0);
    HidAnalogStickState right = padGetStickPos(&input_g_pad, 1);
    out->left_x = input_normalize_stick(left.x);
    out->left_y = input_normalize_stick(left.y);
    out->right_x = input_normalize_stick(right.x);
    out->right_y = input_normalize_stick(right.y);

    HidTouchScreenState touch_state;
    memset(&touch_state, 0, sizeof(touch_state));
    if (hidGetTouchScreenStates(&touch_state, 1) > 0) {
        int count = touch_state.count;
        if (count < 0) count = 0;
        if (count > APK64_INPUT_MAX_TOUCHES) count = APK64_INPUT_MAX_TOUCHES;
        out->touch_count = count;
        for (int i = 0; i < count; ++i) {
            const HidTouchState *src = &touch_state.touches[i];
            Apk64TouchPoint *dst = &out->touches[i];
            dst->id = src->finger_id;
            dst->x = (float)src->x;
            dst->y = (float)src->y;
            dst->diameter_x = (float)src->diameter_x;
            dst->diameter_y = (float)src->diameter_y;
            dst->attributes = src->attributes;
        }
    }

    if (out->controller_connected)
        input_refresh_sensors(out->handheld, out->style_set);
    else
        input_stop_sensors();

    if (input_g_sensor_started) {
        for (int i = 0; i < input_g_sensor_count && i < APK64_INPUT_MAX_SENSORS; ++i) {
            HidSixAxisSensorState state;
            memset(&state, 0, sizeof(state));
            if (hidGetSixAxisSensorStates(input_g_sensor_handles[i], &state, 1) > 0) {
                out->sensors[out->sensor_count].available = true;
                out->sensors[out->sensor_count].accel_x = state.acceleration.x;
                out->sensors[out->sensor_count].accel_y = state.acceleration.y;
                out->sensors[out->sensor_count].accel_z = state.acceleration.z;
                out->sensors[out->sensor_count].gyro_x = state.angular_velocity.x;
                out->sensors[out->sensor_count].gyro_y = state.angular_velocity.y;
                out->sensors[out->sensor_count].gyro_z = state.angular_velocity.z;
                out->sensor_count++;
            }
        }
    }

    input_g_stats.frames++;
    if (out->controller_connected) input_g_stats.saw_controller = true;
    if (out->buttons_down) input_g_stats.saw_button = true;
    if (fabsf(out->left_x) > 0.20f || fabsf(out->left_y) > 0.20f) input_g_stats.saw_left_stick = true;
    if (fabsf(out->right_x) > 0.20f || fabsf(out->right_y) > 0.20f) input_g_stats.saw_right_stick = true;
    if (out->touch_count > 0) input_g_stats.saw_touch = true;
    if (out->touch_count > 1) input_g_stats.saw_multitouch = true;
    if (out->sensor_count > 0) {
        if (!input_g_stats.saw_sensor)
            APK64_LOGCI("INPUT", "sixaxis first sample accel=(%.3f,%.3f,%.3f) gyro=(%.3f,%.3f,%.3f)",
                        (double)out->sensors[0].accel_x, (double)out->sensors[0].accel_y,
                        (double)out->sensors[0].accel_z, (double)out->sensors[0].gyro_x,
                        (double)out->sensors[0].gyro_y, (double)out->sensors[0].gyro_z);
        input_g_stats.saw_sensor = true;
    }
    if ((uint32_t)out->touch_count > input_g_stats.max_touches) input_g_stats.max_touches = (uint32_t)out->touch_count;

    if (out->buttons_down)
        APK64_LOGCI("INPUT", "buttons down=0x%016llx held=0x%016llx",
                    (unsigned long long)out->buttons_down, (unsigned long long)out->buttons);
    if (out->buttons_up)
        APK64_LOGCI("INPUT", "buttons up=0x%016llx held=0x%016llx",
                    (unsigned long long)out->buttons_up, (unsigned long long)out->buttons);
    pthread_mutex_lock(&paleteria_cursor_lock);
    float dt=paleteria_cursor_dt();
    bool pressed=(out->buttons&APK64_BTN_A)!=0;
    bool cursor_was_enabled=paleteria_cursor.enabled;
    paleteria_cursor_update(&paleteria_cursor,out,dt);
    if(cursor_was_enabled!=paleteria_cursor.enabled)
        APK64_LOGCI("CURSOR","enabled=%d position=%.0f,%.0f",paleteria_cursor.enabled,
                    (double)paleteria_cursor.x,(double)paleteria_cursor.y);
    paleteria_cursor_publish(pressed);
    pthread_mutex_unlock(&paleteria_cursor_lock);
}

void apk64_input_get_stats(Apk64InputSessionStats *out) {
    if (out) *out = input_g_stats;
}

void apk64_input_shutdown(void) {
    if (!input_g_initialized) return;
    input_stop_sensors();
    input_g_initialized = false;
    pthread_mutex_lock(&paleteria_cursor_lock);
    paleteria_cursor.enabled=false;
    paleteria_cursor_previous_ns=0;
    atomic_store(&paleteria_cursor_frame,0);
    pthread_mutex_unlock(&paleteria_cursor_lock);
    APK64_LOGCI("INPUT", "input backend shutdown");
}




#endif
/* sensors */
#pragma push_macro("APK64_ANDROID_STANDARD_GRAVITY")
#pragma push_macro("APK64_SENSOR_QUEUE_MAX")





#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct Apk64ASensorManager { int tag; };
struct Apk64ASensor {
    int type;
    int handle;
    int min_delay_us;
    const char *name;
    const char *vendor;
    float resolution;
};
struct Apk64ASensorEventQueue {
    Apk64ALooper *looper;
    bool accel_enabled;
    bool gyro_enabled;
    int32_t period_us;
    uint64_t last_sequence;
    unsigned consumed;
    int fds[2];
    bool notified;
};

typedef struct sensors_SensorState {
    pthread_mutex_t lock;
    bool initialized;
    uint64_t sequence;
    int64_t timestamp_ns;
    bool have_accel;
    bool have_gyro;
    float accel[3];
    float gyro[3];
} sensors_SensorState;

static struct Apk64ASensorManager sensors_g_manager = { 0x6400 };
static const struct Apk64ASensor sensors_g_accel = {
    APK64_ASENSOR_TYPE_ACCELEROMETER, 1, 10000,
    "APK64 Accelerometer", "APK64", 0.0f
};
static const struct Apk64ASensor sensors_g_gyro = {
    APK64_ASENSOR_TYPE_GYROSCOPE, 4, 10000,
    "APK64 Gyroscope", "APK64", 0.0f
};
static const Apk64ASensor *const sensors_g_sensor_list[] = { &sensors_g_accel, &sensors_g_gyro };
static sensors_SensorState sensors_g_state = { .lock = PTHREAD_MUTEX_INITIALIZER };
#define APK64_SENSOR_QUEUE_MAX 8
#define APK64_ANDROID_STANDARD_GRAVITY 9.80665f
static Apk64ASensorEventQueue *sensors_g_queues[APK64_SENSOR_QUEUE_MAX];
static uint64_t sensors_g_accel_events_delivered;

/* Readiness lives on a real APK64 descriptor so callback and identifier queues
 * use the same ALooper path as other Android descriptor sources. */
static unsigned sensors_sensor_queue_ready(const Apk64ASensorEventQueue *q) {
    unsigned ready = (q->accel_enabled && sensors_g_state.have_accel ? 1u : 0u) |
                     (q->gyro_enabled && sensors_g_state.have_gyro ? 2u : 0u);
    return ready & ~(q->last_sequence == sensors_g_state.sequence ? q->consumed : 0u);
}

/* Caller holds g_state.lock. Coalesce notification bytes along with samples. */
static void sensors_sensor_queue_notify(Apk64ASensorEventQueue *q) {
    unsigned char token = 1;
    const bool ready = sensors_sensor_queue_ready(q) != 0;
    if (ready && !q->notified)
        q->notified = apk64_fd_write(q->fds[1], &token, 1) == 1;
    else if (!ready && q->notified) {
        (void)apk64_fd_read(q->fds[0], &token, 1);
        q->notified = false;
    }
}

static int64_t sensors_monotonic_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (int64_t)ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec;
}

Apk64ASensorManager *ASensorManager_getInstance(void) { return &sensors_g_manager; }
Apk64ASensorManager *ASensorManager_getInstanceForPackage(const char *package_name) {
    (void)package_name;
    return &sensors_g_manager;
}

int ASensorManager_getSensorList(Apk64ASensorManager *manager,
                                 const Apk64ASensor *const **out_list) {
    if (!manager || !out_list) return -EINVAL;
    *out_list = sensors_g_sensor_list;
    return (int)(sizeof(sensors_g_sensor_list) / sizeof(sensors_g_sensor_list[0]));
}

const Apk64ASensor *ASensorManager_getDefaultSensor(Apk64ASensorManager *manager, int type) {
    if (!manager) return NULL;
    if (type == APK64_ASENSOR_TYPE_ACCELEROMETER) return &sensors_g_accel;
    if (type == APK64_ASENSOR_TYPE_GYROSCOPE) return &sensors_g_gyro;
    return NULL;
}

Apk64ASensorEventQueue *ASensorManager_createEventQueue(Apk64ASensorManager *manager,
                                                         Apk64ALooper *looper,
                                                         int ident,
                                                         int (*callback)(int, int, void *),
                                                         void *data) {
    if (!manager || !looper) {
        errno = EINVAL;
        return NULL;
    }
    Apk64ASensorEventQueue *q = calloc(1, sizeof(*q));
    if (!q) return NULL;
    q->looper = looper;
    q->period_us = 33333;
    if (apk64_fd_pipe(q->fds) != 0) { free(q); return NULL; }
    pthread_mutex_lock(&sensors_g_state.lock);
    q->last_sequence = sensors_g_state.sequence;
    bool registered = false;
    for (size_t i = 0; i < APK64_SENSOR_QUEUE_MAX; ++i) {
        if (!sensors_g_queues[i]) { sensors_g_queues[i] = q; registered = true; break; }
    }
    if (registered && ALooper_addFd(looper, q->fds[0], ident, 1, callback, data) != 1) {
        for (size_t i = 0; i < APK64_SENSOR_QUEUE_MAX; ++i) if (sensors_g_queues[i] == q) sensors_g_queues[i] = NULL;
        registered = false;
        errno = EINVAL;
    } else if (!registered) errno = ENOSPC;
    pthread_mutex_unlock(&sensors_g_state.lock);
    if (!registered) {
        (void)apk64_fd_close(q->fds[0]); (void)apk64_fd_close(q->fds[1]);
        free(q); return NULL;
    }
    APK64_LOGCI("SENSOR", "event queue created ident=%d callback=%p looper=%p fd=%d", ident, (void *)callback, (void *)looper, q->fds[0]);
    return q;
}

int ASensorManager_destroyEventQueue(Apk64ASensorManager *manager, Apk64ASensorEventQueue *queue) {
    if (!manager || !queue) return -EINVAL;
    pthread_mutex_lock(&sensors_g_state.lock);
    for (size_t i = 0; i < APK64_SENSOR_QUEUE_MAX; ++i) if (sensors_g_queues[i] == queue) sensors_g_queues[i] = NULL;
    (void)ALooper_removeFd(queue->looper, queue->fds[0]);
    (void)apk64_fd_close(queue->fds[0]); (void)apk64_fd_close(queue->fds[1]);
    pthread_mutex_unlock(&sensors_g_state.lock);
    free(queue);
    return 0;
}

static bool sensors_sensor_matches(const Apk64ASensor *sensor, int type) {
    return sensor && sensor->type == type;
}

int ASensorEventQueue_enableSensor(Apk64ASensorEventQueue *queue, const Apk64ASensor *sensor) {
    if (!queue || !sensor) return -EINVAL;
    if (!sensors_sensor_matches(sensor, 1) && !sensors_sensor_matches(sensor, 4)) return -EINVAL;
    pthread_mutex_lock(&sensors_g_state.lock);
    if (sensors_sensor_matches(sensor, APK64_ASENSOR_TYPE_ACCELEROMETER)) queue->accel_enabled = true;
    else if (sensors_sensor_matches(sensor, APK64_ASENSOR_TYPE_GYROSCOPE)) queue->gyro_enabled = true;
    sensors_sensor_queue_notify(queue);
    pthread_mutex_unlock(&sensors_g_state.lock);
    APK64_LOGCI("SENSOR", "sensor enabled type=%d", sensor->type);
    return 0;
}

int ASensorEventQueue_disableSensor(Apk64ASensorEventQueue *queue, const Apk64ASensor *sensor) {
    if (!queue || !sensor) return -EINVAL;
    if (!sensors_sensor_matches(sensor, 1) && !sensors_sensor_matches(sensor, 4)) return -EINVAL;
    pthread_mutex_lock(&sensors_g_state.lock);
    if (sensors_sensor_matches(sensor, APK64_ASENSOR_TYPE_ACCELEROMETER)) queue->accel_enabled = false;
    else if (sensors_sensor_matches(sensor, APK64_ASENSOR_TYPE_GYROSCOPE)) queue->gyro_enabled = false;
    sensors_sensor_queue_notify(queue);
    pthread_mutex_unlock(&sensors_g_state.lock);
    return 0;
}

int ASensorEventQueue_setEventRate(Apk64ASensorEventQueue *queue, const Apk64ASensor *sensor,
                                   int32_t usec) {
    if (!queue || !sensor || usec < 0) return -EINVAL;
    pthread_mutex_lock(&sensors_g_state.lock);
    queue->period_us = usec < sensor->min_delay_us ? sensor->min_delay_us : usec;
    pthread_mutex_unlock(&sensors_g_state.lock);
    return 0;
}

static void sensors_sensor_write_event(Apk64ASensorEvent *ev, const Apk64ASensor *sensor, const float *values) {
    memset(ev, 0, sizeof(*ev));
    ev->version = (int32_t)sizeof(*ev);
    ev->sensor = sensor->handle;
    ev->type = sensor->type;
    ev->timestamp = sensors_g_state.timestamp_ns;
    memcpy(ev->data, values, 3 * sizeof(float));
}

size_t apk64_sensors_read_latest(Apk64ASensorEvent *events, size_t capacity) {
    if (!events) return 0;
    size_t count = 0;
    pthread_mutex_lock(&sensors_g_state.lock);
    if (sensors_g_state.have_accel && count < capacity) sensors_sensor_write_event(&events[count++], &sensors_g_accel, sensors_g_state.accel);
    if (sensors_g_state.have_gyro && count < capacity) sensors_sensor_write_event(&events[count++], &sensors_g_gyro, sensors_g_state.gyro);
    pthread_mutex_unlock(&sensors_g_state.lock);
    return count;
}

int ASensorEventQueue_hasEvents(Apk64ASensorEventQueue *queue) {
    if (!queue) return -EINVAL;
    pthread_mutex_lock(&sensors_g_state.lock);
    const bool ready = sensors_sensor_queue_ready(queue) != 0;
    pthread_mutex_unlock(&sensors_g_state.lock);
    return ready ? 1 : 0;
}

ssize_t ASensorEventQueue_getEvents(Apk64ASensorEventQueue *queue,
                                    Apk64ASensorEvent *events, size_t count) {
    if (!queue || (!events && count)) return -EINVAL;
    if (count == 0) return 0;

    pthread_mutex_lock(&sensors_g_state.lock);
    if (queue->last_sequence != sensors_g_state.sequence) {
        queue->last_sequence = sensors_g_state.sequence;
        queue->consumed = 0;
    }
    const unsigned ready = sensors_sensor_queue_ready(queue);
    size_t written = 0;
    if ((ready & 1u) && written < count) {
        Apk64ASensorEvent *ev = &events[written++];
        memset(ev, 0, sizeof(*ev));
        ev->version = (int32_t)sizeof(*ev);
        ev->sensor = sensors_g_accel.handle;
        ev->type = sensors_g_accel.type;
        ev->timestamp = sensors_g_state.timestamp_ns;
        ev->data[0] = sensors_g_state.accel[0];
        ev->data[1] = sensors_g_state.accel[1];
        ev->data[2] = sensors_g_state.accel[2];
        queue->consumed |= 1u;
        sensors_g_accel_events_delivered++;
        if (sensors_g_accel_events_delivered <= 3u || (sensors_g_accel_events_delivered % 300u) == 0u) {
            APK64_LOGCI("SENSOR", "Android accel event=%llu m/s2=(%.3f,%.3f,%.3f)",
                        (unsigned long long)sensors_g_accel_events_delivered,
                        (double)ev->data[0], (double)ev->data[1], (double)ev->data[2]);
        }
    }
    if ((ready & 2u) && written < count) {
        Apk64ASensorEvent *ev = &events[written++];
        memset(ev, 0, sizeof(*ev));
        ev->version = (int32_t)sizeof(*ev);
        ev->sensor = sensors_g_gyro.handle;
        ev->type = sensors_g_gyro.type;
        ev->timestamp = sensors_g_state.timestamp_ns;
        ev->data[0] = sensors_g_state.gyro[0];
        ev->data[1] = sensors_g_state.gyro[1];
        ev->data[2] = sensors_g_state.gyro[2];
        queue->consumed |= 2u;
    }
    sensors_sensor_queue_notify(queue);
    pthread_mutex_unlock(&sensors_g_state.lock);
    return (ssize_t)written;
}

const char *ASensor_getName(const Apk64ASensor *sensor) {
    return sensor ? sensor->name : NULL;
}

const char *ASensor_getVendor(const Apk64ASensor *sensor) {
    return sensor ? sensor->vendor : NULL;
}

int ASensor_getType(const Apk64ASensor *sensor) {
    return sensor ? sensor->type : 0;
}

float ASensor_getResolution(const Apk64ASensor *sensor) {
    /* APK64 currently exposes a host-backed virtual sensor rather than raw
     * sensor-HAL quantization. Zero advertises no stronger precision claim. */
    return sensor ? sensor->resolution : 0.0f;
}

int ASensor_getMinDelay(const Apk64ASensor *sensor) {
    return sensor ? sensor->min_delay_us : 0;
}

void apk64_sensors_publish(const Apk64InputSnapshot *snapshot) {
    if (!snapshot) return;
    pthread_mutex_lock(&sensors_g_state.lock);
    sensors_g_state.sequence++;
    sensors_g_state.timestamp_ns = sensors_monotonic_ns();
    sensors_g_state.have_accel = false;
    sensors_g_state.have_gyro = false;
    if (snapshot->sensor_count > 0 && snapshot->sensors[0].available) {
        const Apk64MotionSample *s = &snapshot->sensors[0];
        sensors_g_state.have_accel = true;
        sensors_g_state.have_gyro = true;
        /* Sensors use the device's fixed natural frame. Display.getRotation()
         * already reports portrait quarter-turns from the Switch's natural
         * landscape orientation; rotating samples here makes apps apply that
         * turn twice and swaps side tilt with forward tilt. Keep chassis X/Y
         * fixed and convert libnx's gravity sign and g units at this boundary
         * (screen-up Switch Z=-1 g, Android Z=+9.80665 m/s^2). */
        sensors_g_state.accel[0] = -s->accel_x * APK64_ANDROID_STANDARD_GRAVITY;
        sensors_g_state.accel[1] = -s->accel_y * APK64_ANDROID_STANDARD_GRAVITY;
        sensors_g_state.accel[2] = -s->accel_z * APK64_ANDROID_STANDARD_GRAVITY;

        /* Angular velocity retains its chassis axes and rotation signs. */
        sensors_g_state.gyro[0] = s->gyro_x;
        sensors_g_state.gyro[1] = s->gyro_y;
        sensors_g_state.gyro[2] =  s->gyro_z;
    }
    for (size_t i = 0; i < APK64_SENSOR_QUEUE_MAX; ++i)
        if (sensors_g_queues[i]) sensors_sensor_queue_notify(sensors_g_queues[i]);
    pthread_mutex_unlock(&sensors_g_state.lock);
}

void apk64_sensors_reset(void) {
    pthread_mutex_lock(&sensors_g_state.lock);
    sensors_g_state.sequence++;
    sensors_g_state.timestamp_ns = 0;
    sensors_g_state.have_accel = false;
    sensors_g_state.have_gyro = false;
    memset(sensors_g_state.accel, 0, sizeof(sensors_g_state.accel));
    memset(sensors_g_state.gyro, 0, sizeof(sensors_g_state.gyro));
    sensors_g_accel_events_delivered = 0;
    for (size_t i = 0; i < APK64_SENSOR_QUEUE_MAX; ++i) {
        if (sensors_g_queues[i]) {
            (void)ALooper_removeFd(sensors_g_queues[i]->looper, sensors_g_queues[i]->fds[0]);
            (void)apk64_fd_close(sensors_g_queues[i]->fds[0]);
            (void)apk64_fd_close(sensors_g_queues[i]->fds[1]);
        }
        free(sensors_g_queues[i]);
        sensors_g_queues[i] = NULL;
    }
    pthread_mutex_unlock(&sensors_g_state.lock);
}

#pragma pop_macro("APK64_SENSOR_QUEUE_MAX")
#pragma pop_macro("APK64_ANDROID_STANDARD_GRAVITY")


/* cocos_keyboard */



#include <stdlib.h>
#include <string.h>


#ifdef __SWITCH__
#include <switch.h>
#endif

Apk64Result apk64_keyboard_show(const Apk64KeyboardConfig *config,char *out_text,
                                 size_t out_text_size,bool *accepted) {
    if (!config || !accepted || !out_text || out_text_size < 4u) return APK64_ERR_ARGUMENT;
    *accepted = false;
    const char *initial_text = config->initial_text;
    if (!initial_text) initial_text = "";
    out_text[0] = '\0';

#ifdef __SWITCH__
    SwkbdConfig kbd;
    Result rc = swkbdCreate(&kbd, 0);
    if (R_FAILED(rc)) {
        APK64_LOGCW("KEYBOARD", "swkbdCreate failed rc=0x%08x", (unsigned)rc);
        return APK64_ERR_IO;
    }

    swkbdConfigMakePresetDefault(&kbd);
    swkbdConfigSetType(&kbd,config->numeric ? SwkbdType_NumPad : SwkbdType_Normal);
    swkbdConfigSetPasswordFlag(&kbd,config->password ? 1 : 0);
    swkbdConfigSetDicFlag(&kbd,config->password ? 0 : 1);
    swkbdConfigSetReturnButtonFlag(&kbd,config->multiline ? 1 : 0);
    if(config->numeric && config->decimal)swkbdConfigSetLeftOptionalSymbolKey(&kbd,".");
    if(config->numeric && config->signed_number)swkbdConfigSetRightOptionalSymbolKey(&kbd,"-");
    if(config->action_label)swkbdConfigSetOkButtonText(&kbd,config->action_label);
    swkbdConfigSetInitialText(&kbd, initial_text);
    /* One UTF-16 unit needs at most three UTF-8 bytes. Never cut a character
     * to fit the output buffer. The modal libnx output holds 1000 units. */
    size_t max_units=(out_text_size-1u)/3u;
    if(max_units>1000u)max_units=1000u;
    swkbdConfigSetStringLenMax(&kbd,(u32)max_units);
    swkbdConfigSetHeaderText(&kbd, "Text input");

    rc = swkbdShow(&kbd, out_text, out_text_size);
    swkbdClose(&kbd);
    if (R_FAILED(rc)) {
        APK64_LOGCW("KEYBOARD", "swkbdShow closed/cancelled rc=0x%08x", (unsigned)rc);
        /* libnx uses BadExit for both cancel and abnormal exits. Retain rc,
         * return no accepted edit, and let the UI caller preserve its text. */
        out_text[0]='\0';
        return rc==MAKERESULT(Module_Libnx,LibnxError_LibAppletBadExit)?APK64_OK:APK64_ERR_IO;
    }
    APK64_LOGCI("KEYBOARD", "Switch software keyboard committed bytes=%zu", strlen(out_text));
    *accepted=true;
    return APK64_OK;
#else
    /* Host-only deterministic override used by the retained JNI/DEX regression.
     * Switch builds never inspect this environment variable. */
    if(getenv("APK64_TEST_KEYBOARD_FAIL"))return APK64_ERR_IO;
    if(getenv("APK64_TEST_KEYBOARD_CANCEL"))return APK64_OK;
    const char *host_text = getenv("APK64_TEST_KEYBOARD_TEXT");
    if (!host_text) host_text = initial_text;
    size_t n = strlen(host_text);
    if (n >= out_text_size) return APK64_ERR_ARGUMENT;
    memcpy(out_text, host_text, n);
    out_text[n] = '\0';
    *accepted=true;
    return APK64_OK;
#endif
}

Apk64Result apk64_cocos_keyboard_show(const char *initial_text,char *out_text,size_t capacity) {
    Apk64KeyboardConfig config={.initial_text=initial_text,.multiline=true};bool accepted=false;
    Apk64Result rc=apk64_keyboard_show(&config,out_text,capacity,&accepted);
    /* Keep the established Cocos unchanged-edit cancellation behavior. */
    if(!accepted && rc==APK64_OK){
        if(!initial_text)initial_text="";
        size_t n=strlen(initial_text);if(n>=capacity)n=capacity-1u;
        memcpy(out_text,initial_text,n);out_text[n]='\0';return APK64_OK;
    }
    return rc;
}



/* native_activity */
#pragma push_macro("APK64_AINPUT_EVENT_TYPE_KEY")
#pragma push_macro("APK64_AINPUT_EVENT_TYPE_MOTION")
#pragma push_macro("APK64_AKEY_DOWN")
#pragma push_macro("APK64_AKEY_UP")
#pragma push_macro("APK64_ALOOPER_EVENT_ERROR")
#pragma push_macro("APK64_ALOOPER_EVENT_HANGUP")
#pragma push_macro("APK64_ALOOPER_EVENT_INPUT")
#pragma push_macro("APK64_ALOOPER_EVENT_INVALID")
#pragma push_macro("APK64_ALOOPER_EVENT_OUTPUT")
#pragma push_macro("APK64_ALOOPER_POLL_CALLBACK")
#pragma push_macro("APK64_ALOOPER_POLL_ERROR")
#pragma push_macro("APK64_ALOOPER_POLL_TIMEOUT")
#pragma push_macro("APK64_ALOOPER_POLL_WAKE")
#pragma push_macro("APK64_ALOOPER_PREPARE_ALLOW_NON_CALLBACKS")
#pragma push_macro("APK64_AMOTION_DOWN")
#pragma push_macro("APK64_AMOTION_MOVE")
#pragma push_macro("APK64_AMOTION_UP")
#pragma push_macro("APK64_CHOREO_FRAME_NS")
#pragma push_macro("APK64_INPUT_MAX_POINTERS")
#pragma push_macro("APK64_INPUT_QUEUE_CAP")
#pragma push_macro("CONFIG_GETTER")
#pragma push_macro("CONFIG_SETTER")









#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern bool apk64_diag_full_trace_enabled(void) __attribute__((weak));
static bool native_activity_choreographer_full_trace(void) {
    return apk64_diag_full_trace_enabled && apk64_diag_full_trace_enabled();
}

#define APK64_INPUT_QUEUE_CAP 32
#define APK64_INPUT_MAX_POINTERS 16

#define APK64_AINPUT_EVENT_TYPE_KEY 1
#define APK64_AINPUT_EVENT_TYPE_MOTION 2
#define APK64_AKEY_DOWN 0
#define APK64_AKEY_UP 1
#define APK64_AMOTION_DOWN 0
#define APK64_AMOTION_UP 1
#define APK64_AMOTION_MOVE 2
#define APK64_ALOOPER_PREPARE_ALLOW_NON_CALLBACKS 1
#define APK64_ALOOPER_EVENT_INPUT 1
#define APK64_ALOOPER_EVENT_OUTPUT 2
#define APK64_ALOOPER_EVENT_ERROR 4
#define APK64_ALOOPER_EVENT_HANGUP 8
#define APK64_ALOOPER_EVENT_INVALID 16
#define APK64_ALOOPER_POLL_WAKE -1
#define APK64_ALOOPER_POLL_CALLBACK -2
#define APK64_ALOOPER_POLL_TIMEOUT -3
#define APK64_ALOOPER_POLL_ERROR -4

struct Apk64ANativeWindow {
    int32_t refs;
    int32_t width;
    int32_t height;
    int32_t format;
};

struct Apk64AChoreographer {
    pthread_t owner;
    Apk64ALooper *looper;
    bool initialized;
};

typedef struct native_activity_Apk64ChoreographerCallback {
    pthread_t owner;
    Apk64ALooper *looper;
    uint64_t due_ns;
    bool is64;
    union {
        void (*legacy)(long, void *);
        void (*cb64)(int64_t, void *);
    } fn;
    void *data;
    void *caller; /* Diagnostic posting boundary, never used for dispatch. */
    struct native_activity_Apk64ChoreographerCallback *next;
} native_activity_Apk64ChoreographerCallback;

#define APK64_CHOREO_FRAME_NS UINT64_C(16666667)

static pthread_mutex_t native_activity_g_choreo_lock = PTHREAD_MUTEX_INITIALIZER;
static native_activity_Apk64ChoreographerCallback *native_activity_g_choreo_head;
static native_activity_Apk64ChoreographerCallback *native_activity_g_choreo_tail;
static _Thread_local Apk64AChoreographer native_activity_g_thread_choreographer;
static atomic_uint native_activity_g_choreo_get_reports, native_activity_g_choreo_post_reports, native_activity_g_choreo_dispatch_reports;
static uint64_t native_activity_g_choreo_last_queue_report_ns; /* Protected by g_choreo_lock. */

struct Apk64AConfiguration {
    Apk64AssetConfiguration values;
};

typedef struct native_activity_Apk64LooperFd {
    int fd;
    int ident;
    int events;
    int (*callback)(int fd, int events, void *data);
    void *data;
} native_activity_Apk64LooperFd;

struct Apk64ALooper {
    pthread_mutex_t lock;
    native_activity_Apk64LooperFd *fds;
    size_t fd_count;
    size_t fd_capacity;
    int opts;
    pthread_t owner_thread;
    atomic_int references;
    int wake_read_fd;
    int wake_write_fd;
};

struct Apk64AInputEvent {
    int32_t type;
    int32_t action;
    int32_t key_code;
    size_t pointer_count;
    int32_t ids[APK64_INPUT_MAX_POINTERS];
    float xs[APK64_INPUT_MAX_POINTERS];
    float ys[APK64_INPUT_MAX_POINTERS];
};

struct Apk64AInputQueue {
    pthread_mutex_t lock;
    Apk64AInputEvent events[APK64_INPUT_QUEUE_CAP];
    size_t head;
    size_t count;
    Apk64AInputEvent current;
    bool current_valid;
    Apk64ALooper *looper;
    int ident;
    int (*callback)(int fd, int events, void *data);
    void *data;
};

typedef struct native_activity_Apk64NativeRuntime {
    bool initialized;
    bool started;
    bool finish_requested;
    bool input_initialized;
    Apk64ANativeActivity activity;
    Apk64ANativeActivityCallbacks callbacks;
    Apk64ANativeWindow window;
    Apk64AInputQueue input_queue;
    char internal_path[512];
    char external_path[512];
    char obb_path[512];
    Apk64InputSnapshot previous_input;
    Apk64DisplayPlan display_plan;
    bool previous_touch_active;
    int32_t previous_touch_id;
    float previous_touch_x;
    float previous_touch_y;
} native_activity_Apk64NativeRuntime;

static native_activity_Apk64NativeRuntime native_activity_g_native;
static __thread Apk64ALooper *native_activity_g_thread_looper;

static bool native_activity_copy_string(char *dst, size_t cap, const char *src) {
    if (!dst || !cap || !src) return false;
    const size_t n = strlen(src);
    if (n >= cap) return false;
    memcpy(dst, src, n + 1);
    return true;
}

static void native_activity_queue_wake(Apk64AInputQueue *queue) {
    if (!queue || !queue->looper) return;
    ALooper_wake(queue->looper);
}

static bool native_activity_queue_event(Apk64AInputQueue *queue, const Apk64AInputEvent *event) {
    if (!queue || !event) return false;
    pthread_mutex_lock(&queue->lock);
    if (queue->count == APK64_INPUT_QUEUE_CAP) {
        queue->head = (queue->head + 1) % APK64_INPUT_QUEUE_CAP;
        queue->count--;
    }
    const size_t tail = (queue->head + queue->count) % APK64_INPUT_QUEUE_CAP;
    queue->events[tail] = *event;
    queue->count++;
    pthread_mutex_unlock(&queue->lock);
    native_activity_queue_wake(queue);
    return true;
}

static bool native_activity_queue_has_events(Apk64AInputQueue *queue) {
    if (!queue) return false;
    pthread_mutex_lock(&queue->lock);
    const bool have = queue->current_valid || queue->count > 0;
    pthread_mutex_unlock(&queue->lock);
    return have;
}

static void native_activity_make_single_motion(Apk64AInputEvent *event, int action, int32_t id, float x, float y) {
    memset(event, 0, sizeof(*event));
    event->type = APK64_AINPUT_EVENT_TYPE_MOTION;
    event->action = action;
    event->pointer_count = 1;
    event->ids[0] = id;
    event->xs[0] = x;
    event->ys[0] = y;
}

static void native_activity_make_key_event(Apk64AInputEvent *event, int action, int32_t key_code) {
    memset(event, 0, sizeof(*event));
    event->type = APK64_AINPUT_EVENT_TYPE_KEY;
    event->action = action;
    event->key_code = key_code;
}

static void native_activity_queue_key_changes(Apk64AInputQueue *queue, uint64_t changed, int action) {
    static const uint64_t bits[] = {
        APK64_BTN_DUP, APK64_BTN_DDOWN, APK64_BTN_DLEFT, APK64_BTN_DRIGHT,
        APK64_BTN_A, APK64_BTN_B, APK64_BTN_X, APK64_BTN_Y,
        APK64_BTN_L, APK64_BTN_R, APK64_BTN_ZL, APK64_BTN_ZR,
        APK64_BTN_LSTICK, APK64_BTN_RSTICK, APK64_BTN_MINUS,
    };
    for (size_t i = 0; i < sizeof(bits)/sizeof(bits[0]); ++i) {
        if (!(changed & bits[i])) continue;
        const int32_t code = apk64_android_keycode_for_button(bits[i]);
        if (!code) continue;
        Apk64AInputEvent ev;
        native_activity_make_key_event(&ev, action, code);
        (void)native_activity_queue_event(queue, &ev);
    }
}

bool apk64_native_activity_init(const Apk64NativeActivityConfig *config,
                                void *vm, void *env, void *activity_object,
                                Apk64AssetManager *asset_manager) {
    if (!config || !config->internal_data_path || !config->external_data_path || !asset_manager)
        return false;
    apk64_native_activity_reset();
    if (!native_activity_copy_string(native_activity_g_native.internal_path, sizeof(native_activity_g_native.internal_path), config->internal_data_path) ||
        !native_activity_copy_string(native_activity_g_native.external_path, sizeof(native_activity_g_native.external_path), config->external_data_path) ||
        !native_activity_copy_string(native_activity_g_native.obb_path, sizeof(native_activity_g_native.obb_path), config->obb_path ? config->obb_path : config->internal_data_path))
        return false;

    memset(&native_activity_g_native.callbacks, 0, sizeof(native_activity_g_native.callbacks));
    memset(&native_activity_g_native.activity, 0, sizeof(native_activity_g_native.activity));
    native_activity_g_native.window.refs = 1;
    native_activity_g_native.window.width = config->window_width > 0 ? config->window_width : 1280;
    native_activity_g_native.window.height = config->window_height > 0 ? config->window_height : 720;
    native_activity_g_native.window.format = config->window_format;
    if (config->display_plan.logical_width > 0 && config->display_plan.logical_height > 0) {
        native_activity_g_native.display_plan = config->display_plan;
    } else {
        (void)apk64_display_make_plan(APK64_SCREEN_ORIENTATION_UNSPECIFIED, false,
                                      native_activity_g_native.window.width, native_activity_g_native.window.height,
                                      &native_activity_g_native.display_plan);
    }
    pthread_mutex_init(&native_activity_g_native.input_queue.lock, NULL);

    native_activity_g_native.activity.callbacks = &native_activity_g_native.callbacks;
    native_activity_g_native.activity.vm = vm;
    native_activity_g_native.activity.env = env;
    native_activity_g_native.activity.clazz = activity_object;
    native_activity_g_native.activity.internalDataPath = native_activity_g_native.internal_path;
    native_activity_g_native.activity.externalDataPath = native_activity_g_native.external_path;
    native_activity_g_native.activity.sdkVersion = config->sdk_version > 0 ? config->sdk_version : 30;
    native_activity_g_native.activity.assetManager = asset_manager;
    native_activity_g_native.activity.obbPath = native_activity_g_native.obb_path;
    native_activity_g_native.initialized = true;
    APK64_LOGCI("NATIVE", "NativeActivity prepared size=%dx%d sdk=%d data=%s",
                native_activity_g_native.window.width, native_activity_g_native.window.height,
                native_activity_g_native.activity.sdkVersion, native_activity_g_native.internal_path);
    return true;
}

void apk64_native_activity_reset(void) {
    if (native_activity_g_native.input_initialized) {
        apk64_input_shutdown();
    }
    apk64_sensors_reset();
    if (native_activity_g_native.initialized) {
        pthread_mutex_destroy(&native_activity_g_native.input_queue.lock);
    }
    pthread_mutex_lock(&native_activity_g_choreo_lock);
    native_activity_Apk64ChoreographerCallback *cb = native_activity_g_choreo_head;
    native_activity_g_choreo_head = native_activity_g_choreo_tail = NULL;
    native_activity_g_choreo_last_queue_report_ns = 0;
    pthread_mutex_unlock(&native_activity_g_choreo_lock);
    while (cb) {
        native_activity_Apk64ChoreographerCallback *next = cb->next;
        free(cb);
        cb = next;
    }
    memset(&native_activity_g_thread_choreographer, 0, sizeof(native_activity_g_thread_choreographer));
    atomic_store(&native_activity_g_choreo_get_reports, 0);
    atomic_store(&native_activity_g_choreo_post_reports, 0);
    atomic_store(&native_activity_g_choreo_dispatch_reports, 0);
    memset(&native_activity_g_native, 0, sizeof(native_activity_g_native));
}

Apk64ANativeActivity *apk64_native_activity(void) { return native_activity_g_native.initialized ? &native_activity_g_native.activity : NULL; }
Apk64ANativeWindow *apk64_native_window(void) {
    return native_activity_g_native.window.refs > 0 ? &native_activity_g_native.window : NULL;
}
void apk64_native_window_prepare(int32_t width, int32_t height, int32_t format) {
    if (native_activity_g_native.window.refs <= 0) native_activity_g_native.window.refs = 1;
    native_activity_g_native.window.width = width > 0 ? width : 1280;
    native_activity_g_native.window.height = height > 0 ? height : 720;
    native_activity_g_native.window.format = format;
}
Apk64AInputQueue *apk64_native_input_queue(void) { return native_activity_g_native.initialized ? &native_activity_g_native.input_queue : NULL; }
bool apk64_native_window_dimensions(const Apk64ANativeWindow *window, int32_t *out_width, int32_t *out_height) {
    if (!window || window != &native_activity_g_native.window || native_activity_g_native.window.refs <= 0) return false;
    if (out_width) *out_width = window->width;
    if (out_height) *out_height = window->height;
    return window->width > 0 && window->height > 0;
}

bool apk64_native_activity_start(void) {
    if (!native_activity_g_native.initialized) return false;
    Apk64ANativeActivityCallbacks *cb = &native_activity_g_native.callbacks;
    if (!cb->onStart || !cb->onResume || !cb->onNativeWindowCreated || !cb->onInputQueueCreated) {
        APK64_LOGCE("NATIVE", "guest NativeActivity callbacks incomplete start=%p resume=%p window=%p input=%p",
                    (void *)cb->onStart, (void *)cb->onResume,
                    (void *)cb->onNativeWindowCreated, (void *)cb->onInputQueueCreated);
        return false;
    }
    if (!apk64_input_init()) {
        APK64_LOGCE("NATIVE", "Switch input backend init failed");
        return false;
    }
    native_activity_g_native.input_initialized = true;

    cb->onStart(&native_activity_g_native.activity);
    cb->onResume(&native_activity_g_native.activity);
    cb->onInputQueueCreated(&native_activity_g_native.activity, &native_activity_g_native.input_queue);
    cb->onNativeWindowCreated(&native_activity_g_native.activity, &native_activity_g_native.window);
    if (cb->onWindowFocusChanged) cb->onWindowFocusChanged(&native_activity_g_native.activity, 1);
    if (cb->onContentRectChanged) {
        Apk64ARect rect = {0, 0, native_activity_g_native.window.width, native_activity_g_native.window.height};
        cb->onContentRectChanged(&native_activity_g_native.activity, &rect);
    }
    native_activity_g_native.started = true;
    APK64_LOGCI("NATIVE", "NativeActivity lifecycle START/RESUME/window/input/focus delivered");
    return true;
}

void apk64_native_activity_pump_input(bool *out_exit_requested) {
    if (out_exit_requested) *out_exit_requested = false;
    if (!native_activity_g_native.started || !native_activity_g_native.input_initialized) return;

    Apk64InputSnapshot now;
    apk64_input_update(&now);
    apk64_sensors_publish(&now);
    if ((now.buttons_down & APK64_BTN_PLUS) || native_activity_g_native.finish_requested) {
        if (out_exit_requested) *out_exit_requested = true;
    }

    bool cur_touch = false;
    int32_t cur_id = -1;
    float cur_x = 0.0f, cur_y = 0.0f;
    if (now.touch_count > 0) {
        cur_id = (int32_t)now.touches[0].id;
        cur_touch = apk64_display_map_touch(&native_activity_g_native.display_plan,
                                            now.touches[0].x, now.touches[0].y,
                                            &cur_x, &cur_y);
    }

    const bool prev_touch = native_activity_g_native.previous_touch_active;
    Apk64AInputEvent event;
    if (!prev_touch && cur_touch) {
        native_activity_make_single_motion(&event, APK64_AMOTION_DOWN, cur_id, cur_x, cur_y);
        (void)native_activity_queue_event(&native_activity_g_native.input_queue, &event);
    } else if (prev_touch && cur_touch) {
        native_activity_make_single_motion(&event, APK64_AMOTION_MOVE, cur_id, cur_x, cur_y);
        (void)native_activity_queue_event(&native_activity_g_native.input_queue, &event);
    } else if (prev_touch && !cur_touch) {
        native_activity_make_single_motion(&event, APK64_AMOTION_UP,
                           native_activity_g_native.previous_touch_id,
                           native_activity_g_native.previous_touch_x,
                           native_activity_g_native.previous_touch_y);
        (void)native_activity_queue_event(&native_activity_g_native.input_queue, &event);
    }

    /* Controller buttons remain controller keys.  APK64 no longer fabricates
     * touchscreen taps from gamepad buttons; that mixed host/Android behavior
     * violated the input contract and made touch-only/gamepad-aware apps
     * indistinguishable. */
    native_activity_queue_key_changes(&native_activity_g_native.input_queue, now.buttons_down, APK64_AKEY_DOWN);
    native_activity_queue_key_changes(&native_activity_g_native.input_queue, now.buttons_up, APK64_AKEY_UP);

    native_activity_g_native.previous_touch_active = cur_touch;
    if (cur_touch) {
        native_activity_g_native.previous_touch_id = cur_id;
        native_activity_g_native.previous_touch_x = cur_x;
        native_activity_g_native.previous_touch_y = cur_y;
    }
    native_activity_g_native.previous_input = now;
}

void apk64_native_activity_stop(void) {
    if (!native_activity_g_native.started) return;
    Apk64ANativeActivityCallbacks *cb = &native_activity_g_native.callbacks;
    APK64_LOGCI("NATIVE", "NativeActivity shutdown lifecycle begin");
    if (cb->onWindowFocusChanged) cb->onWindowFocusChanged(&native_activity_g_native.activity, 0);
    if (cb->onInputQueueDestroyed) cb->onInputQueueDestroyed(&native_activity_g_native.activity, &native_activity_g_native.input_queue);
    if (cb->onNativeWindowDestroyed) cb->onNativeWindowDestroyed(&native_activity_g_native.activity, &native_activity_g_native.window);
    if (cb->onPause) cb->onPause(&native_activity_g_native.activity);
    if (cb->onStop) cb->onStop(&native_activity_g_native.activity);
    if (cb->onDestroy) cb->onDestroy(&native_activity_g_native.activity);
    native_activity_g_native.started = false;
    APK64_LOGCI("NATIVE", "NativeActivity shutdown lifecycle complete");
}

void ANativeActivity_finish(Apk64ANativeActivity *activity) {
    (void)activity;
    native_activity_g_native.finish_requested = true;
}
void ANativeActivity_setWindowFormat(Apk64ANativeActivity *activity, int32_t format) {
    (void)activity; native_activity_g_native.window.format = format;
}
void ANativeActivity_setWindowFlags(Apk64ANativeActivity *activity, uint32_t add_flags, uint32_t remove_flags) {
    (void)activity; (void)add_flags; (void)remove_flags;
}
void ANativeActivity_showSoftInput(Apk64ANativeActivity *activity, uint32_t flags) { (void)activity; (void)flags; }
void ANativeActivity_hideSoftInput(Apk64ANativeActivity *activity, uint32_t flags) { (void)activity; (void)flags; }

void ANativeWindow_acquire(Apk64ANativeWindow *window) { if (window) window->refs++; }
void ANativeWindow_release(Apk64ANativeWindow *window) { if (window && window->refs > 0) window->refs--; }
int32_t ANativeWindow_getWidth(Apk64ANativeWindow *window) { return window ? window->width : 0; }
int32_t ANativeWindow_getHeight(Apk64ANativeWindow *window) { return window ? window->height : 0; }
int32_t ANativeWindow_getFormat(Apk64ANativeWindow *window) { return window ? window->format : 0; }
int32_t ANativeWindow_setBuffersGeometry(Apk64ANativeWindow *window, int32_t width, int32_t height, int32_t format) {
    if (!window) return -EINVAL;
    /* Android permits width/height 0 to mean the base window dimensions. */
    if (width > 0) window->width = width;
    if (height > 0) window->height = height;
    if (format != 0) window->format = format;
    return 0;
}

Apk64ANativeWindow *ANativeWindow_fromSurface(void *env, void *surface) {
    (void)env;
    if (!surface || !apk64_jni_is_instance_of_class_host(surface, "android/view/Surface")) return NULL;
    Apk64ANativeWindow *window = apk64_native_window();
    if (!window) return NULL;
    ANativeWindow_acquire(window);
    return window;
}

void *ANativeWindow_toSurface(void *env, Apk64ANativeWindow *window) {
    if (!env || !window) return NULL;
    void *surface = apk64_jni_new_local_object(env, "android/view/Surface");
    if (!surface) return NULL;
    (void)apk64_jni_set_object_host_data(surface, window);
    return surface;
}

int32_t ANativeWindow_lock(Apk64ANativeWindow *window, void *buffer, Apk64ARect *dirty) {
    (void)dirty;
    if (!window || !buffer) return -22; /* Android EINVAL, independent of host errno. */
    APK64_LOGCW("NATIVE", "ANativeWindow_lock unavailable: no CPU window-buffer backend");
    return -38; /* ENOSYS: leave the caller's buffer and dirty rectangle untouched. */
}

int32_t ANativeWindow_unlockAndPost(Apk64ANativeWindow *window) {
    if (!window) return -22;
    return -38;
}

static uint64_t native_activity_choreographer_now_ns(void) {
    struct timespec ts = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static uint64_t native_activity_choreographer_next_frame_ns(uint64_t earliest_ns) {
    if (earliest_ns == 0) earliest_ns = 1;
    const uint64_t rem = earliest_ns % APK64_CHOREO_FRAME_NS;
    return rem ? earliest_ns + (APK64_CHOREO_FRAME_NS - rem)
               : earliest_ns + APK64_CHOREO_FRAME_NS;
}

Apk64AChoreographer *AChoreographer_getInstance(void) {
    if (!native_activity_g_thread_choreographer.initialized) {
        native_activity_g_thread_choreographer.owner = pthread_self();
        native_activity_g_thread_choreographer.looper = native_activity_g_thread_looper;
        native_activity_g_thread_choreographer.initialized = true;
    } else if (!native_activity_g_thread_choreographer.looper && native_activity_g_thread_looper) {
        /* Android associates a Choreographer with the calling thread's Looper.
         * Keep the historical permissive getInstance() behavior for callers that
         * query before preparing a Looper, but bind it as soon as one exists. */
        native_activity_g_thread_choreographer.looper = native_activity_g_thread_looper;
    }
    if (atomic_fetch_add(&native_activity_g_choreo_get_reports, 1) < 8u || native_activity_choreographer_full_trace())
        APK64_LOGCI("CHOREO", "get choreographer=%p owner=%p looper=%p caller=%p",
                    (void *)&native_activity_g_thread_choreographer,
                    (void *)(uintptr_t)native_activity_g_thread_choreographer.owner,
                    (void *)native_activity_g_thread_choreographer.looper,
                    __builtin_extract_return_addr(__builtin_return_address(0)));
    return &native_activity_g_thread_choreographer;
}

static void native_activity_choreographer_post_delayed(Apk64AChoreographer *choreographer, bool is64,
                                       void *callback, void *data, uint32_t delay_ms,
                                       void *caller) {
    if (!choreographer || !callback) return;
    native_activity_Apk64ChoreographerCallback *node = calloc(1, sizeof(*node));
    if (!node) return;

    const uint64_t now = native_activity_choreographer_now_ns();
    uint64_t earliest = now;
    if (delay_ms > 0 && UINT64_MAX - earliest > (uint64_t)delay_ms * UINT64_C(1000000))
        earliest += (uint64_t)delay_ms * UINT64_C(1000000);
    else if (delay_ms > 0)
        earliest = UINT64_MAX - APK64_CHOREO_FRAME_NS;

    node->owner = choreographer->initialized ? choreographer->owner : pthread_self();
    node->looper = choreographer->looper;
    node->due_ns = native_activity_choreographer_next_frame_ns(earliest);
    node->is64 = is64;
    if (is64) node->fn.cb64 = (void (*)(int64_t, void *))callback;
    else node->fn.legacy = (void (*)(long, void *))callback;
    node->data = data;
    node->caller = caller;
    /* Dispatch can free the node as soon as it is queued. Keep reporting/wake
     * values local instead of inspecting the node after releasing the lock. */
    Apk64ALooper *looper = node->looper;
    const pthread_t owner = node->owner;
    const uint64_t due_ns = node->due_ns;

    pthread_mutex_lock(&native_activity_g_choreo_lock);
    if (native_activity_g_choreo_tail) native_activity_g_choreo_tail->next = node; else native_activity_g_choreo_head = node;
    native_activity_g_choreo_tail = node;
    pthread_mutex_unlock(&native_activity_g_choreo_lock);

    /* AChoreographer callbacks are delivered by the owning ALooper. Posting
     * from another thread must make a sleeping pollAll() reconsider its timer. */
    if (looper) ALooper_wake(looper);

    if (atomic_fetch_add(&native_activity_g_choreo_post_reports, 1) < 8u || native_activity_choreographer_full_trace()) {
        APK64_LOGCI("CHOREO", "post owner=%p looper=%p callback=%p data=%p is64=%d delayMs=%u nowNs=%llu dueNs=%llu caller=%p",
                    (void *)(uintptr_t)owner, (void *)looper, callback, data, is64,
                    delay_ms, (unsigned long long)now, (unsigned long long)due_ns, caller);
    }
}

void AChoreographer_postFrameCallback(Apk64AChoreographer *choreographer,
                                      void (*callback)(long, void *), void *data) {
    native_activity_choreographer_post_delayed(choreographer, false, (void *)callback, data, 0,
                              __builtin_extract_return_addr(__builtin_return_address(0)));
}

void AChoreographer_postFrameCallback64(Apk64AChoreographer *choreographer,
                                        void (*callback)(int64_t, void *), void *data) {
    native_activity_choreographer_post_delayed(choreographer, true, (void *)callback, data, 0,
                              __builtin_extract_return_addr(__builtin_return_address(0)));
}

void AChoreographer_postFrameCallbackDelayed(Apk64AChoreographer *choreographer,
                                             void (*callback)(long, void *), void *data,
                                             long delay_millis) {
    const uint32_t delay = delay_millis <= 0 ? 0u :
                           (delay_millis > (long)UINT32_MAX ? UINT32_MAX : (uint32_t)delay_millis);
    native_activity_choreographer_post_delayed(choreographer, false, (void *)callback, data, delay,
                              __builtin_extract_return_addr(__builtin_return_address(0)));
}

void AChoreographer_postFrameCallbackDelayed64(Apk64AChoreographer *choreographer,
                                               void (*callback)(int64_t, void *), void *data,
                                               uint32_t delay_millis) {
    native_activity_choreographer_post_delayed(choreographer, true, (void *)callback, data, delay_millis,
                              __builtin_extract_return_addr(__builtin_return_address(0)));
}

static size_t native_activity_choreographer_dispatch_due(Apk64ALooper *looper, pthread_t self) {
    const uint64_t now = native_activity_choreographer_now_ns();
    native_activity_Apk64ChoreographerCallback *run_head = NULL, *run_tail = NULL;

    pthread_mutex_lock(&native_activity_g_choreo_lock);
    native_activity_Apk64ChoreographerCallback **link = &native_activity_g_choreo_head;
    while (*link) {
        native_activity_Apk64ChoreographerCallback *node = *link;
        const bool owner_match = node->looper ? (node->looper == looper)
                                              : pthread_equal(node->owner, self);
        if (!owner_match || node->due_ns > now) {
            link = &node->next;
            continue;
        }
        *link = node->next;
        node->next = NULL;
        if (run_tail) run_tail->next = node; else run_head = node;
        run_tail = node;
    }
    native_activity_g_choreo_tail = NULL;
    for (native_activity_Apk64ChoreographerCallback *it = native_activity_g_choreo_head; it; it = it->next) native_activity_g_choreo_tail = it;
    pthread_mutex_unlock(&native_activity_g_choreo_lock);

    size_t count = 0;
    const int64_t frame_time = (int64_t)now;
    while (run_head) {
        native_activity_Apk64ChoreographerCallback *next = run_head->next;
        const bool report = atomic_fetch_add(&native_activity_g_choreo_dispatch_reports, 1) < 8u || native_activity_choreographer_full_trace();
        void *callback = run_head->is64 ? (void *)run_head->fn.cb64 : (void *)run_head->fn.legacy;
        if (report)
            APK64_LOGCI("CHOREO", "dispatch BEGIN owner=%p current=%p looper=%p callback=%p data=%p is64=%d frameNs=%lld dueNs=%llu postCaller=%p",
                        (void *)(uintptr_t)run_head->owner, (void *)(uintptr_t)self,
                        (void *)run_head->looper, callback, run_head->data, run_head->is64,
                        (long long)frame_time, (unsigned long long)run_head->due_ns, run_head->caller);
        const uint64_t begin_ns = report ? native_activity_choreographer_now_ns() : 0;
        if (run_head->is64) run_head->fn.cb64(frame_time, run_head->data);
        else run_head->fn.legacy((long)frame_time, run_head->data);
        if (report)
            APK64_LOGCI("CHOREO", "dispatch END callback=%p data=%p frameNs=%lld elapsedNs=%llu",
                        callback, run_head->data, (long long)frame_time,
                        (unsigned long long)(native_activity_choreographer_now_ns() - begin_ns));
        free(run_head);
        run_head = next;
        count++;
    }
    if (count && native_activity_choreographer_full_trace()) {
        APK64_LOGCI("CHOREO", "dispatch looper=%p callbacks=%zu frameNs=%lld",
                    (void *)looper, count, (long long)frame_time);
    }
    return count;
}

static int native_activity_choreographer_wait_ms(Apk64ALooper *looper, pthread_t self) {
    uint64_t next_due = UINT64_MAX;
    pthread_mutex_lock(&native_activity_g_choreo_lock);
    for (native_activity_Apk64ChoreographerCallback *it = native_activity_g_choreo_head; it; it = it->next) {
        const bool owner_match = it->looper ? (it->looper == looper)
                                            : pthread_equal(it->owner, self);
        if (owner_match && it->due_ns < next_due) next_due = it->due_ns;
    }
    pthread_mutex_unlock(&native_activity_g_choreo_lock);
    if (next_due == UINT64_MAX) return -1;

    const uint64_t now = native_activity_choreographer_now_ns();
    if (next_due <= now) return 0;
    const uint64_t delta_ns = next_due - now;
    uint64_t ms = (delta_ns + UINT64_C(999999)) / UINT64_C(1000000);
    if (ms > (uint64_t)INT32_MAX) ms = INT32_MAX;
    return (int)ms;
}

void apk64_choreographer_pump(void) {
    (void)native_activity_choreographer_dispatch_due(native_activity_g_thread_looper, pthread_self());
    const uint64_t now = native_activity_choreographer_now_ns();
    native_activity_Apk64ChoreographerCallback overdue[4];
    size_t count = 0, queued = 0, waiting = 0;
    pthread_mutex_lock(&native_activity_g_choreo_lock);
    if (now >= native_activity_g_choreo_last_queue_report_ns &&
        now - native_activity_g_choreo_last_queue_report_ns >= UINT64_C(5000000000)) {
        native_activity_g_choreo_last_queue_report_ns = now;
        for (native_activity_Apk64ChoreographerCallback *it = native_activity_g_choreo_head; it; it = it->next) {
            ++queued;
            if (now < it->due_ns || now - it->due_ns < UINT64_C(1000000000)) continue;
            ++waiting;
            if (count < sizeof(overdue) / sizeof(overdue[0])) overdue[count++] = *it;
        }
    }
    pthread_mutex_unlock(&native_activity_g_choreo_lock);
    for (size_t i = 0; i < count; ++i) {
        APK64_LOGCW("CHOREO", "queued overdueMs=%llu queued=%zu overdue=%zu owner=%p current=%p looper=%p callback=%p data=%p is64=%d dueNs=%llu nowNs=%llu postCaller=%p",
                    (unsigned long long)((now - overdue[i].due_ns) / UINT64_C(1000000)),
                    queued, waiting, (void *)(uintptr_t)overdue[i].owner,
                    (void *)(uintptr_t)pthread_self(), (void *)overdue[i].looper,
                    overdue[i].is64 ? (void *)overdue[i].fn.cb64 : (void *)overdue[i].fn.legacy,
                    overdue[i].data, overdue[i].is64, (unsigned long long)overdue[i].due_ns,
                    (unsigned long long)now, overdue[i].caller);
    }
}

Apk64AConfiguration *AConfiguration_new(void) {
    return (Apk64AConfiguration *)calloc(1, sizeof(Apk64AConfiguration));
}

void AConfiguration_delete(Apk64AConfiguration *config) { free(config); }

void AConfiguration_copy(Apk64AConfiguration *dest, Apk64AConfiguration *src) {
    if (dest && src) dest->values = src->values;
}

void AConfiguration_fromAssetManager(Apk64AConfiguration *config, Apk64AssetManager *manager) {
    if (!config) return;
    Apk64AssetConfiguration values = {0};
    if (apk64_assets_get_configuration(manager, &values)) config->values = values;
    else memset(&config->values, 0, sizeof(config->values));
}

#define CONFIG_GETTER(name, field) \
    int32_t AConfiguration_get##name(Apk64AConfiguration *config) { return config ? config->values.field : 0; }
#define CONFIG_SETTER(name, field) \
    void AConfiguration_set##name(Apk64AConfiguration *config, int32_t value) { if (config) config->values.field = value; }

CONFIG_GETTER(Mcc, mcc)
CONFIG_SETTER(Mcc, mcc)
CONFIG_GETTER(Mnc, mnc)
CONFIG_SETTER(Mnc, mnc)

void AConfiguration_getLanguage(Apk64AConfiguration *config, char *out) {
    if (!out) return;
    out[0] = config ? config->values.language[0] : 0;
    out[1] = config ? config->values.language[1] : 0;
}
void AConfiguration_setLanguage(Apk64AConfiguration *config, const char *language) {
    if (!config) return;
    config->values.language[0] = language ? language[0] : 0;
    config->values.language[1] = language ? language[1] : 0;
}
void AConfiguration_getCountry(Apk64AConfiguration *config, char *out) {
    if (!out) return;
    out[0] = config ? config->values.country[0] : 0;
    out[1] = config ? config->values.country[1] : 0;
}
void AConfiguration_setCountry(Apk64AConfiguration *config, const char *country) {
    if (!config) return;
    config->values.country[0] = country ? country[0] : 0;
    config->values.country[1] = country ? country[1] : 0;
}

CONFIG_GETTER(Orientation, orientation)
CONFIG_SETTER(Orientation, orientation)
CONFIG_GETTER(Touchscreen, touchscreen)
CONFIG_SETTER(Touchscreen, touchscreen)
CONFIG_GETTER(Density, density)
CONFIG_SETTER(Density, density)
CONFIG_GETTER(Keyboard, keyboard)
CONFIG_SETTER(Keyboard, keyboard)
CONFIG_GETTER(Navigation, navigation)
CONFIG_SETTER(Navigation, navigation)
CONFIG_GETTER(KeysHidden, keys_hidden)
CONFIG_SETTER(KeysHidden, keys_hidden)
CONFIG_GETTER(NavHidden, nav_hidden)
CONFIG_SETTER(NavHidden, nav_hidden)
CONFIG_GETTER(SdkVersion, sdk_version)
CONFIG_SETTER(SdkVersion, sdk_version)
CONFIG_GETTER(ScreenSize, screen_size)
CONFIG_SETTER(ScreenSize, screen_size)
CONFIG_GETTER(ScreenLong, screen_long)
CONFIG_SETTER(ScreenLong, screen_long)
CONFIG_GETTER(UiModeType, ui_mode_type)
CONFIG_SETTER(UiModeType, ui_mode_type)
CONFIG_GETTER(UiModeNight, ui_mode_night)
CONFIG_SETTER(UiModeNight, ui_mode_night)
CONFIG_GETTER(ScreenWidthDp, screen_width_dp)
CONFIG_SETTER(ScreenWidthDp, screen_width_dp)
CONFIG_GETTER(ScreenHeightDp, screen_height_dp)
CONFIG_SETTER(ScreenHeightDp, screen_height_dp)
CONFIG_GETTER(SmallestScreenWidthDp, smallest_screen_width_dp)
CONFIG_SETTER(SmallestScreenWidthDp, smallest_screen_width_dp)
CONFIG_GETTER(LayoutDirection, layout_direction)
CONFIG_SETTER(LayoutDirection, layout_direction)
CONFIG_GETTER(ScreenRound, screen_round)
CONFIG_SETTER(ScreenRound, screen_round)

#undef CONFIG_GETTER
#undef CONFIG_SETTER

Apk64ALooper *ALooper_forThread(void) {
    return native_activity_g_thread_looper;
}

static short native_activity_looper_poll_events(int events) {
    short out = 0;
    if (events & APK64_ALOOPER_EVENT_INPUT) out |= POLLIN;
    if (events & APK64_ALOOPER_EVENT_OUTPUT) out |= POLLOUT;
    return out;
}

static int native_activity_looper_android_events(short revents) {
    int out = 0;
    if (revents & POLLIN) out |= APK64_ALOOPER_EVENT_INPUT;
    if (revents & POLLOUT) out |= APK64_ALOOPER_EVENT_OUTPUT;
    if (revents & POLLERR) out |= APK64_ALOOPER_EVENT_ERROR;
    if (revents & POLLHUP) out |= APK64_ALOOPER_EVENT_HANGUP;
    if (revents & POLLNVAL) out |= APK64_ALOOPER_EVENT_INVALID;
    return out;
}

static void native_activity_looper_drain_wake(Apk64ALooper *looper) {
    if (!looper || looper->wake_read_fd < 0) return;
    unsigned char buffer[64];
    while (apk64_fd_read(looper->wake_read_fd, buffer, sizeof(buffer)) > 0) {}
}

Apk64ALooper *ALooper_prepare(int opts) {
    if (native_activity_g_thread_looper) return native_activity_g_thread_looper;
    if ((opts & ~APK64_ALOOPER_PREPARE_ALLOW_NON_CALLBACKS) != 0) return NULL;
    Apk64ALooper *looper = (Apk64ALooper *)calloc(1, sizeof(*looper));
    if (!looper) return NULL;
    int wake_fds[2] = {-1, -1};
    if (apk64_fd_pipe(wake_fds) != 0) {
        free(looper);
        return NULL;
    }
    if (pthread_mutex_init(&looper->lock, NULL) != 0) {
        (void)apk64_fd_close(wake_fds[0]);
        (void)apk64_fd_close(wake_fds[1]);
        free(looper);
        return NULL;
    }
    looper->opts = opts;
    looper->owner_thread = pthread_self();
    looper->wake_read_fd = wake_fds[0];
    looper->wake_write_fd = wake_fds[1];
    atomic_init(&looper->references, 1);
    native_activity_g_thread_looper = looper;
    return looper;
}

void ALooper_acquire(Apk64ALooper *looper) {
    if (looper) (void)atomic_fetch_add_explicit(&looper->references, 1, memory_order_relaxed);
}

void ALooper_release(Apk64ALooper *looper) {
    if (!looper) return;
    int refs = atomic_load_explicit(&looper->references, memory_order_relaxed);
    while (refs > 1 && !atomic_compare_exchange_weak_explicit(&looper->references, &refs, refs - 1,
                                                               memory_order_release, memory_order_relaxed)) {}
}

void ALooper_wake(Apk64ALooper *looper) {
    if (!looper || looper->wake_write_fd < 0) return;
    const unsigned char token = 1;
    if (apk64_fd_write(looper->wake_write_fd, &token, 1) < 0 && errno != EAGAIN) {
        APK64_LOGCW("NATIVE", "ALooper wake write failed errno=%d", errno);
    }
}

static void native_activity_looper_wake_if_cross_thread(Apk64ALooper *looper) {
    if (looper && !pthread_equal(pthread_self(), looper->owner_thread)) ALooper_wake(looper);
}

int ALooper_addFd(Apk64ALooper *looper, int fd, int ident, int events,
                  int (*callback)(int fd, int events, void *data), void *data) {
    const int allowed_events = APK64_ALOOPER_EVENT_INPUT | APK64_ALOOPER_EVENT_OUTPUT;
    if (!looper || fd < 0 || !(events & allowed_events) || (events & ~allowed_events)) return -1;
    if (!callback && (!(looper->opts & APK64_ALOOPER_PREPARE_ALLOW_NON_CALLBACKS) || ident < 0)) return -1;

    pthread_mutex_lock(&looper->lock);
    for (size_t i = 0; i < looper->fd_count; ++i) {
        if (looper->fds[i].fd == fd) {
            looper->fds[i] = (native_activity_Apk64LooperFd){fd, ident, events, callback, data};
            pthread_mutex_unlock(&looper->lock);
            native_activity_looper_wake_if_cross_thread(looper);
            return 1;
        }
    }
    if (looper->fd_count == looper->fd_capacity) {
        size_t next_capacity = looper->fd_capacity ? looper->fd_capacity * 2u : 8u;
        native_activity_Apk64LooperFd *grown = (native_activity_Apk64LooperFd *)realloc(looper->fds, next_capacity * sizeof(*grown));
        if (!grown) {
            pthread_mutex_unlock(&looper->lock);
            return -1;
        }
        looper->fds = grown;
        looper->fd_capacity = next_capacity;
    }
    looper->fds[looper->fd_count++] = (native_activity_Apk64LooperFd){fd, ident, events, callback, data};
    pthread_mutex_unlock(&looper->lock);
    native_activity_looper_wake_if_cross_thread(looper);
    return 1;
}

int ALooper_removeFd(Apk64ALooper *looper, int fd) {
    if (!looper || fd < 0) return 0;
    int removed = 0;
    pthread_mutex_lock(&looper->lock);
    for (size_t i = 0; i < looper->fd_count; ++i) {
        if (looper->fds[i].fd != fd) continue;
        memmove(&looper->fds[i], &looper->fds[i + 1],
                (looper->fd_count - i - 1u) * sizeof(looper->fds[0]));
        looper->fd_count--;
        removed = 1;
        break;
    }
    pthread_mutex_unlock(&looper->lock);
    if (removed) native_activity_looper_wake_if_cross_thread(looper);
    return removed;
}

static int native_activity_poll_input_queue(int *out_events, void **out_data) {
    Apk64AInputQueue *queue = &native_activity_g_native.input_queue;
    if (queue->looper == native_activity_g_thread_looper && native_activity_queue_has_events(queue)) {
        if (out_events) *out_events = APK64_ALOOPER_EVENT_INPUT;
        if (out_data) *out_data = queue->data;
        return queue->ident;
    }
    return APK64_ALOOPER_POLL_TIMEOUT;
}

static int native_activity_looper_process_ready(Apk64ALooper *looper, const native_activity_Apk64LooperFd *snapshot,
                                const short *revents, size_t count,
                                int *out_fd, int *out_events, void **out_data) {
    bool callback_called = false;
    int ready_ident = APK64_ALOOPER_POLL_TIMEOUT;
    int ready_fd = -1;
    int ready_events = 0;
    void *ready_data = NULL;

    for (size_t i = 0; i < count; ++i) {
        if (!revents[i]) continue;
        const int events = native_activity_looper_android_events(revents[i]);
        if (!events) continue;
        if (snapshot[i].callback) {
            const int keep = snapshot[i].callback(snapshot[i].fd, events, snapshot[i].data);
            callback_called = true;
            if (!keep) (void)ALooper_removeFd(looper, snapshot[i].fd);
        } else if (ready_ident == APK64_ALOOPER_POLL_TIMEOUT) {
            ready_ident = snapshot[i].ident;
            ready_fd = snapshot[i].fd;
            ready_events = events;
            ready_data = snapshot[i].data;
        }
    }

    if (ready_ident >= 0) {
        if (out_fd) *out_fd = ready_fd;
        if (out_events) *out_events = ready_events;
        if (out_data) *out_data = ready_data;
        return ready_ident;
    }
    return callback_called ? APK64_ALOOPER_POLL_CALLBACK : APK64_ALOOPER_POLL_TIMEOUT;
}

/* The looper's host waits are signal safe points (apk64_signal_safe_call):
 * a guest thread parked in ALooper_pollOnce still takes thread-directed
 * signals. No guest callback runs inside them. */
typedef struct native_activity_LooperHostWait {
    struct pollfd *fds;
    nfds_t count;
    int timeout_ms;
    struct timespec *sleep;
} native_activity_LooperHostWait;

static long native_activity_looper_host_wait_run(void *ctx) {
    native_activity_LooperHostWait *wait = (native_activity_LooperHostWait *)ctx;
    if (wait->fds) return poll(wait->fds, wait->count, wait->timeout_ms);
    return nanosleep(wait->sleep, wait->sleep);
}

int ALooper_pollOnce(int timeout_millis, int *out_fd, int *out_events, void **out_data) {
    Apk64ALooper *looper = native_activity_g_thread_looper;
    if (out_fd) *out_fd = -1;
    if (out_events) *out_events = 0;
    if (out_data) *out_data = NULL;
    if (!looper) return APK64_ALOOPER_POLL_ERROR;

    const bool wait_forever = timeout_millis < 0;
    int remaining_ms = timeout_millis;

    for (;;) {
        if (native_activity_choreographer_dispatch_due(looper, pthread_self()) > 0)
            return APK64_ALOOPER_POLL_CALLBACK;

        int input_ready = native_activity_poll_input_queue(out_events, out_data);
        if (input_ready != APK64_ALOOPER_POLL_TIMEOUT) return input_ready;

        pthread_mutex_lock(&looper->lock);
        const size_t count = looper->fd_count;
        native_activity_Apk64LooperFd *snapshot = count ? (native_activity_Apk64LooperFd *)malloc(count * sizeof(*snapshot)) : NULL;
        if (count && !snapshot) {
            pthread_mutex_unlock(&looper->lock);
            return APK64_ALOOPER_POLL_ERROR;
        }
        if (count) memcpy(snapshot, looper->fds, count * sizeof(*snapshot));
        pthread_mutex_unlock(&looper->lock);

        short *revents = count ? (short *)calloc(count, sizeof(*revents)) : NULL;
        struct pollfd *host = count ? (struct pollfd *)calloc(count, sizeof(*host)) : NULL;
        size_t *host_map = count ? (size_t *)calloc(count, sizeof(*host_map)) : NULL;
        if (count && (!revents || !host || !host_map)) {
            free(snapshot); free(revents); free(host); free(host_map);
            return APK64_ALOOPER_POLL_ERROR;
        }

        bool immediate = false;
        size_t host_count = 0;
        for (size_t i = 0; i < count; ++i) {
            const short requested = native_activity_looper_poll_events(snapshot[i].events);
            if (apk64_fd_is_virtual(snapshot[i].fd)) {
                revents[i] = apk64_fd_poll_revents(snapshot[i].fd, requested);
                if (revents[i]) immediate = true;
            } else {
                host[host_count].fd = snapshot[i].fd;
                host[host_count].events = requested;
                host_map[host_count++] = i;
            }
        }

        const bool wake_ready = (apk64_fd_poll_revents(looper->wake_read_fd, POLLIN) & POLLIN) != 0;
        if (wake_ready) immediate = true;

        int slice_ms = 0;
        if (!immediate && timeout_millis != 0) {
            slice_ms = wait_forever ? 8 : (remaining_ms < 8 ? remaining_ms : 8);
            const int choreo_ms = native_activity_choreographer_wait_ms(looper, pthread_self());
            if (choreo_ms >= 0 && choreo_ms < slice_ms) slice_ms = choreo_ms;
            if (slice_ms < 0) slice_ms = 0;
        }

        if (host_count > 0) {
            int rc;
            native_activity_LooperHostWait wait = { host, (nfds_t)host_count, immediate ? 0 : slice_ms, NULL };
            do { rc = (int)apk64_signal_safe_call(native_activity_looper_host_wait_run, &wait); } while (rc < 0 && errno == EINTR);
            if (rc < 0) {
                free(snapshot); free(revents); free(host); free(host_map);
                return APK64_ALOOPER_POLL_ERROR;
            }
            for (size_t i = 0; i < host_count; ++i) revents[host_map[i]] = host[i].revents;
        } else if (!immediate && slice_ms > 0) {
            struct timespec ts = {.tv_sec = slice_ms / 1000,
                                  .tv_nsec = (long)(slice_ms % 1000) * 1000000L};
            native_activity_LooperHostWait wait = { NULL, 0, 0, &ts };
            while (apk64_signal_safe_call(native_activity_looper_host_wait_run, &wait) != 0 && errno == EINTR) {}
        }

        const int ready = native_activity_looper_process_ready(looper, snapshot, revents, count,
                                               out_fd, out_events, out_data);
        free(snapshot); free(revents); free(host); free(host_map);
        if (ready != APK64_ALOOPER_POLL_TIMEOUT) return ready;

        if (wake_ready || (apk64_fd_poll_revents(looper->wake_read_fd, POLLIN) & POLLIN)) {
            native_activity_looper_drain_wake(looper);
            return APK64_ALOOPER_POLL_WAKE;
        }
        if (timeout_millis == 0) return APK64_ALOOPER_POLL_TIMEOUT;
        if (!wait_forever) {
            remaining_ms -= slice_ms;
            if (remaining_ms <= 0) return APK64_ALOOPER_POLL_TIMEOUT;
        }
    }
}

int ALooper_pollAll(int timeout_millis, int *out_fd, int *out_events, void **out_data) {
    int result;
    do {
        result = ALooper_pollOnce(timeout_millis, out_fd, out_events, out_data);
        if (result == APK64_ALOOPER_POLL_CALLBACK) timeout_millis = 0;
    } while (result == APK64_ALOOPER_POLL_CALLBACK);
    return result;
}

void AInputQueue_attachLooper(Apk64AInputQueue *queue, Apk64ALooper *looper, int ident,
                              int (*callback)(int fd, int events, void *data), void *data) {
    if (!queue) return;
    pthread_mutex_lock(&queue->lock);
    queue->looper = looper; queue->ident = ident; queue->callback = callback; queue->data = data;
    pthread_mutex_unlock(&queue->lock);
    native_activity_queue_wake(queue);
}
void AInputQueue_detachLooper(Apk64AInputQueue *queue) {
    if (!queue) return;
    pthread_mutex_lock(&queue->lock);
    queue->looper = NULL; queue->ident = 0; queue->callback = NULL; queue->data = NULL;
    pthread_mutex_unlock(&queue->lock);
}
int32_t AInputQueue_getEvent(Apk64AInputQueue *queue, Apk64AInputEvent **out_event) {
    if (!queue || !out_event) return -1;
    pthread_mutex_lock(&queue->lock);
    if (queue->current_valid || queue->count == 0) {
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    queue->current = queue->events[queue->head];
    queue->head = (queue->head + 1) % APK64_INPUT_QUEUE_CAP;
    queue->count--;
    queue->current_valid = true;
    *out_event = &queue->current;
    pthread_mutex_unlock(&queue->lock);
    return 0;
}
int32_t AInputQueue_preDispatchEvent(Apk64AInputQueue *queue, Apk64AInputEvent *event) { (void)queue; (void)event; return 0; }
void AInputQueue_finishEvent(Apk64AInputQueue *queue, Apk64AInputEvent *event, int handled) {
    (void)event; (void)handled;
    if (!queue) return;
    pthread_mutex_lock(&queue->lock); queue->current_valid = false; pthread_mutex_unlock(&queue->lock);
}
int32_t AInputEvent_getType(const Apk64AInputEvent *event) { return event ? event->type : 0; }
int32_t AKeyEvent_getAction(const Apk64AInputEvent *event) { return event && event->type == APK64_AINPUT_EVENT_TYPE_KEY ? event->action : 0; }
int32_t AKeyEvent_getKeyCode(const Apk64AInputEvent *event) { return event && event->type == APK64_AINPUT_EVENT_TYPE_KEY ? event->key_code : 0; }
int32_t AMotionEvent_getAction(const Apk64AInputEvent *event) { return event ? event->action : 0; }
size_t AMotionEvent_getPointerCount(const Apk64AInputEvent *event) { return event ? event->pointer_count : 0; }
int32_t AMotionEvent_getPointerId(const Apk64AInputEvent *event, size_t i) { return event && i<event->pointer_count ? event->ids[i] : -1; }
float AMotionEvent_getX(const Apk64AInputEvent *event, size_t i) { return event && i<event->pointer_count ? event->xs[i] : 0.0f; }
float AMotionEvent_getY(const Apk64AInputEvent *event, size_t i) { return event && i<event->pointer_count ? event->ys[i] : 0.0f; }

#pragma pop_macro("CONFIG_SETTER")
#pragma pop_macro("CONFIG_GETTER")
#pragma pop_macro("APK64_INPUT_QUEUE_CAP")
#pragma pop_macro("APK64_INPUT_MAX_POINTERS")
#pragma pop_macro("APK64_CHOREO_FRAME_NS")
#pragma pop_macro("APK64_AMOTION_UP")
#pragma pop_macro("APK64_AMOTION_MOVE")
#pragma pop_macro("APK64_AMOTION_DOWN")
#pragma pop_macro("APK64_ALOOPER_PREPARE_ALLOW_NON_CALLBACKS")
#pragma pop_macro("APK64_ALOOPER_POLL_WAKE")
#pragma pop_macro("APK64_ALOOPER_POLL_TIMEOUT")
#pragma pop_macro("APK64_ALOOPER_POLL_ERROR")
#pragma pop_macro("APK64_ALOOPER_POLL_CALLBACK")
#pragma pop_macro("APK64_ALOOPER_EVENT_OUTPUT")
#pragma pop_macro("APK64_ALOOPER_EVENT_INVALID")
#pragma pop_macro("APK64_ALOOPER_EVENT_INPUT")
#pragma pop_macro("APK64_ALOOPER_EVENT_HANGUP")
#pragma pop_macro("APK64_ALOOPER_EVENT_ERROR")
#pragma pop_macro("APK64_AKEY_UP")
#pragma pop_macro("APK64_AKEY_DOWN")
#pragma pop_macro("APK64_AINPUT_EVENT_TYPE_MOTION")
#pragma pop_macro("APK64_AINPUT_EVENT_TYPE_KEY")


/* media_ndk */
#pragma push_macro("APK64_MEDIA_IO_BUFFER")


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>


typedef enum media_ndk_Apk64MediaValueType {
    APK64_MEDIA_VALUE_INT32 = 1,
    APK64_MEDIA_VALUE_INT64 = 2,
    APK64_MEDIA_VALUE_FLOAT = 3,
    APK64_MEDIA_VALUE_STRING = 4,
    APK64_MEDIA_VALUE_BUFFER = 5,
} media_ndk_Apk64MediaValueType;

typedef struct media_ndk_Apk64MediaEntry {
    char *name;
    media_ndk_Apk64MediaValueType type;
    union {
        int32_t i32;
        int64_t i64;
        float f;
        char *s;
        struct { void *data; size_t size; } buffer;
    } value;
    struct media_ndk_Apk64MediaEntry *next;
} media_ndk_Apk64MediaEntry;

struct Apk64AMediaFormat {
    media_ndk_Apk64MediaEntry *entries;
    char *debug;
};

const char *apk64_AMEDIAFORMAT_KEY_BIT_RATE = "bitrate";
const char *apk64_AMEDIAFORMAT_KEY_I_FRAME_INTERVAL = "i-frame-interval";
const char *apk64_AMEDIAFORMAT_KEY_CHANNEL_COUNT = "channel-count";
const char *apk64_AMEDIAFORMAT_KEY_COLOR_FORMAT = "color-format";
const char *apk64_AMEDIAFORMAT_KEY_COLOR_RANGE = "color-range";
const char *apk64_AMEDIAFORMAT_KEY_COLOR_STANDARD = "color-standard";
const char *apk64_AMEDIAFORMAT_KEY_DURATION = "durationUs";
const char *apk64_AMEDIAFORMAT_KEY_ENCODER_DELAY = "encoder-delay";
const char *apk64_AMEDIAFORMAT_KEY_FRAME_RATE = "frame-rate";
const char *apk64_AMEDIAFORMAT_KEY_HEIGHT = "height";
const char *apk64_AMEDIAFORMAT_KEY_LANGUAGE = "language";
const char *apk64_AMEDIAFORMAT_KEY_MIME = "mime";
const char *apk64_AMEDIAFORMAT_KEY_ROTATION = "rotation-degrees";
const char *apk64_AMEDIAFORMAT_KEY_SAMPLE_RATE = "sample-rate";
const char *apk64_AMEDIAFORMAT_KEY_SLICE_HEIGHT = "slice-height";
const char *apk64_AMEDIAFORMAT_KEY_STRIDE = "stride";
const char *apk64_AMEDIAFORMAT_KEY_WIDTH = "width";

static media_ndk_Apk64MediaEntry *media_ndk_find_entry(Apk64AMediaFormat *format, const char *name) {
    if (!format || !name) return NULL;
    for (media_ndk_Apk64MediaEntry *it = format->entries; it; it = it->next)
        if (strcmp(it->name, name) == 0) return it;
    return NULL;
}

static void media_ndk_clear_value(media_ndk_Apk64MediaEntry *entry) {
    if (entry && entry->type == APK64_MEDIA_VALUE_STRING) free(entry->value.s);
    if (entry && entry->type == APK64_MEDIA_VALUE_BUFFER) free(entry->value.buffer.data);
}

static media_ndk_Apk64MediaEntry *media_ndk_ensure_entry(Apk64AMediaFormat *format, const char *name,
                                     media_ndk_Apk64MediaValueType type) {
    if (!format || !name) return NULL;
    media_ndk_Apk64MediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry) {
        entry = (media_ndk_Apk64MediaEntry *)calloc(1, sizeof(*entry));
        if (!entry) return NULL;
        entry->name = strdup(name);
        if (!entry->name) { free(entry); return NULL; }
        entry->next = format->entries;
        format->entries = entry;
    } else {
        media_ndk_clear_value(entry);
        memset(&entry->value, 0, sizeof(entry->value));
    }
    entry->type = type;
    return entry;
}

Apk64AMediaFormat *apk64_AMediaFormat_new(void) {
    return (Apk64AMediaFormat *)calloc(1, sizeof(Apk64AMediaFormat));
}

int apk64_AMediaFormat_delete(Apk64AMediaFormat *format) {
    if (!format) return 0;
    media_ndk_Apk64MediaEntry *entry = format->entries;
    while (entry) {
        media_ndk_Apk64MediaEntry *next = entry->next;
        media_ndk_clear_value(entry);
        free(entry->name);
        free(entry);
        entry = next;
    }
    free(format->debug);
    free(format);
    return 0; /* AMEDIA_OK */
}

bool apk64_AMediaFormat_getInt32(Apk64AMediaFormat *format, const char *name, int32_t *out) {
    media_ndk_Apk64MediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != APK64_MEDIA_VALUE_INT32 || !out) return false;
    *out = entry->value.i32;
    return true;
}

bool apk64_AMediaFormat_getInt64(Apk64AMediaFormat *format, const char *name, int64_t *out) {
    media_ndk_Apk64MediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != APK64_MEDIA_VALUE_INT64 || !out) return false;
    *out = entry->value.i64;
    return true;
}

bool apk64_AMediaFormat_getFloat(Apk64AMediaFormat *format, const char *name, float *out) {
    media_ndk_Apk64MediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != APK64_MEDIA_VALUE_FLOAT || !out) return false;
    *out = entry->value.f;
    return true;
}

bool apk64_AMediaFormat_getString(Apk64AMediaFormat *format, const char *name, const char **out) {
    if (!format || !out) return false;
    media_ndk_Apk64MediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != APK64_MEDIA_VALUE_STRING) return false;
    *out = entry->value.s ? entry->value.s : "";
    return true;
}

void apk64_AMediaFormat_setInt32(Apk64AMediaFormat *format, const char *name, int32_t value) {
    media_ndk_Apk64MediaEntry *entry = media_ndk_ensure_entry(format, name, APK64_MEDIA_VALUE_INT32);
    if (entry) entry->value.i32 = value;
}

void apk64_AMediaFormat_setInt64(Apk64AMediaFormat *format, const char *name, int64_t value) {
    media_ndk_Apk64MediaEntry *entry = media_ndk_ensure_entry(format, name, APK64_MEDIA_VALUE_INT64);
    if (entry) entry->value.i64 = value;
}

void apk64_AMediaFormat_setFloat(Apk64AMediaFormat *format, const char *name, float value) {
    media_ndk_Apk64MediaEntry *entry = media_ndk_ensure_entry(format, name, APK64_MEDIA_VALUE_FLOAT);
    if (entry) entry->value.f = value;
}

void apk64_AMediaFormat_setString(Apk64AMediaFormat *format, const char *name, const char *value) {
    if (!format || !name || !value) return;
    /* getString may supply the old value: copy before replacing its entry. */
    char *copy = strdup(value);
    if (!copy) return;
    media_ndk_Apk64MediaEntry *entry = media_ndk_ensure_entry(format, name, APK64_MEDIA_VALUE_STRING);
    if (!entry) { free(copy); return; }
    entry->value.s = copy;
}

bool apk64_AMediaFormat_getBuffer(Apk64AMediaFormat *format, const char *name, void **data, size_t *size) {
    media_ndk_Apk64MediaEntry *entry = media_ndk_find_entry(format, name);
    if (!entry || entry->type != APK64_MEDIA_VALUE_BUFFER || !data || !size) return false;
    *data = entry->value.buffer.data;
    *size = entry->value.buffer.size;
    return true;
}

void apk64_AMediaFormat_setBuffer(Apk64AMediaFormat *format, const char *name, const void *data, size_t size) {
    if (!format || !name || (!data && size)) return;
    void *copy = malloc(size ? size : 1);
    if (!copy) return;
    if (size) memcpy(copy, data, size);
    media_ndk_Apk64MediaEntry *entry = media_ndk_ensure_entry(format, name, APK64_MEDIA_VALUE_BUFFER);
    if (!entry) { free(copy); return; }
    entry->value.buffer.data = copy;
    entry->value.buffer.size = size;
}

static int media_ndk_media_entry_text(char *out, size_t cap, const media_ndk_Apk64MediaEntry *entry) {
    switch (entry->type) {
    case APK64_MEDIA_VALUE_INT32: return snprintf(out, cap, "%s: int32(%" PRId32 ")", entry->name, entry->value.i32);
    case APK64_MEDIA_VALUE_INT64: return snprintf(out, cap, "%s: int64(%" PRId64 ")", entry->name, entry->value.i64);
    case APK64_MEDIA_VALUE_FLOAT: return snprintf(out, cap, "%s: float(%f)", entry->name, entry->value.f);
    case APK64_MEDIA_VALUE_STRING: return snprintf(out, cap, "%s: string(%s)", entry->name, entry->value.s);
    case APK64_MEDIA_VALUE_BUFFER: return snprintf(out, cap, "%s: data", entry->name);
    default: return snprintf(out, cap, "%s: unknown", entry->name);
    }
}

const char *apk64_AMediaFormat_toString(Apk64AMediaFormat *format) {
    if (!format) return NULL;
    size_t size = 3;
    for (media_ndk_Apk64MediaEntry *e = format->entries; e; e = e->next) {
        int n = media_ndk_media_entry_text(NULL, 0, e);
        if (n < 0 || size > SIZE_MAX - 2 || (size_t)n > SIZE_MAX - size - 2) return NULL;
        size += (size_t)n + 2;
    }
    char *text = malloc(size);
    if (!text) return NULL;
    size_t pos = 0;
    text[pos++] = '{';
    for (media_ndk_Apk64MediaEntry *e = format->entries; e; e = e->next) {
        if (pos > 1) { text[pos++] = ','; text[pos++] = ' '; }
        pos += (size_t)media_ndk_media_entry_text(text + pos, size - pos, e);
    }
    text[pos++] = '}'; text[pos] = 0;
    free(format->debug);
    format->debug = text;
    return text;
}

Apk64AMediaCodec *apk64_AMediaCodec_createEncoderByType(const char *mime_type) {
    APK64_LOGCW("MEDIA", "encoder unavailable mime=%s", mime_type ? mime_type : "<null>");
    return NULL;
}

/* -------------------------------------------------------------------------
 * Minimal real NDK media backend.
 *
 * APK64 currently has a real PCM/WAV path but no H.264/AAC/VPx decoder. The
 * NDK surface below therefore implements WAV extraction and an audio/raw
 * pass-through decoder exactly enough to be useful, while unsupported codecs
 * fail with normal NDK error/null results. This intentionally avoids fake
 * success for video or compressed codecs APK64 cannot decode yet.
 * ------------------------------------------------------------------------- */


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define APK64_MEDIA_IO_BUFFER (64u * 1024u)

typedef struct media_ndk_Apk64WavInfo {
    uint16_t format_tag;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    int64_t data_offset;
    int64_t data_length;
} media_ndk_Apk64WavInfo;

struct Apk64AMediaDataSource {
    void *userdata;
    Apk64AMediaDataSourceReadAt read_at;
    Apk64AMediaDataSourceGetSize get_size;
    Apk64AMediaDataSourceClose close_cb;
};

struct Apk64AMediaExtractor {
    int fd;
    bool owns_fd;
    int64_t source_offset;
    int64_t source_length;
    Apk64AMediaDataSource *custom_source;
    media_ndk_Apk64WavInfo wav;
    bool source_ready;
    bool track_selected;
    int64_t sample_pos;
};

struct Apk64AMediaCodec {
    bool configured;
    bool started;
    bool input_dequeued;
    bool output_ready;
    bool format_change_pending;
    int32_t sample_rate;
    int32_t channel_count;
    uint8_t input[APK64_MEDIA_IO_BUFFER];
    uint8_t output[APK64_MEDIA_IO_BUFFER];
    size_t output_size;
    int64_t output_pts;
    uint32_t output_flags;
};

static uint16_t media_ndk_rd16le(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}
static uint32_t media_ndk_rd32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static ssize_t media_ndk_read_at_fd(int fd, int64_t offset, void *dst, size_t bytes) {
    if (fd < 0 || offset < 0 || (!dst && bytes)) { errno = EINVAL; return -1; }
    off_t saved = lseek(fd, 0, SEEK_CUR);
    if (saved < 0) return -1;
    if (lseek(fd, (off_t)offset, SEEK_SET) < 0) return -1;
    ssize_t rc = read(fd, dst, bytes);
    int saved_errno = errno;
    (void)lseek(fd, saved, SEEK_SET);
    errno = saved_errno;
    return rc;
}

typedef ssize_t (*media_ndk_Apk64MediaReadAt)(void *ctx, int64_t offset, void *dst, size_t bytes);

static ssize_t media_ndk_fd_reader(void *ctx, int64_t offset, void *dst, size_t bytes) {
    return media_ndk_read_at_fd((int)(intptr_t)ctx, offset, dst, bytes);
}

static ssize_t media_ndk_custom_reader(void *ctx, int64_t offset, void *dst, size_t bytes) {
    Apk64AMediaDataSource *source = (Apk64AMediaDataSource *)ctx;
    if (!source || !source->read_at || !source->userdata || offset < 0) return -1;
    return source->read_at(source->userdata, offset, dst, bytes);
}

static bool media_ndk_parse_wav_reader(media_ndk_Apk64MediaReadAt reader, void *ctx, int64_t base, int64_t length, media_ndk_Apk64WavInfo *out) {
    if (!out || !reader || base < 0 || length < 12) return false;
    uint8_t header[12];
    if (reader(ctx, base, header, sizeof(header)) != (ssize_t)sizeof(header)) return false;
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) return false;

    bool have_fmt = false, have_data = false;
    media_ndk_Apk64WavInfo info;
    memset(&info, 0, sizeof(info));
    int64_t pos = 12;
    while (pos + 8 <= length) {
        uint8_t chunk[8];
        if (reader(ctx, base + pos, chunk, sizeof(chunk)) != (ssize_t)sizeof(chunk)) return false;
        const uint32_t size = media_ndk_rd32le(chunk + 4);
        const int64_t payload = pos + 8;
        if ((uint64_t)size > (uint64_t)(length - payload)) return false;
        if (memcmp(chunk, "fmt ", 4) == 0) {
            uint8_t fmt[40];
            const size_t want = size < sizeof(fmt) ? size : sizeof(fmt);
            if (want < 16 || reader(ctx, base + payload, fmt, want) != (ssize_t)want) return false;
            info.format_tag = media_ndk_rd16le(fmt + 0);
            info.channels = media_ndk_rd16le(fmt + 2);
            info.sample_rate = media_ndk_rd32le(fmt + 4);
            info.byte_rate = media_ndk_rd32le(fmt + 8);
            info.block_align = media_ndk_rd16le(fmt + 12);
            info.bits_per_sample = media_ndk_rd16le(fmt + 14);
            have_fmt = true;
        } else if (memcmp(chunk, "data", 4) == 0) {
            info.data_offset = payload;
            info.data_length = size;
            have_data = true;
        }
        pos = payload + (int64_t)size + (size & 1u);
        if (have_fmt && have_data) break;
    }
    if (!have_fmt || !have_data || info.format_tag != 1 || info.channels == 0 ||
        info.sample_rate == 0 || info.block_align == 0 || info.bits_per_sample == 0) return false;
    if (info.data_offset < 0 || info.data_length < 0 || info.data_offset + info.data_length > length) return false;
    *out = info;
    return true;
}

static bool media_ndk_parse_wav(int fd, int64_t base, int64_t length, media_ndk_Apk64WavInfo *out) {
    return media_ndk_parse_wav_reader(media_ndk_fd_reader, (void *)(intptr_t)fd, base, length, out);
}

static void media_ndk_extractor_clear(Apk64AMediaExtractor *ex) {
    if (!ex) return;
    if (ex->owns_fd && ex->fd >= 0) close(ex->fd);
    ex->fd = -1;
    ex->owns_fd = false;
    ex->source_offset = ex->source_length = 0;
    ex->custom_source = NULL;
    memset(&ex->wav, 0, sizeof(ex->wav));
    ex->source_ready = false;
    ex->track_selected = false;
    ex->sample_pos = 0;
}

Apk64AMediaDataSource *apk64_AMediaDataSource_new(void) {
    return (Apk64AMediaDataSource *)calloc(1, sizeof(Apk64AMediaDataSource));
}

void apk64_AMediaDataSource_delete(Apk64AMediaDataSource *source) {
    free(source);
}

void apk64_AMediaDataSource_setUserdata(Apk64AMediaDataSource *source, void *userdata) {
    if (source) source->userdata = userdata;
}

void apk64_AMediaDataSource_setReadAt(Apk64AMediaDataSource *source, Apk64AMediaDataSourceReadAt read_at) {
    if (source) source->read_at = read_at;
}

void apk64_AMediaDataSource_setGetSize(Apk64AMediaDataSource *source, Apk64AMediaDataSourceGetSize get_size) {
    if (source) source->get_size = get_size;
}

void apk64_AMediaDataSource_setClose(Apk64AMediaDataSource *source, Apk64AMediaDataSourceClose close_cb) {
    if (source) source->close_cb = close_cb;
}

Apk64AMediaExtractor *apk64_AMediaExtractor_new(void) {
    Apk64AMediaExtractor *ex = (Apk64AMediaExtractor *)calloc(1, sizeof(*ex));
    if (ex) ex->fd = -1;
    return ex;
}

int apk64_AMediaExtractor_delete(Apk64AMediaExtractor *ex) {
    if (!ex) return APK64_AMEDIA_OK;
    media_ndk_extractor_clear(ex);
    free(ex);
    return APK64_AMEDIA_OK;
}

int apk64_AMediaExtractor_setDataSourceFd(Apk64AMediaExtractor *ex, int fd, int64_t offset, int64_t length) {
    if (!ex || fd < 0 || offset < 0 || length < 0) return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < 0 || offset > (int64_t)st.st_size)
        return APK64_AMEDIA_ERROR_IO;
    int64_t available = (int64_t)st.st_size - offset;
    if (length > available) length = available;
#ifdef F_DUPFD
    int owned = fcntl(fd, F_DUPFD, 0);
#else
    int owned = -1;
    errno = ENOTSUP;
#endif
    if (owned < 0) return APK64_AMEDIA_ERROR_IO;
    media_ndk_Apk64WavInfo wav;
    if (!media_ndk_parse_wav(owned, offset, length, &wav)) {
        close(owned);
        return APK64_AMEDIA_ERROR_UNSUPPORTED;
    }
    media_ndk_extractor_clear(ex);
    ex->fd = owned;
    ex->owns_fd = true;
    ex->source_offset = offset;
    ex->source_length = length;
    ex->wav = wav;
    ex->source_ready = true;
    return APK64_AMEDIA_OK;
}

int apk64_AMediaExtractor_setDataSource(Apk64AMediaExtractor *ex, const char *location) {
    if (!ex || !location) return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    const char *path = strncmp(location, "file://", 7) == 0 ? location + 7 : location;
    int fd = apk64_compat_open_guest_readonly(path);
    if (fd < 0) return APK64_AMEDIA_ERROR_IO;
    struct stat st;
    int status = APK64_AMEDIA_ERROR_IO;
    if (fstat(fd, &st) == 0 && st.st_size >= 0)
        status = apk64_AMediaExtractor_setDataSourceFd(ex, fd, 0, (int64_t)st.st_size);
    close(fd);
    return status;
}

int apk64_AMediaExtractor_setDataSourceCustom(Apk64AMediaExtractor *ex, Apk64AMediaDataSource *source) {
    if (!ex || !source || !source->userdata || !source->read_at || !source->get_size)
        return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    const ssize_t size = source->get_size(source->userdata);
    if (size < 12) return size < 0 ? APK64_AMEDIA_ERROR_UNSUPPORTED : APK64_AMEDIA_ERROR_MALFORMED;
    media_ndk_Apk64WavInfo wav;
    if (!media_ndk_parse_wav_reader(media_ndk_custom_reader, source, 0, (int64_t)size, &wav))
        return APK64_AMEDIA_ERROR_UNSUPPORTED;
    media_ndk_extractor_clear(ex);
    ex->custom_source = source;
    ex->source_offset = 0;
    ex->source_length = (int64_t)size;
    ex->wav = wav;
    ex->source_ready = true;
    return APK64_AMEDIA_OK;
}

size_t apk64_AMediaExtractor_getTrackCount(Apk64AMediaExtractor *ex) {
    return ex && ex->source_ready ? 1u : 0u;
}

Apk64AMediaFormat *apk64_AMediaExtractor_getTrackFormat(Apk64AMediaExtractor *ex, size_t idx) {
    if (!ex || !ex->source_ready || idx != 0) return NULL;
    Apk64AMediaFormat *fmt = apk64_AMediaFormat_new();
    if (!fmt) return NULL;
    apk64_AMediaFormat_setString(fmt, "mime", "audio/raw");
    apk64_AMediaFormat_setInt32(fmt, "sample-rate", (int32_t)ex->wav.sample_rate);
    apk64_AMediaFormat_setInt32(fmt, "channel-count", (int32_t)ex->wav.channels);
    if (ex->wav.byte_rate) {
        int64_t duration = (ex->wav.data_length * INT64_C(1000000)) / ex->wav.byte_rate;
        apk64_AMediaFormat_setInt64(fmt, "durationUs", duration);
    }
    return fmt;
}

int apk64_AMediaExtractor_selectTrack(Apk64AMediaExtractor *ex, size_t idx) {
    if (!ex || !ex->source_ready) return APK64_AMEDIA_ERROR_INVALID_OBJECT;
    if (idx != 0) return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    ex->track_selected = true;
    return APK64_AMEDIA_OK;
}

static size_t media_ndk_extractor_current_size(const Apk64AMediaExtractor *ex) {
    if (!ex || !ex->source_ready || !ex->track_selected || ex->sample_pos >= ex->wav.data_length) return 0;
    int64_t remain = ex->wav.data_length - ex->sample_pos;
    size_t amount = remain > 32768 ? 32768u : (size_t)remain;
    if (ex->wav.block_align > 1 && amount > ex->wav.block_align) {
        amount -= amount % ex->wav.block_align;
    }
    return amount;
}

ssize_t apk64_AMediaExtractor_readSampleData(Apk64AMediaExtractor *ex, uint8_t *buffer, size_t capacity) {
    if (!ex || !buffer || !capacity) return -1;
    size_t sample = media_ndk_extractor_current_size(ex);
    if (!sample) return -1;
    if (sample > capacity) sample = capacity - (capacity % ex->wav.block_align);
    if (!sample) return 0;
    const int64_t offset = ex->source_offset + ex->wav.data_offset + ex->sample_pos;
    ssize_t rc = ex->custom_source
        ? media_ndk_custom_reader(ex->custom_source, offset, buffer, sample)
        : media_ndk_read_at_fd(ex->fd, offset, buffer, sample);
    return rc == (ssize_t)sample ? rc : -1;
}

int apk64_AMediaExtractor_getSampleTrackIndex(Apk64AMediaExtractor *ex) {
    return media_ndk_extractor_current_size(ex) ? 0 : -1;
}

int64_t apk64_AMediaExtractor_getSampleTime(Apk64AMediaExtractor *ex) {
    if (!media_ndk_extractor_current_size(ex) || !ex->wav.byte_rate) return -1;
    return (ex->sample_pos * INT64_C(1000000)) / ex->wav.byte_rate;
}

bool apk64_AMediaExtractor_advance(Apk64AMediaExtractor *ex) {
    size_t amount = media_ndk_extractor_current_size(ex);
    if (!amount) return false;
    ex->sample_pos += (int64_t)amount;
    return media_ndk_extractor_current_size(ex) != 0;
}

int apk64_AMediaExtractor_seekTo(Apk64AMediaExtractor *ex, int64_t seek_pos_us, int mode) {
    if (!ex || !ex->source_ready) return APK64_AMEDIA_ERROR_INVALID_OBJECT;
    if (mode < 0 || mode > 2) return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    if (seek_pos_us < 0) seek_pos_us = 0;
    uint64_t frame = ((uint64_t)seek_pos_us * ex->wav.sample_rate) / UINT64_C(1000000);
    uint64_t byte = frame * ex->wav.block_align;
    if (byte > (uint64_t)ex->wav.data_length) byte = (uint64_t)ex->wav.data_length;
    ex->sample_pos = (int64_t)byte;
    return APK64_AMEDIA_OK;
}

Apk64AMediaCodec *apk64_AMediaCodec_createDecoderByType(const char *mime_type) {
    if (!mime_type || strcmp(mime_type, "audio/raw") != 0) return NULL;
    return (Apk64AMediaCodec *)calloc(1, sizeof(Apk64AMediaCodec));
}

int apk64_AMediaCodec_delete(Apk64AMediaCodec *codec) {
    free(codec);
    return APK64_AMEDIA_OK;
}

int apk64_AMediaCodec_configure(Apk64AMediaCodec *codec, const Apk64AMediaFormat *format,
                                void *surface, void *crypto, uint32_t flags) {
    if (!codec || !format) return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    if (surface || crypto || (flags & APK64_AMEDIACODEC_CONFIGURE_FLAG_ENCODE))
        return APK64_AMEDIA_ERROR_UNSUPPORTED;
    int32_t rate = 0, channels = 0;
    const char *mime = NULL;
    if (!apk64_AMediaFormat_getString((Apk64AMediaFormat *)format, "mime", &mime) ||
        !mime || strcmp(mime, "audio/raw") != 0 ||
        !apk64_AMediaFormat_getInt32((Apk64AMediaFormat *)format, "sample-rate", &rate) ||
        !apk64_AMediaFormat_getInt32((Apk64AMediaFormat *)format, "channel-count", &channels) ||
        rate <= 0 || channels <= 0)
        return APK64_AMEDIA_ERROR_UNSUPPORTED;
    codec->sample_rate = rate;
    codec->channel_count = channels;
    codec->configured = true;
    return APK64_AMEDIA_OK;
}

int apk64_AMediaCodec_start(Apk64AMediaCodec *codec) {
    if (!codec || !codec->configured) return APK64_AMEDIA_ERROR_INVALID_OPERATION;
    codec->started = true;
    codec->input_dequeued = false;
    codec->output_ready = false;
    codec->format_change_pending = true;
    return APK64_AMEDIA_OK;
}

int apk64_AMediaCodec_stop(Apk64AMediaCodec *codec) {
    if (!codec || !codec->started) return APK64_AMEDIA_ERROR_INVALID_OPERATION;
    codec->started = false;
    codec->input_dequeued = false;
    codec->output_ready = false;
    return APK64_AMEDIA_OK;
}

int apk64_AMediaCodec_flush(Apk64AMediaCodec *codec) {
    if (!codec || !codec->started) return APK64_AMEDIA_ERROR_INVALID_OPERATION;
    codec->input_dequeued = false;
    codec->output_ready = false;
    codec->format_change_pending = true;
    codec->output_size = 0;
    return APK64_AMEDIA_OK;
}

ssize_t apk64_AMediaCodec_dequeueInputBuffer(Apk64AMediaCodec *codec, int64_t timeout_us) {
    (void)timeout_us;
    if (!codec || !codec->started) return APK64_AMEDIACODEC_INFO_TRY_AGAIN_LATER;
    if (codec->input_dequeued || codec->output_ready) return APK64_AMEDIACODEC_INFO_TRY_AGAIN_LATER;
    codec->input_dequeued = true;
    return 0;
}

uint8_t *apk64_AMediaCodec_getInputBuffer(Apk64AMediaCodec *codec, size_t idx, size_t *out_size) {
    if (!codec || !codec->started || !codec->input_dequeued || idx != 0) return NULL;
    if (out_size) *out_size = sizeof(codec->input);
    return codec->input;
}

int apk64_AMediaCodec_queueInputBuffer(Apk64AMediaCodec *codec, size_t idx, off_t offset,
                                       size_t size, uint64_t time_us, uint32_t flags) {
    if (!codec || !codec->started || !codec->input_dequeued || idx != 0 || offset < 0 ||
        (uint64_t)offset + size > sizeof(codec->input))
        return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    memcpy(codec->output, codec->input + offset, size);
    codec->output_size = size;
    codec->output_pts = (int64_t)time_us;
    codec->output_flags = flags;
    codec->output_ready = true;
    codec->input_dequeued = false;
    return APK64_AMEDIA_OK;
}

ssize_t apk64_AMediaCodec_dequeueOutputBuffer(Apk64AMediaCodec *codec,
                                               Apk64AMediaCodecBufferInfo *info,
                                               int64_t timeout_us) {
    (void)timeout_us;
    if (!codec || !codec->started) return APK64_AMEDIACODEC_INFO_TRY_AGAIN_LATER;
    if (codec->format_change_pending) {
        codec->format_change_pending = false;
        return APK64_AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED;
    }
    if (!codec->output_ready) return APK64_AMEDIACODEC_INFO_TRY_AGAIN_LATER;
    if (info) {
        info->offset = 0;
        info->size = (int32_t)codec->output_size;
        info->presentationTimeUs = codec->output_pts;
        info->flags = codec->output_flags;
    }
    return 0;
}

Apk64AMediaFormat *apk64_AMediaCodec_getOutputFormat(Apk64AMediaCodec *codec) {
    if (!codec || !codec->configured) return NULL;
    Apk64AMediaFormat *fmt = apk64_AMediaFormat_new();
    if (!fmt) return NULL;
    apk64_AMediaFormat_setString(fmt, "mime", "audio/raw");
    apk64_AMediaFormat_setInt32(fmt, "sample-rate", codec->sample_rate);
    apk64_AMediaFormat_setInt32(fmt, "channel-count", codec->channel_count);
    return fmt;
}

uint8_t *apk64_AMediaCodec_getOutputBuffer(Apk64AMediaCodec *codec, size_t idx, size_t *out_size) {
    if (!codec || !codec->started || !codec->output_ready || idx != 0) return NULL;
    if (out_size) *out_size = codec->output_size;
    return codec->output;
}

int apk64_AMediaCodec_releaseOutputBuffer(Apk64AMediaCodec *codec, size_t idx, bool render) {
    if (!codec || !codec->started || !codec->output_ready || idx != 0)
        return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    if (render) return APK64_AMEDIA_ERROR_UNSUPPORTED;
    codec->output_ready = false;
    codec->output_size = 0;
    return APK64_AMEDIA_OK;
}

int apk64_AMediaCodec_setOutputSurface(Apk64AMediaCodec *codec, void *surface) {
    if (!codec) return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    if (!codec->configured) return APK64_AMEDIA_ERROR_INVALID_OPERATION;
    /* APK64's current codec backend is audio/raw only and is never configured
       with a rendering surface. Advertising success here would falsely expose
       a video decoder path that does not exist. */
    (void)surface;
    return APK64_AMEDIA_ERROR_UNSUPPORTED;
}

/* AImage objects are produced only by AImageReader_acquire*Image(). APK64's
   codec backend has no surface/image output, so it does not export
   AImageReader and no guest AImage handle can be valid. The accessors keep the
   NDK contract: missing arguments are INVALID_PARAMETER, and any image handle
   is an invalid object. */
static int media_ndk_aimage_accessor(const Apk64AImage *image, const void *out) {
    if (!image || !out) return APK64_AMEDIA_ERROR_INVALID_PARAMETER;
    return APK64_AMEDIA_ERROR_INVALID_OBJECT;
}

int apk64_AImage_getWidth(const Apk64AImage *image, int32_t *width) {
    return media_ndk_aimage_accessor(image, width);
}

int apk64_AImage_getHeight(const Apk64AImage *image, int32_t *height) {
    return media_ndk_aimage_accessor(image, height);
}

int apk64_AImage_getFormat(const Apk64AImage *image, int32_t *format) {
    return media_ndk_aimage_accessor(image, format);
}

int apk64_AImage_getTimestamp(const Apk64AImage *image, int64_t *timestamp_ns) {
    return media_ndk_aimage_accessor(image, timestamp_ns);
}

int apk64_AImage_getNumberOfPlanes(const Apk64AImage *image, int32_t *num_planes) {
    return media_ndk_aimage_accessor(image, num_planes);
}

int apk64_AImage_getCropRect(const Apk64AImage *image, Apk64AImageCropRect *rect) {
    return media_ndk_aimage_accessor(image, rect);
}

#pragma pop_macro("APK64_MEDIA_IO_BUFFER")
