#ifndef LV_KSC_H
#define LV_KSC_H

#if CONFIG_PACKAGE_libksc

#include "system_setting.h"

#define KEYSTONE_TOP_LEFT_ID         (0)
#define KEYSTONE_TOP_RIGHT_ID        (1)
#define KEYSTONE_BOTTOM_RIGHT_ID     (2)
#define KEYSTONE_BOTTOM_LEFT_ID      (3)

#define MIRROR_H_0_V_0    PROJECTION_REAR_ID
#define MIRROR_H_0_V_1    PROJECTION_CEILING_FRONT_ID
#define MIRROR_H_1_V_0    PROJECTION_FRONT_ID
#define MIRROR_H_1_V_1    PROJECTION_REAR_CEILING_ID

#define LIMIT_KSC_X_MAX_VALUE     (350)
#define LIMIT_KSC_Y_MAX_VALUE     (150)

struct kscdata{
    uint16_t tl_x;
    uint16_t tl_y;
    uint16_t tr_x;
    uint16_t tr_y;
    uint16_t bl_x;
    uint16_t bl_y;
    uint16_t br_x;
    uint16_t br_y;
    uint8_t flip_h;
    uint8_t flip_v;
    float zoom_h;
    float zoom_v;
};

int set_keystone_mirror(uint8_t mode);
int get_keystone_mirror(struct kscdata *user_data);

int set_keystone_pos(uint8_t mode, int x, int y);
int get_keystone_pos(struct kscdata *user_data);

int set_keystone_zoom(float w_ratio, float h_ratio);
int get_keystone_zoom(struct kscdata *user_data);

#endif
#endif
