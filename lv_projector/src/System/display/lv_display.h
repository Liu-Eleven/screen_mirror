#ifndef LV_DISPLAY_H
#define LV_DISPLAY_H

#define DISPLAY_CONFIG_PATH     ("/tmp/Reserve0/disp_config.ini")
#define DISPLAY_CONFIG_BAK_PATH ("/tmp/boot-resource/disp_config.ini")

#define DISP_KSC_MAINKEY               ("ksc")
#define DISP_KSC_FLIP_H_SUBKEY         ("flip_h")
#define DISP_KSC_FLIP_V_SUBKEY         ("flip_v")

int display_set_ksc_flip_mode(int mode);
int display_get_ksc_flip_mode(void);

#endif
