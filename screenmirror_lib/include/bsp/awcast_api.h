/*
 * awcast_api.h - Forward declarations for Allwinner AWCast / USBCast BSP symbols
 *
 * These declarations mirror the ABIs exposed by:
 *   libwfd2.so          (Miracast engine)
 *   libawdlna.so        (DLNA engine)
 *   libthirdparty_mirror.so  (AirPlay / Android wire protocols)
 *
 * On host builds (no H133_BOARD) all guarded blocks are inactive and the
 * stub implementations in each protocol file are used instead.
 */

#ifndef AWCAST_API_H
#define AWCAST_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* ── AWCast unified wireless cast engine ─────────────────────────────────
 * From: lv_projector/src/AWCast/awcast.h (extended API)
 */
#ifdef H133_BOARD

int AWCast_init(void);
int AWCast_exit(void);
int AWCast_StartService(void);
int AWCast_StopService(void);
int AWCast_remove(void);
int AWCast_reset(void);
char *__Get_AWCast_device_name(void);

/* ── Miracast engine (libwfd2.so) ────────────────────────────────────────
 * From: lv_projector/src/AWCast/include/miracast2.h
 */
typedef enum {
    MIRACAST_CBK_P2P_PEERS_CHANGED = 0,
    MIRACAST_CBK_P2P_FOUND,
    MIRACAST_CBK_P2P_CONNECTING,
    MIRACAST_CBK_P2P_CONNECTED,
    MIRACAST_CBK_P2P_DISCONNECTED,
    MIRACAST_CBK_P2P_PERSISTENT_GROUPS_CHANGED,
    MIRACAST_CHK_P2P_GET_PEERS_ADDRESS,
    MIRACAST_CBK_PLAYER_START_ERROR = 10,
    MIRACAST_CBK_PLAYER_STOP_FINISHED,
    MIRACAST_CBK_PLAYER_FIRST_SHOW,
    MIRACAST_CBK_HDCP_INIT_ERROR,
    MIRACAST_CBK_HDCP_START_ERROR,
    MIRACAST_CBK_RTXP_NETWORK_ERROR = 15,
    MIRACAST_CBK_RTP_LOST_PACKET,
    MIRACAST_CBK_WFD_START_FINISHED,
    MIRACAST_CBK_WFD_STOP_FINISHED,
    MIRACAST_CHK_P2P_NEGOTIATION_ERROR = 20,
    MIRACAST_CHK_P2P_FORMATION_ERROR,
    MIRACAST_CHK_P2P_TIMEOUT_ERROR,
    MIRACAST_CHK_P2P_OVERLAP_ERROR,
    MIRACAST_CBK_P2P_BUTT,
    MIRACAST_CBK_P2P_GO_NEG_REQUEST,
    MIRACAST_CBK_P2P_INVITATION_ACCEPTED,
    MIRACAST_CBK_P2P_GROUP_STARTED,
} MIRACAST_EVENT_CALLBACK_E;

typedef int (*Miracast_Event_CallBack)(MIRACAST_EVENT_CALLBACK_E event, void *priv);

int  Miracast_Init(int enable_hdcp);
int  Miracast_DeInit(void);
int  Miracast_Start(const char *device_name, Miracast_Event_CallBack cb);
int  Miracast_Stop(void);
void Miracast_SetKey(unsigned char *key_buf);

/* ── DLNA engine (libawdlna.so) ──────────────────────────────────────────
 * From: lv_projector/src/AWCast/include/aw_dlna.h
 */
#define AWD_EVENT_ENTRY  1
#define AWD_EVENT_QUIT   2
#define AWD_EVENT_VOLUME 3
#define AWD_EVENT_PAUSE  4
#define AWD_EVENT_START  5

typedef struct AWDlnaS AWDlnaT;

/* Render handle – concrete type lives in irender.h on H133.
 * We treat it as opaque here. */
typedef void RenderT;

struct AWD_LinstenerS {
    int (*notify)(int event, void *param);
};

AWDlnaT *AWD_Instance(char *device_name, char *uuid,
                      struct AWD_LinstenerS *listener,
                      RenderT *player_render);
int AWD_Start(AWDlnaT *awd);
int AWD_Stop(AWDlnaT *awd);
int AWD_Render_Stop(void);
int AWD_Destroy(AWDlnaT *awd);

/* ── AirPlay / Android-wire protocol bridge (libthirdparty_mirror.so) ───
 * From: lv_projector/src/AWCast/include/aw_mirror_interface.h
 */
#define AIRPLAY_LIB "/usr/lib/libthirdparty_mirror.so"
#define WIRED_LIB   "/usr/lib/libthirdparty_mirror.so"

typedef enum {
    ANDROID_MIRROR_START,
    ANDROID_MIRROR_STOP,
    IOS_MIRROR_START,
    IOS_MIRROR_STOP,
    AIRPLAY_MIRROR_START,
    AIRPLAY_MIRROR_STOP,
    AIRPLAY_AUDIO_START,
    AIRPLAY_AUDIO_STOP,
    AIRPLAY_URL_START,
    AIRPLAY_URL_STOP,
} MirrorEventE;

typedef int (*mirror_event_cb)(MirrorEventE event, void *param);

struct AirplayServiceS {
    int (*init)(char *dev_name, void *cb);
    int (*exit)(void);
    int (*start)(void);
    int (*stop)(void);
};

struct AndroidLineServiceS {
    int (*init)(char *dev_name, void *cb);
    int (*exit)(void);
    int (*start)(void);
    int (*stop)(void);
};

struct AirplayLineServiceS {
    int (*init)(char *dev_name, void *cb);
    int (*exit)(void);
    int (*start)(void);
    int (*stop)(void);
};

typedef struct AirplayServiceS *     (*AirplayServiceGetInstance)(void);
typedef struct AirplayLineServiceS * (*AirplayLineServiceGetInstance)(void);
typedef struct AndroidLineServiceS * (*AndroidLineServiceGetInstance)(void);

/* ── USBCast (wired, WSP_ prefix) ───────────────────────────────────────
 * From: lv_projector/src/USBCast/usbcast.h
 */
int WSP_init(void);
int WSP_exit(void);
int WSP_StartService(void);
int WSP_StopService(void);

#endif /* H133_BOARD */

#ifdef __cplusplus
}
#endif

#endif /* AWCAST_API_H */
