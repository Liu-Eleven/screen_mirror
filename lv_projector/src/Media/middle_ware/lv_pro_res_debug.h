#ifndef LV_PRO_RES_DEBUG_H
#define LV_PRO_RES_DEBUG_H


#define mw_err(fmt, args...) printf("[%s, %d]error: "fmt"\r\n", __func__, __LINE__, ##args)

#define MIDDLE_WARE_DEBUG 1
#if MIDDLE_WARE_DEBUG
#define mw_dbg(fmt, args...) printf("[%s, %d]debug "fmt"\r\n", __func__, __LINE__, ##args)
#else
#define mw_dbg(fmt, args...)
#endif


#endif  /*LV_PRO_RES_DEBUG_H*/
