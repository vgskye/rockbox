
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Skye Green
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fixedpoint.h"
#include "bt_sw_volume.h"

#define BT_SW_VOLUME_FRACBITS 16
#define BT_SW_VOLUME_UNITY (1 << BT_SW_VOLUME_FRACBITS)

static uint32_t bt_sw_volume_new_factor_l = 0, bt_sw_volume_new_factor_r = 0;
static uint32_t bt_sw_volume_factor_l = BT_SW_VOLUME_UNITY, bt_sw_volume_factor_r = BT_SW_VOLUME_UNITY;
static uint32_t bt_sw_volume_scale_progress = 0;
static typeof (memcpy) *bt_sw_volume_scaling_fn = memcpy;

static void bt_sw_volume_sync_pcm_factors(void);

/* Return the scale factor corresponding to the centibel level */
static uint32_t bt_sw_volume_centibels_to_factor(int volume)
{
    if (volume <= -730)
        return 0;
    
    if (volume > 0)
        return BT_SW_VOLUME_UNITY;

    /* Centibels -> fixedpoint */
    return (uint32_t)fp_factor(fp_div(volume, 10, BT_SW_VOLUME_FRACBITS),
                               BT_SW_VOLUME_FRACBITS);
}

static inline int32_t bt_sw_volume_scale_sample(uint32_t f, int32_t s)
{
    return (f * s) >> BT_SW_VOLUME_FRACBITS;
}

static void * bt_sw_volume_scale_buffer_cut(void *dst, const void *src, size_t src_size)
{
    int16_t *d = dst;
    const int16_t *s = src;
    uint32_t factor_l = bt_sw_volume_factor_l, factor_r = bt_sw_volume_factor_r;

    while (src_size)
    {
        *d++ = bt_sw_volume_scale_sample(factor_l, *s++);
        *d++ = bt_sw_volume_scale_sample(factor_r, *s++);
        src_size -= (2 * sizeof (int16_t));
    }

    return dst;
}

/* Transition the volume change smoothly across a frame */
static void * bt_sw_volume_scale_buffer_trans(void *dst, const void *src, size_t src_size)
{
    int16_t *d = dst;
    const int16_t *s = src;
    uint32_t factor_l = bt_sw_volume_factor_l, factor_r = bt_sw_volume_factor_r;

    /* Transition from the old value to the new value using an inverted cosinus
       from PI..0 in order to minimize amplitude-modulated harmonics generation
       (zipper effects). */
    uint32_t new_factor_l = bt_sw_volume_new_factor_l;
    uint32_t new_factor_r = bt_sw_volume_new_factor_r;

    int32_t diff_l = (int32_t)new_factor_l - (int32_t)factor_l;
    int32_t diff_r = (int32_t)new_factor_r - (int32_t)factor_r;

    while (src_size && bt_sw_volume_scale_progress < 1024)
    {
        int32_t sweep = (1 << 14) - fp14_cos(180*bt_sw_volume_scale_progress / 1024); /* 0.0..2.0 */
        uint32_t f_l = fp_mul(sweep, diff_l, 15) + factor_l;
        uint32_t f_r = fp_mul(sweep, diff_r, 15) + factor_r;
        *d++ = bt_sw_volume_scale_sample(f_l, *s++);
        *d++ = bt_sw_volume_scale_sample(f_r, *s++);
        src_size -= (2 * sizeof (int16_t));
        bt_sw_volume_scale_progress += 1;
    }

    if (bt_sw_volume_scale_progress >= 1024) {
        bt_sw_volume_sync_pcm_factors();
    }

    if (src_size) {
        bt_sw_volume_scaling_fn(d, s, src_size);
    }

    return dst;
}

/* Assign the new scaling function for normal steady-state operation */
static void bt_sw_volume_sync_pcm_factors(void)
{
    uint32_t new_factor_l = bt_sw_volume_new_factor_l;
    uint32_t new_factor_r = bt_sw_volume_new_factor_r;

    bt_sw_volume_factor_l = new_factor_l;
    bt_sw_volume_factor_r = new_factor_r;

    if (new_factor_l == 65536 && new_factor_r == 65536) {
        bt_sw_volume_scaling_fn = memcpy;
    } else {
        bt_sw_volume_scaling_fn = bt_sw_volume_scale_buffer_cut;
    }
}

void bt_sw_volume_set_volume(int vol_l, int vol_r, bool immediate)
{
    bt_sw_volume_new_factor_l = bt_sw_volume_centibels_to_factor(vol_l);
    bt_sw_volume_new_factor_r = bt_sw_volume_centibels_to_factor(vol_r);

    if (immediate) {
        bt_sw_volume_sync_pcm_factors();
    } else if (bt_sw_volume_new_factor_l != bt_sw_volume_factor_l || bt_sw_volume_new_factor_r != bt_sw_volume_factor_r) {
        bt_sw_volume_scale_progress = 0;
        bt_sw_volume_scaling_fn = bt_sw_volume_scale_buffer_trans;
    }
}

void bt_sw_volume_copy_buffer(void *dst, const void *src, size_t src_size)
{
    bt_sw_volume_scaling_fn(dst, src, src_size);
}