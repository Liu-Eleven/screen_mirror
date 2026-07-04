
#ifndef MIRACLE_UIBCCTL_H
#define MIRACLE_UIBCCTL_H
#ifndef VideoFormats
#define VideoFormats
#include "VideoFormats.h"
#endif

typedef struct UIBCImplS UIBCContextT;

#ifdef __cplusplus
extern "C"
{
#endif

UIBCContextT *UIBC_Instance(char const* host, unsigned short port, struct config_t* negotiated_video_format);

int UIBC_Destroy(UIBCContextT *ctx);

#ifdef __cplusplus
}
#endif

#endif
