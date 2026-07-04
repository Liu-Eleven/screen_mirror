#include <stdint.h>
#include "motor/lv_motor.h"
#include "lv_common.h"
#if ENABLE_KSC
#include "lv_ksc.h"
#include "libksc.h"

#define KSC_OPEN        (0)
#define KSC_CLOSE       (1)
#define KSC_SET_POS     (2)
#define KSC_GET_POS     (3)
#define KSC_SET_MIRROR  (4)
#define KSC_GET_MIRROR  (5)
#define KSC_SET_ZOOM    (6)
#define KSC_GET_ZOOM    (7)

struct kscdata g_ksc_data;

/*

(0, 0)                                                                        x
+---------------------------------------------------------------------------->
|                                                                            |
|                                                                            |
|            (tl_x,tl_y)   /---------------------\ (tr_x, tr_y)              |
|                         /                       \                          |
|                        /                         \                         |
|                       /                           \                        |
|                      /                             \                       |
|                     /                               \                      |
|                    /                                 \                     |
|                   /                                   \                    |
|                  /                                     \                   |
|                 /                                       \                  |
|    (bl_x, bl_y)/-----------------------------------------\  (br_x, br_y)   |
|                                                                            |
|                                                                            |
v----------------------------------------------------------------------------+
 y

*/

static int keystone_pos_remap(struct ksc_para *para, struct kscdata *ksc_data_p)
{
#if 0
    printf("1 tl[%d,%d], tr[%d,%d], bl[%d,%d], br[%d,%d]\n", para->pos_dst_tl_x, para->pos_dst_tl_y,
        para->pos_dst_tr_x, para->pos_dst_tr_y, para->pos_dst_bl_x, para->pos_dst_bl_y,
        para->pos_dst_br_x, para->pos_dst_br_y);
#endif

    if (para->pos_dst_br_x == 0 && para->pos_dst_br_y == 0) {
        para->pos_dst_tl_x = 0;
        para->pos_dst_tl_y = 0;

        para->pos_dst_tr_x = para->src_w - 1;
        para->pos_dst_tr_y = 0;

        para->pos_dst_bl_x = 0;
        para->pos_dst_bl_y = para->src_h - 1;

        para->pos_dst_br_x = para->src_w - 1;
        para->pos_dst_br_y = para->src_h - 1;

    }

    para->pos_dst_tl_x = ksc_data_p->tl_x;
    para->pos_dst_tl_y = ksc_data_p->tl_y;
    para->pos_dst_tr_x = para->src_w - ksc_data_p->tr_x - 1;
    para->pos_dst_tr_y = ksc_data_p->tr_y;
    para->pos_dst_bl_x = ksc_data_p->bl_x;
    para->pos_dst_bl_y = para->src_h - ksc_data_p->bl_y - 1;
    para->pos_dst_br_x = para->src_w - ksc_data_p->br_x - 1;
    para->pos_dst_br_y = para->src_h - ksc_data_p->br_y - 1;
#if 0
    printf("2 remap: tl[%d,%d], tr[%d,%d], bl[%d,%d], br[%d,%d]\n", para->pos_dst_tl_x, para->pos_dst_tl_y,
        para->pos_dst_tr_x, para->pos_dst_tr_y, para->pos_dst_bl_x, para->pos_dst_bl_y,
        para->pos_dst_br_x, para->pos_dst_br_y);
#endif

    return 0;
}

static void init_ksc_para(struct ksc_para *para)
{
    para->ksc_en = 1;

    /* limit zoom min and max value */
    if (para->zoom_h <= 40 || para->zoom_h > 100)
        para->zoom_h = 100;

    if (para->zoom_v <= 40 || para->zoom_v > 100)
        para->zoom_v = 100;
}

static int soc_set_ksc(uint8_t style, uint8_t mode, struct kscdata *ksc_data_p)
{
    struct ksc_para ksc_para;
    int ret;

    ret = libksc_init();
    if (ret) {
        loge("libksc_init fali!\n");
        goto OUT;
    }

    memset(&ksc_para, 0, sizeof(struct ksc_para));

    ret = libksc_get_para(&ksc_para);
    if (ret) {
        loge("libksc_get_para fali!\n");
        goto OUT;
    }

    init_ksc_para(&ksc_para);

    /* pos is not save in driver, so must set keystone pos */
    keystone_pos_remap(&ksc_para, ksc_data_p);

    if (style == KSC_OPEN) {
    } else if (style == KSC_CLOSE) {
        ksc_para.ksc_en = 0;
    } else if (style == KSC_SET_POS) {
    } else if (style == KSC_GET_POS) {
        return 0;
    } else if (style == KSC_SET_MIRROR) {
        if (mode == MIRROR_H_0_V_0) {
            ksc_para.flip_h = 0;
            ksc_para.flip_v = 0;
            ksc_data_p->flip_h = 0;
            ksc_data_p->flip_v = 0;
        } else if (mode == MIRROR_H_0_V_1) {
            ksc_para.flip_h = 0;
            ksc_para.flip_v = 1;
            ksc_data_p->flip_h = 0;
            ksc_data_p->flip_v = 1;
        } else if (mode == MIRROR_H_1_V_0) {
            ksc_para.flip_h = 1;
            ksc_para.flip_v = 0;
            ksc_data_p->flip_h = 1;
            ksc_data_p->flip_v = 0;
        } else if (mode == MIRROR_H_1_V_1) {
            ksc_para.flip_h = 1;
            ksc_para.flip_v = 1;
            ksc_data_p->flip_h = 1;
            ksc_data_p->flip_v = 1;
        } else {
            goto OUT;
        }
    } else if (style == KSC_GET_MIRROR) {
    } else if (style == KSC_SET_ZOOM) {
    } else if (style == KSC_GET_ZOOM) {
    }

    /* zoom is not save in driver, so must set keystone zoom */
    if (ksc_data_p->zoom_h)
        ksc_para.zoom_h = ksc_data_p->zoom_h;

    if (ksc_data_p->zoom_v)
        ksc_para.zoom_v = ksc_data_p->zoom_v;

    libksc_set_para(&ksc_para);
    return 0;

OUT:
    return -1;
}

int set_keystone_pos(uint8_t mode, int x, int y)
{
    if (x < 0)
        x = 0;
    else if (x > LIMIT_KSC_X_MAX_VALUE)
        x = LIMIT_KSC_X_MAX_VALUE;

    if (y < 0)
        y = 0;
    else if (y > LIMIT_KSC_Y_MAX_VALUE)
        y = LIMIT_KSC_Y_MAX_VALUE;

    if (mode == KEYSTONE_TOP_LEFT_ID) {
        g_ksc_data.tl_x = x;
        g_ksc_data.tl_y = y;
    } else if (mode == KEYSTONE_TOP_RIGHT_ID) {
        g_ksc_data.tr_x = x;
        g_ksc_data.tr_y = y;
    } else if (mode == KEYSTONE_BOTTOM_LEFT_ID) {
        g_ksc_data.bl_x = x;
        g_ksc_data.bl_y = y;
    } else if (mode == KEYSTONE_BOTTOM_RIGHT_ID) {
        g_ksc_data.br_x = x;
        g_ksc_data.br_y = y;
    }

    return soc_set_ksc(KSC_SET_POS, mode, &g_ksc_data);
}

int get_keystone_pos(struct kscdata *user_data)
{
    if (!user_data) {
        loge("get_keystone_pos is error\n");
        return -1;
    }
    memcpy(user_data, &g_ksc_data, sizeof(struct kscdata));
    return 0;
}

int set_keystone_mirror(uint8_t mode)
{
    return soc_set_ksc(KSC_SET_MIRROR, mode, &g_ksc_data);
}

int get_keystone_mirror(struct kscdata *user_data)
{
    if (!user_data) {
        loge("get_keystone_mirror is error\n");
        return -1;
    }
    memcpy(user_data, &g_ksc_data, sizeof(struct kscdata));
    return 0;
}

int set_keystone_zoom(float w_ratio, float h_ratio)
{
    if (w_ratio > 0 && w_ratio <=100)
        g_ksc_data.zoom_h = w_ratio;

    if (h_ratio > 0 && h_ratio <= 100)
        g_ksc_data.zoom_v = h_ratio;

    return soc_set_ksc(KSC_SET_ZOOM, 0, &g_ksc_data);
}

int get_keystone_zoom(struct kscdata *user_data)
{
    if (!user_data) {
        loge("get_keystone_pos is error\n");
        return -1;
    }
    memcpy(user_data, &g_ksc_data, sizeof(struct kscdata));
    return 0;
}
#endif
