/*
 * Copyright (C) 2014 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/log.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <hardware/lights.h>

/******************************************************************************/

#define MAX_PATH_SIZE 80

#define LED_LIGHT_OFF 0
#define LED_LIGHT_ON 255

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static struct light_state_t g_attention;

char const*const LCD_FILE
        = "/sys/class/leds/lcd-backlight/brightness";

char const*const RGB_CONTROL_FILE
        = "/sys/class/leds/rgb/control";

/**
 * device methods
 */

void init_globals(void)
{
    // init the mutex
    pthread_mutex_init(&g_lock, NULL);
}

static int
write_int(char const* path, int value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        int bytes = sprintf(buffer, "%d\n", value);
        int amt = write(fd, buffer, bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_int failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int
write_str(char const* path, char *value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[PAGE_SIZE];
        int bytes = sprintf(buffer, "%s\n", value);
        int amt = write(fd, buffer, bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_str failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int
is_lit(struct light_state_t const* state)
{
    return state->color & 0x00ffffff;
}

static int
rgb_to_brightness(struct light_state_t const* state)
{
    int color = state->color & 0x00ffffff;
    return ((77*((color>>16)&0x00ff))
            + (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
}

static int
set_light_backlight(__attribute__((unused)) struct light_device_t* dev,
        struct light_state_t const* state)
{
    int err = 0;
    int brightness = rgb_to_brightness(state);
    pthread_mutex_lock(&g_lock);
    err = write_int(LCD_FILE, brightness);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static int
set_light_locked(struct light_state_t const* state)
{
    int onMS, offMS;
    int brightness_level;
    char blink_pattern[PAGE_SIZE];

    switch (state->flashMode) {
        case LIGHT_FLASH_TIMED:
        case LIGHT_FLASH_HARDWARE:
            onMS = state->flashOnMS;
            offMS = state->flashOffMS;
            break;
        case LIGHT_FLASH_NONE:
        default:
            onMS = 0;
            offMS = 0;
            break;
    }

    if (is_lit(state))
        brightness_level = (state->color & 0xff000000) ?
                           (state->color & 0xff000000) >> 24 : LED_LIGHT_ON;
    else
        brightness_level = LED_LIGHT_OFF;

    /*
     * The red component of what should be the color of the LED
     * is actually the brightness level (0 <-> 255).
     * Ramp up/down are hard-coded in the kernel driver.
     */
    sprintf(blink_pattern, "%x0000 %d %d 1 1", brightness_level, onMS, offMS);

    return write_str(RGB_CONTROL_FILE, blink_pattern);
}

static int
handle_led_prioritized_locked(struct light_state_t const* state)
{
    if (is_lit(&g_attention)) {
        return set_light_locked(&g_attention);
    } else {
        return set_light_locked(state);
    }
}

static int
set_light_notifications(__attribute__((unused)) struct light_device_t* dev,
        struct light_state_t const* state)
{
    int err = 0;
    pthread_mutex_lock(&g_lock);
    err = handle_led_prioritized_locked(state);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static int
set_light_attention(__attribute__((unused)) struct light_device_t* dev,
        struct light_state_t const* state)
{
    int err = 0;
    pthread_mutex_lock(&g_lock);
    g_attention = *state;
    err = handle_led_prioritized_locked(state);
    pthread_mutex_unlock(&g_lock);
    return err;
}


/** Close the lights device */
static int
close_lights(struct light_device_t *dev)
{
    if (dev) {
        free(dev);
    }
    return 0;
}


/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int (*set_light)(struct light_device_t* dev,
            struct light_state_t const* state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
        set_light = set_light_notifications;
    else if (0 == strcmp(LIGHT_ID_ATTENTION, name))
        set_light = set_light_attention;
    else
        return -EINVAL;

    pthread_once(&g_init, init_globals);

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*
 * The lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "MSM8226 lights Module",
    .author = "Google, Inc., dhacker29",
    .methods = &lights_module_methods,
};
