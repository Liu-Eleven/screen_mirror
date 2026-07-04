/*
rsc_mgr.c: use to manage the UI resource: string, font, etc
 */
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "lvgl/lvgl.h"
#include "lvgl/src/font/lv_font.h"

#include "lv_common.h"
#include "sys_param.h"
#include "../Common/language/string/lv_string_id.h"
#include "../Common/language/string/str_english.h"
#include "../Common/language/string/str_chinese.h"
#include "../Common/language/string/str_french.h"
#include "../Common/language/string/str_german.h"
#include "../Common/language/string/str_italian.h"
#include "../Common/language/string/str_japanese.h"
#include "../Common/language/string/str_russian.h"
#include "../Common/language/string/str_spanish.h"

uint8_t *lv_get_string_by_langid(uint8_t lang_id, uint32_t string_id)
{
    switch (lang_id) {
        case LANGUAGE_ENGLISH:
            return strs_array_english[string_id];
        case LANGUAGE_CHINESE:
            return strs_array_chinese[string_id];
        case LANGUAGE_FRENCH:
            return strs_array_french[string_id];
        case LANGUAGE_GERMAN:
            return strs_array_german[string_id];
        case LANGUAGE_JAPANESE:
            return strs_array_japanese[string_id];
        case LANGUAGE_ITALIAN:
            return strs_array_italian[string_id];
        case LANGUAGE_SPANISH:
            return strs_array_spanish[string_id];
        case LANGUAGE_RUSSIAN:
            return strs_array_russian[string_id];
        default:
            return NULL;
    }
}

uint8_t *lv_get_string(uint32_t string_id)
{
    uint8_t lang_id = lv_get_sys_param(P_OSD_LANGUAGE);
    return lv_get_string_by_langid(lang_id, string_id);
}

