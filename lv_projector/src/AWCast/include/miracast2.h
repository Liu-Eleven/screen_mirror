#ifndef __MIRACAST_LINUX_H__
#define __MIRACAST_LINUX_H__
#include <stdint.h>
//#include "WFDPlayer2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SOURCE_NAME_MAX_LEN (255)

/**Miracast State*//** CNcomment:Miracast ״̬ */
typedef enum MIRACAST_STATE
{
    INVALID = 0,
    INIT = 1,
    START = 2,
    CONNECT = 3,
    DISCONNECT = START,
    STOP = INIT,
    DEINIT = INVALID,
} MIRACAST_STATE;

typedef enum LOWDELAY_MODE_E
{
    MIRACAST_LOWDELAY_FIRST,
    MIRACAST_LOWDELAY_SMOOTH_LEVEL1
} MIRACAST_LOWDELAY_MODE_E;

/**Miracast callback function type*//** CNcomment:Miracast �ص����������� */
typedef enum MIRACAST_EVENT_CALLBACK_E
{
    MIRACAST_P2P_CBK_INVALID = -1,               /**<Invalid Enum - Lower Bound Value *//**<CNcomment:��ЧEnum-����ֵ */

    MIRACAST_CBK_P2P_PEERS_CHANGED = 0,          /**< P2P peers changed *//** CNcomment:P2P�Զ�״̬�仯�Ļص����� */
    MIRACAST_CBK_P2P_FOUND,                      /**< p2p found *//** CNcomment:P2P 设备发现 */
    MIRACAST_CBK_P2P_CONNECTING,                 /**< P2P connecting *//** CNcomment:source�˺�sink�˽��������еĻص����� */
    MIRACAST_CBK_P2P_CONNECTED,                  /**< P2P connected *//** CNcomment:source�˺�sink�����ӳɹ��Ļص����� */
    MIRACAST_CBK_P2P_DISCONNECTED,               /**< P2P disconnected *//** CNcomment:source�˺�sink�˶Ͽ����ӵĻص����� */
    MIRACAST_CBK_P2P_PERSISTENT_GROUPS_CHANGED,  /**< P2P group changed *//** CNcomment:P2P��仯�Ļص����� */
    MIRACAST_CHK_P2P_GET_PEERS_ADDRESS,          /**< P2P get peers address*//** CNcomment:�ɹ���ȡ�Զ�IP */

    MIRACAST_CBK_PLAYER_START_ERROR = 10,        /**< Player start error *//** CNcomment:����������ʧ�� */
    MIRACAST_CBK_PLAYER_STOP_FINISHED,           /**< Player start error *//** CNcomment:�������رս��� */
    MIRACAST_CBK_PLAYER_FIRST_SHOW,              /**< Player start error *//** CNcomment:��������ʾ��һ֡ */
    MIRACAST_CBK_HDCP_INIT_ERROR,                /**< Hdcp init error *//** CNcomment:HDCP��ʼ��ʧ�� */
    MIRACAST_CBK_HDCP_START_ERROR,               /**< Hdcp start error *//** CNcomment:HDCP����ʧ�� */

    MIRACAST_CBK_RTXP_NETWORK_ERROR = 15,        /**< Rtsp/Rtcp/Rtp Network Error *//** CNcomment:Rtsp/Rtcp/Rtp��������ʧ�� */
    MIRACAST_CBK_RTP_LOST_PACKET,                /**< Rtp lost packet *//** CNcomment:RTP���ݰ����� */
    MIRACAST_CBK_WFD_START_FINISHED,             /**< Wfd start finished *//** CNcomment:wifi displayЭ�����ӳɹ� */
    MIRACAST_CBK_WFD_STOP_FINISHED,              /**< Wfd stop finished *//** CNcomment:wifi displayЭ��Ͽ����ӳɹ� */

    MIRACAST_CHK_P2P_NEGOTIATION_ERROR =20,      /**< p2p negotiation error *//** CNcomment:P2PЭ��ʧ�� */
    MIRACAST_CHK_P2P_FORMATION_ERROR,            /**< p2p formation error *//** CNcomment:GO/GC����ʧ�� */
    MIRACAST_CHK_P2P_TIMEOUT_ERROR,              /**< p2p connect timeout error *//** CNcomment:P2P���ӳ�ʱ */
    MIRACAST_CHK_P2P_OVERLAP_ERROR,              /**< p2p overlap error *//** CNcomment:��P2P�źŷ�Χ���ж��豸ͬʱ���ӵ���ʧ�� */

    MIRACAST_CBK_P2P_BUTT,                       /**<Invalid Enum - Higher Bound Value *//**<CNcomment:��ЧEnum-����ֵ */
	MIRACAST_CBK_P2P_GO_NEG_REQUEST,
	MIRACAST_CBK_P2P_INVITATION_ACCEPTED,
	MIRACAST_CBK_P2P_GROUP_STARTED,
} MIRACAST_EVENT_CALLBACK_E;

/**Miracast p2p net mode type *//** CNcomment:Miracast ����ģʽ���� */
typedef enum MIRACAST_P2P_NETMODE_E
{
    MIRACAST_P2P_NETMODE_DEFAULT = 0,   /** default P2p net mode, decide by wifi driver, concurrent mode priority *//** CNcomment: ȱʡ����ģʽ���������� */
    MIRACAST_P2P_NETMODE_CONCURRENT,    /** concurrent P2p net mode, P2p concurrent with sta *//** CNcomment: P2P��STA����ģʽ */
    MIRACAST_P2P_NETMODE_STANDALONE,    /** standalone P2p net mode, P2p doesn't concurrent with sta *//** CNcomment: P2P����ģʽ������STA���� */
    MIRACAST_P2P_NETMODE_BUTT
} MIRACAST_P2P_NETMODE_E;

/**Miracast p2p groupowner mode *//** CNcomment:Miracast Group Ownerģʽ */
typedef enum MIRACAST_P2P_GOMODE_E
{
    MIRACAST_P2P_GOMODE_DEFAULT = 0,   /** default P2p go mode, decide by wifi driver, 2.4G force and 5G negotiation priority *//** CNcomment:ȱʡP2p GOģʽ�����Ȳ���Ϊ2.4ǿ��GO��5GЭ��GO */
    MIRACAST_P2P_GOMODE_NEGOTIATION,   /** P2p go negotiation *//** CNcomment:P2pЭ��GOģʽ */
    MIRACAST_P2P_GOMODE_FORCEGO,       /** P2p force go *//** CNcomment:P2pǿ��GOģʽ */
    MIRACAST_NETMODE_BUTT
} MIRACAST_P2P_GOMODE_E;

typedef struct MIRACAST_P2P_CONNECTING_INFO
{
    char sourceName[SOURCE_NAME_MAX_LEN + 1];
} MIRACAST_P2P_CONNECTING_INFO;

typedef struct MIRACAST_LOST_INFO
{
    uint32_t totalPacket;
    uint32_t lostPacket;
} MIRACAST_LOST_INFO;

/** Callback function of receiving Miracast events *//** CNcomment:����Miracast�¼��Ļص����� */
typedef int (*Miracast_Event_CallBack)(MIRACAST_EVENT_CALLBACK_E enEvent, void* pvPrivateData);

typedef enum {
    WFD_VIDEO_FORMAT_AUTO,
    WFD_VIDEO_FORMAT_1080P30,
    WFD_VIDEO_FORMAT_720P60,
    WFD_VIDEO_FORMAT_720P30,
    WFD_VIDEO_FORMAT_720P24,
} MIRACAST_RTSP_VIDEO_FORMAT;

/**
\brief: init Miracast.CNcomment:��ʼ��Miracast CNend
\attention \n
\param[in] whether support HDCP.CNcomment:�Ƿ�֧��HDCP���� CNend
\retval  ::SUCCESS
\retval  ::FAILURE
\see \n
::Miracast_DeInit
*/
int Miracast_Init(int isHdcp);

/* p2p_interface: wlan0 or wlan1 or p2p0 */
int Miracast_Init_ex(int isHdcp, const char *videoFormat, const char *p2p_interface);

/**
\brief: start Miracast.CNcomment:����Miracast CNend
\attention \n
\param[in] pcDeviceName sink device name, must be less than 33 bytes, can be null.CNcomment:����豸���֣�����С��33���ֽڣ�����Ϊ�� CNend
\param[in] pFnEventCb callback func.CNcomment:�ص�����ʵ�� CNend
\retval  ::SUCCESS
\retval  ::FAILURE
\see \n
::Miracast_Stop
*/
int Miracast_Start(const char* pcDeviceName, Miracast_Event_CallBack pFnEventCb);

/**
\brief: disconnect connection.CNcomment:�Ͽ����� CNend
\attention \n
\param    N/A.CNcomment:�� CNend
\retval  ::SUCCESS
\retval  ::FAILURE
\see \n
::Miracast_Disconnect
*/
int Miracast_Disconnect(void);

/**
\brief: stop Miracast.CNcomment:ͣ��Miracast CNend
\attention \n
\param     N/A.CNcomment:�� CNend
\retval  ::SUCCESS
\retval  ::FAILURE
\see \n
::Miracast_Stop
*/
int Miracast_Stop(void);

/**
\brief: deinit Miracast.CNcomment:ȥ��ʼ��Miracast CNend
\attention \n
\param     N/A.CNcomment:�� CNend
\retval  ::SUCCESS
\retval  ::FAILURE
\see \n
::Miracast_DeInit
*/
int Miracast_DeInit(void);

/**
\brief: get Miracast Current State.CNcomment:��ȡMiracast��ǰ״̬ CNend
\attention \n
\param     N/A.CNcomment:�� CNend
\retval  ::MIRACAST_STATE
\see \n
::
*/
MIRACAST_STATE Miracast_GetState(void);

/**
\brief: modify Miracast Name.CNcomment:�޸�Miracast���� CNend
\attention \n
\param[in] pcDeviceName sink device name, must be less than 33 bytes, can be null.CNcomment:����豸���֣�����С��33���ֽڣ�����Ϊ�� CNend
\retval  ::SUCCESS
\retval  ::FAILURE
\see \n
::
*/
int Miracast_ModifyName(const char* pcDeviceName);

/**
\brief: set Miracast P2 net mode.CNcomment:����Miracast����ģʽ CNend
\attention \n
\param[in] p2pNetMode p2p net mode, default or concurrent or standalone.CNcomment:P2p����ģʽ��ϵͳȱʡ��ģʽ������STA����ģʽ���߶���������ģʽ CNend
\retval  ::SUCCESS
\retval  ::FAILURE
\see \n
*/
int Miracast_SetP2pNetMode(MIRACAST_P2P_NETMODE_E p2pNetMode);

/**
\brief: set Miracast P2 Group Owner mode.CNcomment:����Miracast Group Ownerģʽ CNend
\attention \n
\param[in] p2pGoMode p2p go mode, default or force or negotiation.CNcomment:P2p goģʽ��ȱʡ��ģʽ����ǿ��GOģʽ�����Զ�Э��ģʽ CNend
\retval  ::SUCCESS
\retval  ::FAILURE
\see \n
*/
int Miracast_SetP2pGOMode(MIRACAST_P2P_GOMODE_E p2pGoMode);

void Miracast_SetLowDelay(MIRACAST_LOWDELAY_MODE_E lowDelayMode);

//int Miracast_RotationAngle(enum ROTATEDEGREE eRotateDegree);

//hdcp set and get key for lv_projector
int Miracast_SetKey(unsigned char* key_buf);
unsigned char* Miracast_GetKey(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* __MIRACAST_LINUX_H__ */
