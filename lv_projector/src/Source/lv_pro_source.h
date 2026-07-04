
#ifndef LV_PRO_SIGNALIN_H
#define LV_PRO_SIGNALIN_H

#include "lvgl/lvgl.h"
#include "hdmi/hdmi_control.h"

extern lv_obj_t *SignalIn_activity;
extern lv_group_t *SignalIn_group;
extern bool SignalIn_activity_exit;

void lv_pro_signalin_init(void);
#if CONFIG_PACKAGE_libhdmiapi
void HdmiSelect_Init(void);
void HdmiEvent_Callback(TSourceId source_id, TMsgId msg, void *data);
#endif
void no_signal_cont_init(void);
void no_signal_cont_deinit(void);

#endif  /*LV_PRO_SIGNALIN_H*/
