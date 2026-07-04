#ifndef USBCAST_H
#define USBCAST_H

#ifdef __cplusplus
extern "C"
{
#endif

int WSP_init(void);

int WSP_exit(void);

int WSP_StartService(void);

int WSP_StopService(void);

#ifdef __cplusplus
}
#endif

#endif
