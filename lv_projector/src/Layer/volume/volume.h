#ifndef LVGL_VOLUME_H
#define LVGL_VOLUME_H

void create_ui_volume(uint32_t key);
void create_ui_mute_icon(void);
void del_ui_volume(void);
void lv_set_volume(uint8_t volume);
long lv_get_volume(void);
bool is_volume_mute(void);
void lv_set_mute(bool state);

#endif
