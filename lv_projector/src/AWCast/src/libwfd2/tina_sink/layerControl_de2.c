/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : layerControl_de.cpp
* Description : display DE -- for H133
* History :
*/


#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <cdx_log.h>
#include <errno.h>
#include "cdx_config.h"
#include "layerControl.h"
#include "list.h"
#include "disputils2.h"
#include "vbasetype.h"
//#include "CdxUci.h" //add for get display regionScale value

//#define GLOBAL_LOG_LEVEL    LOG_LEVEL_VERBOSE

#include <CdxIon.h>

#include "sunxi_display2.h"

#include "iniparserapi.h"
//#include "uci.h" //add for get display regionScale value

#define SAVE_PIC (0)

#define GPU_BUFFER_NUM 32

static VideoPicture* gLastPicture = NULL;

#define BUF_MANAGE (0)

#define NUM_OF_PICTURE_NODES 16
#define JUDGE_DIFF_THRESHOLD 0.000001
#define SWITCH_BUFFER_SIZE 2
#define CROP_INFO_NUM 120

#define DISPLAY_UCI_CONFIG_PATH "/etc/config/system"


#if VE_SUPPORT_DECODER_LBC_MODE
#define MB_WTH 8
#define CODE_MODE_Y_BITS 3
#define CODE_MODE_C_BITS 2
#define BLK_MODE_BITS    2
#define C_DTS_BITS       4
#define ALIGN 128
#endif

int LayerCtrlHideVideo(LayerCtrl* l);

typedef struct VPictureNode_t VPictureNode;
struct VPictureNode_t
{
    struct list_head list;
    VideoPicture* pPicture;
    int           bUsed; // whether node is in list
    bool          dequeueable;//node can be dequeued from node list after it displayed,
    bool          bValid;// whether picture node is valid to display
};

typedef struct BufferInfoS
{
    VideoPicture pPicture;
    int          nUsedFlag;
    void*        pMetaDataVirAddr;
    void*        pMetaDataPhyAddr;
    int          nMetaDataVirAddrSize;
    int          nMetaDataMapFd;
}BufferInfoT;

typedef struct LayerContext
{
    LayerCtrl            base;
    enum EPIXELFORMAT    eDisplayPixelFormat;
    int                  nWidth;
    int                  nHeight;
    int                  nLeftOff;
    int                  nTopOff;
    int                  nDisplayWidth;
    int                  nDisplayHeight;

    int                  bHoldLastPictureFlag;
    int                  bVideoWithTwoStreamFlag;
    int                  bIsSoftDecoderFlag;

    int                  bLayerInitialized;
    int                  bProtectFlag;

    void*                pUserData;

    //* use when render derect to hardware layer.
    VPictureNode         picNodes[NUM_OF_PICTURE_NODES];

    int                  nGpuBufferCount;
    BufferInfoT          mBufferInfo[GPU_BUFFER_NUM];
    int                  bLayerShowed;

    int                  fdDisplay;
    int                  nScreenWidth;
    int                  nScreenHeight;
    int screen_x,screen_y,screen_w,screen_h;
    int regionScale;
    int bHdrVideoFlag;
    int b10BitVideoFlag;
    int bAfbcModeFlag;
    int nLbcLossyComMod;//1:1.5x; 2:2x; 3:2.5x;
    unsigned int bIsLossy;       //lossy compression or not
    unsigned int bRcEn;          //compact storage or not
    int nLayerBufferNum;
    int nLayerID;

    // render thread
    pthread_mutex_t      mutex;
    pthread_cond_t       cond_dequeue;
    pthread_cond_t       cond_queue;
    bool                 running;
    pthread_t            renderThread;
    int                  fdFrameBuffer;  // for waiting vsync
    VPictureNode*        prePicNode;
    VPictureNode*        currentPicNode;
    int                  currentFenceFd;
    int                  preFenceFd;
    struct list_head     picNodeListHead;
    unsigned int         count;// member of struct sync_info

    int                  bResolutionChange;
    int                  bResolutionChanging;
    int                  bIsMiracast;
    int                  nDegree;//positive or negative number, here we choose between  0 and 360
    int                  bNeedCrop;
    int                  cropRegion[4]; //x,y,width,height
    int                  cropLT; //left or top
    int                  bScreenRotating;
    int                  crop_x;
    int                  crop_y;
    int                  crop_width;
    int                  crop_height;
    int                  bFullScreen;
    int                  cropInfos[CROP_INFO_NUM];
    int                  cropInfosIndex;
    int                  zorder;
}LayerContext;

static unsigned int getLbcModBuferrSize(LayerContext* lc)
{
	// int nLbcBufferSize = 0;
	 int cmp_ratio = 0;
	 int seg_wth = 16;
	 int seg_hgt = 4;
	 int bit_depth = 8;
	 //int ALIGN = 128;
	 int seg_tar_bits;
	 int segline_tar_bits;
	 int y_mode_bits, c_mode_bits, y_data_bits, c_data_bits;
	 int info_buffer_size;
	 int frm_wth = lc->nWidth;
	 int frm_hgt = (lc->nHeight+15) &~ 15;
	 unsigned int bufferSize;
	 int tmpA, tmpB;

	 if(lc->b10BitVideoFlag == 1)
	 {
	   bit_depth = 10;
	 }

	 if(lc->nLbcLossyComMod == 1)//1.5x
	 {
	   cmp_ratio = 667;
	 }
	 else if(lc->nLbcLossyComMod == 2)//2x
	 {
	  cmp_ratio = 500;
	 }
	 else if(lc->nLbcLossyComMod == 3)//2.5x
	 {
	   cmp_ratio = 400;
	 }

	 if(lc->bIsLossy)
	 {
		 seg_tar_bits = ((seg_wth * seg_hgt * bit_depth * cmp_ratio * 3 / 2000) / ALIGN) * ALIGN;
		 if(lc->bRcEn)
		 {
		    segline_tar_bits = ((frm_wth + seg_wth - 1) / seg_wth * seg_wth * seg_hgt * bit_depth * cmp_ratio * 3 / 2000 + 
ALIGN - 1) / ALIGN * ALIGN;
		 }
		 else
		 {
		    segline_tar_bits = ((frm_wth + seg_wth - 1) / seg_wth) * seg_tar_bits;
		 }
	 }
	 else
	 {
		 y_mode_bits = seg_wth / MB_WTH * (CODE_MODE_Y_BITS * 2 + BLK_MODE_BITS);
		 c_mode_bits = 2 * (seg_wth / 2 / MB_WTH * CODE_MODE_C_BITS);
		 y_data_bits = seg_wth * seg_hgt * bit_depth;
		 c_data_bits = seg_wth * seg_hgt * bit_depth / 2 + 2 * (seg_wth / 2 / MB_WTH) * C_DTS_BITS;
		 seg_tar_bits= (y_data_bits + c_data_bits + y_mode_bits + c_mode_bits + ALIGN - 1) / ALIGN * ALIGN;
		 segline_tar_bits = ((frm_wth + seg_wth - 1) / seg_wth * seg_wth / seg_wth) * seg_tar_bits;
	 }

	 tmpA = ((((frm_wth + seg_wth - 1) / seg_wth + 7) / 8) * 8 * 16 * (frm_hgt /seg_hgt) + 7) / 8;
	 tmpB = ((((frm_hgt + seg_wth - 1) / seg_wth + 7) / 8) * 8 * 16 * (frm_wth /seg_hgt) + 7) / 8;
	 info_buffer_size = (tmpA > tmpB) ? tmpA : tmpB;
	 bufferSize = (segline_tar_bits * frm_hgt / seg_hgt + 7) /8 + info_buffer_size;
	 CDX_LOGD("the_lbc:cmp_ratio=%d, buffer size=%d", cmp_ratio, bufferSize);

	 return bufferSize;
}


//* set usage, scaling_mode, pixelFormat, buffers_geometry, buffers_count, crop
static int setLayerBuffer(LayerContext* lc, int width, int height)
{
    CDX_LOGD("setLayerBuffer: PixelFormat(%d), nW(%d), nH(%d)",
          lc->eDisplayPixelFormat, width, height);
    CDX_LOGD("setLayerBuffer: buffercount(%d), bProtectFlag(%d), bIsSoftDecoderFlag(%d)",
          lc->nGpuBufferCount,lc->bProtectFlag,lc->bIsSoftDecoderFlag);

    int          pixelFormat;
    unsigned int nGpuBufWidth;
    unsigned int nGpuBufHeight;
    int i = 0;
    char* pVirBuf;
    char* pPhyBuf;
    int   nBufFd;

    int   nMemSizeY;
    int   nMemSizeC;

    char* pMeteVirBuf;
    char* pMetePhyBuf;
    int   nMeteBufFd;
    int   nTotalPicPhyBufSize = 0;

    nGpuBufWidth  = width;  //* restore nGpuBufWidth to mWidth;
    nGpuBufHeight = height;

    //* We should double the height if the video with two stream,
    //* so the nativeWindow will malloc double buffer
    if(lc->bVideoWithTwoStreamFlag == 1)
    {
        nGpuBufHeight = 2*nGpuBufHeight;
    }

    if(lc->nGpuBufferCount <= 0)
    {
        loge("error: the lc->nGpuBufferCount[%d] is invalid!",lc->nGpuBufferCount);
        return -1;
    }

    for(i=0; i<lc->nGpuBufferCount; i++)
    {
        nMemSizeY = nGpuBufWidth*nGpuBufHeight;

        if(lc->eDisplayPixelFormat == PIXEL_FORMAT_YUV_PLANER_420 ||
           lc->eDisplayPixelFormat == PIXEL_FORMAT_YV12 ||
           lc->eDisplayPixelFormat == PIXEL_FORMAT_NV21)
            nMemSizeC = nMemSizeY>>2;
        else if(lc->eDisplayPixelFormat == PIXEL_FORMAT_YUV_PLANER_422)
            nMemSizeC = nMemSizeY>>1;
        else
            nMemSizeC = nMemSizeY;  //* PIXEL_FORMAT_YUV_PLANER_444

        int nLower2BitBufSize = 0;
        int nAfbcBufSize = 0;
        int nNormalYuvBufSize = nMemSizeY + nMemSizeC*2;
        int nTotalPicPhyBufSize = 0;
        int frmbuf_c_size = 0;

        nLower2BitBufSize = ((((nGpuBufWidth+3)>>2) + 31) & 0xffffffe0) * nGpuBufHeight * 3/2;
        //int PriChromaStride = ((nGpuBufWidth/2) + 31)&0xffffffe0;
        //frmbuf_c_size = 2 * (PriChromaStride * (((nGpuBufHeight/2)+15)&0xfffffff0)/4);
        if(lc->b10BitVideoFlag)
        {
            if(lc->bAfbcModeFlag == 1)
            {
                nAfbcBufSize = ((nGpuBufWidth+15)>>4) * ((nGpuBufHeight+4+15)>>4) * (512 + 16) + 32 + 1024;
                nTotalPicPhyBufSize = nAfbcBufSize + frmbuf_c_size + nLower2BitBufSize;
                logd("nTotalSize = %d, nAfbcBufSize = %d,\
                     frmbuf_c_size = %d, nLower2BitBufSize = %d",
                     nTotalPicPhyBufSize, nAfbcBufSize,
                     frmbuf_c_size, nLower2BitBufSize);
            }
            else
            {
                nTotalPicPhyBufSize = nNormalYuvBufSize + nLower2BitBufSize;
            }
        }
        else
        {
            if(lc->bAfbcModeFlag == 1)
            {
                nAfbcBufSize = ((nGpuBufWidth+15)>>4) * ((nGpuBufHeight+4+15)>>4) * (384 + 16) + 32 + 1024;
                nTotalPicPhyBufSize = nAfbcBufSize + frmbuf_c_size;
                logd("the_afbc,nAfbcBufSize=%d",nAfbcBufSize);
            }
            else
            {
                nTotalPicPhyBufSize = nNormalYuvBufSize;
            }
        }

        if(lc->nLbcLossyComMod > 0)
        {
            nTotalPicPhyBufSize = getLbcModBuferrSize(lc);
            lc->mBufferInfo[i].pPicture.nLbcSize = nTotalPicPhyBufSize;
        }

        pVirBuf = CdxIonPalloc(nTotalPicPhyBufSize);

        memset(pVirBuf, 0x10, nTotalPicPhyBufSize);

        if (pVirBuf == NULL)
        {
			loge("CdxIonPalloc failure");
        	goto err_out;
        }

        pPhyBuf = CdxIonVir2Phy(pVirBuf);
        nBufFd  = CdxIonVir2Fd(pVirBuf);

        if(lc->b10BitVideoFlag)
        {
            if(lc->bAfbcModeFlag== 1)
            {
                lc->mBufferInfo[i].pPicture.nLower2BitBufSize  = nLower2BitBufSize;
                lc->mBufferInfo[i].pPicture.nLower2BitBufOffset = nAfbcBufSize + frmbuf_c_size;
                lc->mBufferInfo[i].pPicture.nLower2BitBufStride = ((((nGpuBufWidth+3)>>2) + 31) & 0xffffffe0);
                lc->mBufferInfo[i].pPicture.nAfbcSize = nAfbcBufSize;
            }
            else
            {
                lc->mBufferInfo[i].pPicture.pData1 = pVirBuf + nMemSizeY;
                lc->mBufferInfo[i].pPicture.nLower2BitBufSize  = nLower2BitBufSize;
                lc->mBufferInfo[i].pPicture.nLower2BitBufOffset = nNormalYuvBufSize;
                lc->mBufferInfo[i].pPicture.nLower2BitBufStride = ((((nGpuBufWidth+3)>>2) + 31) & 0xffffffe0);
            }
        }
        else
        {
            if(lc->bAfbcModeFlag== 1)
            {
                lc->mBufferInfo[i].pPicture.nAfbcSize = nAfbcBufSize;
                logd("the_afbc,pVirBuf=%p",pVirBuf);
            }
            else
            {
                lc->mBufferInfo[i].pPicture.pData1 = pVirBuf + nMemSizeY;
                if(lc->eDisplayPixelFormat != PIXEL_FORMAT_NV21)
                    lc->mBufferInfo[i].pPicture.pData2 = pVirBuf + nMemSizeY + nMemSizeC;
                else
                    lc->mBufferInfo[i].pPicture.pData2 = NULL;
            }
        }

        lc->mBufferInfo[i].nUsedFlag    = 0;
        lc->mBufferInfo[i].pPicture.nWidth  = width;
        lc->mBufferInfo[i].pPicture.nHeight = height;
        lc->mBufferInfo[i].pPicture.nLineStride  = width;
        lc->mBufferInfo[i].pPicture.pData0       = pVirBuf;
        lc->mBufferInfo[i].pPicture.pData3 = NULL;
        lc->mBufferInfo[i].pPicture.phyYBufAddr  = (uintptr_t)pPhyBuf;
        lc->mBufferInfo[i].pPicture.phyCBufAddr  =
              lc->mBufferInfo[i].pPicture.phyYBufAddr + (height * width);
        lc->mBufferInfo[i].pPicture.nBufSize = nTotalPicPhyBufSize;
        lc->mBufferInfo[i].pPicture.nBufId       = i;
        lc->mBufferInfo[i].pPicture.nBufFd       = nBufFd;
        lc->mBufferInfo[i].pPicture.ePixelFormat = lc->eDisplayPixelFormat;

        if(lc->nLbcLossyComMod > 0)
        {
            lc->mBufferInfo[i].pPicture.pData1 = NULL;
            lc->mBufferInfo[i].pPicture.pData2 = NULL;
            lc->mBufferInfo[i].pPicture.phyCBufAddr = 0;
        }

        pMeteVirBuf = CdxIonPalloc(4096);
        pMetePhyBuf = CdxIonVir2Phy(pMeteVirBuf);
        nMeteBufFd  = CdxIonVir2Fd(pMeteVirBuf);
        //CDX_LOGD("pMetePhyBuf %p",pMetePhyBuf);

        if (pMeteVirBuf == NULL)
        {
            loge("error: ion buff allocate fail!");
            goto err_out;
        }

        lc->mBufferInfo[i].pPicture.pMetaData   = pMeteVirBuf;
        lc->mBufferInfo[i].pMetaDataVirAddr = pMeteVirBuf;
        lc->mBufferInfo[i].pMetaDataPhyAddr = pMetePhyBuf;
        lc->mBufferInfo[i].nMetaDataMapFd   = nMeteBufFd;


        CDX_LOGD("=== init id:%d pVirBuf: %p", i, pVirBuf);
    }

    return 0;

err_out:
    for (i=0; i<lc->nGpuBufferCount; i++)
    {
    	if (lc->mBufferInfo[i].pPicture.pData0)
        {
        	CdxIonPfree(lc->mBufferInfo[i].pPicture.pData0);
        	lc->mBufferInfo[i].pPicture.pData0 = NULL;
        }

        if (lc->mBufferInfo[i].pPicture.pMetaData)
        {
            CdxIonPfree(lc->mBufferInfo[i].pPicture.pMetaData);
            lc->mBufferInfo[i].pPicture.pMetaData = NULL;
        }
    }

	return -1;
}

int detectPictureXEdge(LayerContext* lc, char* yuv_data, int adjustdiff, int buffer_width, int detectStart,
                       int detectEnd) {
    logv("X detectStart:%d, detectEnd:%d", detectStart, detectEnd);
    // start from max valid picture range to reduce traverse point num
    for (int w = detectStart; w < detectEnd; w++) {
        int count = 0;
        // h+stride: reduce traverse point num
        // from startLimit to endLimit, we detect more and more careful by stride--
    #if 1
        int stride = 550 - (w - detectStart + 1) * 550 / (detectEnd - detectStart - 1);
        // the first column carefully checked to determine whether phone is horizontal or vertical
        if(w == detectStart)
            stride = 10;
    #else
        int stride = 20 - (w - detectStart) * 20 / (detectEnd - detectStart);
    #endif
        logv("stride:%d", stride);
        if (stride < 1) stride = 1;
        for (int h = lc->nTopOff; h < lc->nTopOff + lc->nDisplayHeight; h = h + stride) {
            int data = yuv_data[w + buffer_width * h];
            // read all the Y value and count num, in this situation,Y=16 means black color

            // black is 16 in Y channel, in order to crop suitable,
            // we think 14-19 is all black without valid picture
            if ((data < 14 || data > 19) && data != 0 && data != 255) {
                logv("data = %d, h = %d, w = %d, count = %d", data, h, w, count);
                count++;
                stride = 1;
                if (count > adjustdiff) {
                    return w;
                }
            }
        }
    }
    return lc->cropLT;
}

//H133 g2d use this
int detectPictureYEdge(LayerContext* lc, char* yuv_data, int adjustdiff, int buffer_width, int detectStart,
                       int detectEnd) {
    logv("Y detectStart:%d, detectEnd:%d", detectStart, detectEnd);
    // start from max valid picture range to reduce traverse point num
    for (int h = detectStart; h < detectEnd; h++) {
        int count = 0;
        // w+stride: reduce traverse point num
        // from startLimit to endLimit, we detect more and more careful by stride--
    #if 1
        int stride = 550 - (h - detectStart + 1) * 550 / (detectEnd - detectStart - 1);
        // the first line carefully checked to determine whether phone is horizontal or vertical
        if(h == detectStart)
            stride = 10;
    #else
        int stride = 20 - (h - detectStart) * 20 / (detectEnd - detectStart);
    #endif

        logv("stride:%d", stride);
        if (stride < 1) stride = 1;
        for (int w = lc->nLeftOff; w < lc->nLeftOff + lc->nDisplayWidth; w = w + stride) {
            int data = yuv_data[w + buffer_width * h];
            // read all the Y value and count num, in this situation,Y=16 means black color

            // black is 16 in Y channel, in order to crop suitable,
            // we think 14-19 is all black without valid picture
            if ((data < 14 || data > 19) && data != 0 && data != 255) {
                logv("data = %d, h = %d, w = %d, count = %d", data, h, w, count);
                count++;
                stride = 1;
                if (count > adjustdiff) {
                    return h;
                }
            }
        }
    }
    return lc->cropLT;
}

int estimateCropInfo(LayerContext* lc, VideoPicture* pPicture, int adjustdiff) {
    int buffer_width = pPicture->nWidth;
    int buffer_height = pPicture->nHeight;
    int detectStart, detectEnd, cropLT;
    char* yuv_data = pPicture->pData0;
    // miracast: detect whether the picture has been rotated, width > height:no, height > width:yes
    // for g2d and decoder rotation solution: height > width, picture has been rotated
    // for gpu and android rotation solution: width > height, picture hasn't been rotated
    if (buffer_width > buffer_height) {
        // Miracast: the resoltion height and width ratio range of phone is 15:9(800x480)-21:9(2400x1080), some tablets are 4:3. this is our
        // key aera to detect
        int startLimit = lc->nDisplayHeight * 3 / 4; // max valid picture height
        int endLimit = lc->nDisplayHeight * 9 / 21;   // min valid picture height

        detectStart = lc->nLeftOff + lc->nDisplayWidth / 2 - startLimit / 2;
        detectEnd = lc->nLeftOff + lc->nDisplayWidth / 2 - endLimit / 2;

        if(detectStart < 1 || detectEnd < 0)
        {
            loge("invaild detectStart: %d, detectEnd: %d", detectStart, detectEnd);
            return -1;
        }
        cropLT = detectPictureXEdge(lc, yuv_data, adjustdiff, buffer_width, detectStart-1, detectEnd);
    } else {
        // Miracast: the resoltion height and width ratio range of phone is 15:9(800x480)-21:9(2400x1080), some tablets are 4:3. this is our
        // key aera to detect
        int startLimit = lc->nDisplayWidth * 3 / 4; // max valid picture height
        int endLimit = lc->nDisplayWidth * 9 / 21;   // min valid picture height

        detectStart = lc->nTopOff + lc->nDisplayHeight / 2 - startLimit / 2;
        detectEnd = lc->nTopOff + lc->nDisplayHeight / 2 - endLimit / 2;

        if(detectStart < 1 || detectEnd < 0)
        {
            loge("invaild detectStart: %d, detectEnd: %d", detectStart, detectEnd);
            return -1;
        }
        cropLT = detectPictureYEdge(lc, yuv_data, adjustdiff, buffer_width, detectStart-1, detectEnd);
    }

    if (cropLT > detectStart) {
        if (lc->cropLT == 0) {
            lc->cropLT = cropLT;
            lc->bNeedCrop = 1;
            logd("previous cropLT = 0, crop change:%d", cropLT);
            lc->cropInfosIndex = 0;
        } else {
            if (lc->cropInfosIndex < CROP_INFO_NUM) {
                lc->cropInfos[lc->cropInfosIndex] = cropLT;
                lc->cropInfosIndex++;
                int sumCropInfos = 0;
                for (int i = 0; i < lc->cropInfosIndex; i++) {
                    sumCropInfos = sumCropInfos + lc->cropInfos[i];
                }
                lc->cropLT = sumCropInfos / lc->cropInfosIndex;
                if (lc->cropInfosIndex % 20 == 0) logd("portrain crop change:%d", cropLT);
            }
            lc->bNeedCrop = 1;
        }
    } else {
        cropLT = 0;
        lc->cropInfosIndex = 0;
        if (cropLT != lc->cropLT) {
            lc->cropLT = cropLT;
            lc->bNeedCrop = 1;
            logd("landscape crop change:%d", cropLT);
        }
    }

    if (lc->bScreenRotating) { // screen is rotating
        logd("screen has rotated, we need update crop parameters");
        lc->bScreenRotating = 0;
        lc->bNeedCrop = 1;
    }

    if (lc->bResolutionChanging && !lc->bResolutionChange) { // pc's resolution is changing
        logd("resolution has changed, we need update crop parameters");
        lc->bResolutionChanging = 0;
        lc->bNeedCrop = 1;
    }

    logv("bNeedCrop: %d\n", lc->bNeedCrop);
    if (lc->bNeedCrop == 1) {
        if (buffer_width > buffer_height) {
            lc->cropRegion[0] = lc->cropLT;
            lc->cropRegion[1] = lc->nTopOff;
            lc->cropRegion[2] = lc->nLeftOff + lc->nDisplayWidth - (lc->cropLT - lc->nLeftOff);
            lc->cropRegion[3] = lc->nTopOff + lc->nDisplayHeight;
        } else {
            lc->cropRegion[0] = lc->nLeftOff;
            lc->cropRegion[1] = lc->cropLT;
            lc->cropRegion[2] = lc->nLeftOff + lc->nDisplayWidth;
            lc->cropRegion[3] = lc->nTopOff + lc->nDisplayHeight - (lc->cropLT - lc->nTopOff);
        }
        if (lc->cropInfosIndex % 50/*10*/ == 0)
            logw("new cropRegion: %d, %d, %d, %d", lc->cropRegion[0], lc->cropRegion[1], lc->cropRegion[2],
                 lc->cropRegion[3]);
    }
    return 1;
}

static int isNeedBlackBorderCheck(LayerContext* lc, VideoPicture* pPicture)
{
    //logd("bIsMiracast: %d, p w: %d, p h: %d, s w: %d, s h: %d\n", lc->bIsMiracast, pPicture->nWidth, \
            pPicture->nHeight, lc->nScreenWidth, lc->nScreenHeight);
    if(lc->bIsMiracast == 1)
    {
        if(lc->nDegree == 90 || lc->nDegree == 270)
        {
            return (pPicture->nWidth > pPicture->nHeight && lc->nScreenWidth > lc->nScreenHeight) || \
            (pPicture->nWidth < pPicture->nHeight && lc->nScreenWidth < lc->nScreenHeight);
        }
        else
        {
            return (pPicture->nWidth < pPicture->nHeight && lc->nScreenWidth > lc->nScreenHeight) || \
            (pPicture->nWidth > pPicture->nHeight && lc->nScreenWidth < lc->nScreenHeight);
        }
    }
    return 0;
}

static int SetLayerParam(LayerContext* lc, VideoPicture* pPicture)
{
    struct disp_layer_config2 config;
    unsigned long     args[4];
    int ret = 0;
    if (pPicture == NULL) {
        CDX_LOGE("pPicture == NULL, just return.");
        return -1;
    }
    if (pPicture->pData0 == NULL) {
        CDX_LOGE("pPicture->pData0 == NULL, maybe resolution changed, just return.");
        return -1;
    }
    if (lc->bResolutionChange == 1) {
        CDX_LOGE("resolution is changing, ignore the last picture!");
        return -1;
    }

    // close the layer first, otherwise, in case when last frame is kept showing,
    // the picture showed will not valid because parameters changed.
    memset(&config.info, 0, sizeof(struct disp_layer_info2));
    if(lc->bLayerShowed == 1)
    {
        lc->bLayerShowed = 0;
        //TO DO.
    }

    //* transform pixel format.
    switch(lc->eDisplayPixelFormat)
    {
        case PIXEL_FORMAT_YUV_PLANER_420:
            config.info.fb.format = DISP_FORMAT_YUV420_P;
            config.info.fb.size[0].width     = pPicture->nWidth;
            config.info.fb.size[0].height    = pPicture->nHeight;
            config.info.fb.size[1].width     = pPicture->nWidth/2;
            config.info.fb.size[1].height    = pPicture->nHeight/2;
            config.info.fb.size[2].width     = pPicture->nWidth/2;
            config.info.fb.size[2].height    = pPicture->nHeight/2;
        break;

        case PIXEL_FORMAT_YV12:
            config.info.fb.format = DISP_FORMAT_YUV420_P;
            config.info.fb.size[0].width     = pPicture->nWidth;
            config.info.fb.size[0].height    = pPicture->nHeight;
            config.info.fb.size[1].width     = pPicture->nWidth/2;
            config.info.fb.size[1].height    = pPicture->nHeight/2;
            config.info.fb.size[2].width     = pPicture->nWidth/2;
            config.info.fb.size[2].height    = pPicture->nHeight/2;
        break;

        case PIXEL_FORMAT_NV12:
            config.info.fb.format = DISP_FORMAT_YUV420_SP_UVUV;
            config.info.fb.size[0].width     = pPicture->nWidth;
            config.info.fb.size[0].height    = pPicture->nHeight;

            config.info.fb.size[1].width     = pPicture->nWidth/2;
            config.info.fb.size[1].height    = pPicture->nHeight/2;
        break;

        case PIXEL_FORMAT_NV21:
            config.info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
            config.info.fb.size[0].width     = pPicture->nWidth;
            config.info.fb.size[0].height    = pPicture->nHeight;
            config.info.fb.size[1].width     = pPicture->nWidth/2;
            config.info.fb.size[1].height    = pPicture->nHeight/2;
        break;

        default:
        {
            loge("unsupported pixel format.");
            return -1;
        }
    }

    // miracast set crop info
#ifdef CONF_SUPPORT_G2D
    // nDegree could not sync with picture, so use width and height to decide whether rotate
    // for example, SetLayerParam is probably processing landscape picture when set degree to 90 or 270
    if (isNeedBlackBorderCheck(lc, pPicture))
    //if (lc->bIsMiracast == 1 && ((pPicture->nWidth < pPicture->nHeight && lc->nScreenWidth > lc->nScreenHeight) || \
                                     (pPicture->nWidth > pPicture->nHeight && lc->nScreenWidth < lc->nScreenHeight)))
#else
    if (lc->bIsMiracast == 1 && (lc->nDegree == 90 || lc->nDegree == 270)) 
#endif
	{
        if(estimateCropInfo(lc, pPicture, 5 /*3*/) == -1)
            return -1;

        if (lc->bNeedCrop) {
            logv("set crop to adapt screen");
            lc->crop_x = lc->cropRegion[0];
            lc->crop_y = lc->cropRegion[1];
            lc->crop_width = lc->cropRegion[2] - lc->cropRegion[0];
            lc->crop_height = lc->cropRegion[3] - lc->cropRegion[1];
            lc->bNeedCrop = 0;
        }
        #if 0 //if picture is landscape, lc->bNeedCrop is 0, but if need cut picture black edge above and below to full screen, need to turn it on
        else {
            lc->crop_x = pPicture->nLeftOffset;
            lc->crop_y = pPicture->nTopOffset;
            lc->crop_width = pPicture->nWidth;
            lc->crop_height = pPicture->nHeight;

            char* yuv_data = pPicture->pData0;
            logv("pPicture->nWidth: %d\n", pPicture->nWidth);
            int xcropLT = detectPictureXEdge(lc, yuv_data, 3, pPicture->nWidth, 3, 200);
            if (xcropLT > 3) {
                lc->crop_x = xcropLT;
                lc->crop_width = lc->nLeftOff + lc->nDisplayWidth - (xcropLT - lc->nLeftOff) - xcropLT;
                //logd("xcropLT:%d, lc->crop_width: %d", xcropLT, lc->crop_width);
            }
        }
        #endif
    } else {
        // default crop info is picture display region
        lc->crop_x = pPicture->nLeftOffset;
        lc->crop_y = pPicture->nTopOffset;
        lc->crop_width = pPicture->nRightOffset - pPicture->nLeftOffset;
        lc->crop_height = pPicture->nBottomOffset - pPicture->nTopOffset;
    }

#if 0
    logd("x: %d, y: %d, cw: %d, ch: %d\n", lc->crop_x, lc->crop_y, lc->crop_width, lc->crop_height);
#endif

    // final set crop info
    config.info.fb.crop.x = (unsigned long long)lc->crop_x << 32;
    config.info.fb.crop.y = (unsigned long long)lc->crop_y << 32;
    config.info.fb.crop.width = (unsigned long long)lc->crop_width << 32;
    config.info.fb.crop.height = (unsigned long long)lc->crop_height << 32;
    config.info.fb.color_space = (pPicture->nHeight < 720) ? DISP_BT601 : DISP_BT709;

    //set screen display region
    if (lc->screen_w == 0 || lc->screen_h == 0) {
        int scaleW = lc->nScreenWidth  * lc->regionScale / 100;
        int scaleH = lc->nScreenHeight * lc->regionScale / 100;
        int cropDisplayWidth = lc->crop_width;
        int cropDisplayHeight = lc->crop_height;

        config.info.screen_win.x        = 0 + (lc->nScreenWidth - scaleW)/2;
        config.info.screen_win.y        = 0 + (lc->nScreenHeight -  scaleH)/2;
        config.info.screen_win.width    = scaleW;
        config.info.screen_win.height   = scaleH;

        float displayRatio = cropDisplayWidth*1.0/cropDisplayHeight;
        float screenRatio  = lc->nScreenWidth*1.0/lc->nScreenHeight;
        if(displayRatio - screenRatio > JUDGE_DIFF_THRESHOLD)
        {
            config.info.screen_win.height  = cropDisplayHeight * scaleW / cropDisplayWidth;
            config.info.screen_win.y       = (lc->nScreenHeight - config.info.screen_win.height)/2;
        }
        else if(screenRatio - displayRatio > JUDGE_DIFF_THRESHOLD)
        {
            config.info.screen_win.width   = cropDisplayWidth * scaleH /cropDisplayHeight;
            config.info.screen_win.x       = (lc->nScreenWidth - config.info.screen_win.width )/2;
        }
        #ifdef CONF_SUPPORT_G2D
        if(lc->bFullScreen == 1)
        {
            config.info.screen_win.x = 0;
            config.info.screen_win.y = 0;
            config.info.screen_win.width = lc->nScreenWidth;
            config.info.screen_win.height = lc->nScreenHeight;
        }
        #else
        if (lc->nDegree == 90 || lc->nDegree == 270) {
            if (lc->bFullScreen == 1) { // portrait full screen
                config.info.screen_win.y = 0 + (lc->nScreenHeight - scaleH) / 2;
                config.info.screen_win.height = scaleH;
            }
        }
        #endif
    } else {
        config.info.screen_win.x        = lc->screen_x;
        config.info.screen_win.y        = lc->screen_y;
        config.info.screen_win.width    = lc->screen_w;
        config.info.screen_win.height   = lc->screen_h;
    }

#if 0
    logd("crop '%d', '%d', '%d', '%d'",
		config.info.screen_win.x, config.info.screen_win.y,
		config.info.screen_win.width, config.info.screen_win.height);
#endif

    int i;
    for(i = 0; i < GPU_BUFFER_NUM; i++)
    {
        if(lc->mBufferInfo[i].pMetaDataVirAddr == pPicture->pMetaData)
        {
            config.info.fb.metadata_fd = lc->mBufferInfo[i].nMetaDataMapFd;
            config.info.fb.metadata_size = 4096;
            if (lc->bAfbcModeFlag)
            {
                config.info.fb.metadata_flag = 1<<4;
                config.info.fb.fbd_en = 1;
            }
            if (lc->bHdrVideoFlag)
            {
                config.info.fb.metadata_flag = config.info.fb.metadata_flag|(1<<1);
            }
            break;
        }
    }

    config.info.fb.fd = pPicture->nBufFd;
    config.info.alpha_mode          = 1;
    if (pPicture->nWidth == SWITCH_BUFFER_SIZE) {
        config.info.screen_win.x        = 0;
        config.info.screen_win.y        = 0;
        config.info.screen_win.width    = SWITCH_BUFFER_SIZE;
        config.info.screen_win.height   = SWITCH_BUFFER_SIZE;
        config.info.zorder          = 0;//try to hide the switchbuffer layer
    } else {
        config.info.zorder          = lc->zorder;
    }

    /*
     * alpha_mode: 0: pixel alpha;  1: global alpha;  2: global pixel alpha (mixed alpha)
     * alpha value effect when alpha_mode = 1(global alpha) or alpha_mode = 2(global pixel alpha)
     * video channel 0 no alpha mode, other video channel only support global alpha;
     * ui channel support both pixel and global alpha;
     */
    config.info.alpha_value         = 0xff;
    config.info.mode            = LAYER_MODE_BUFFER;
    config.channel = 0;
    config.enable = 1;
    config.layer_id = lc->nLayerID;
    config.info.id = lc->count;
    //* set layerInfo to the driver.
    //args[1] = (unsigned long)(&config);
    //args[2] = 1;
    //ret = ioctl(lc->fdDisplay, DISP_LAYER_SET_CONFIG2, (void*)args);
    // lbc config
#if 1
        CDX_LOGV("lbc_mode = %d, width = %d, height = %d", lc->nLbcLossyComMod, pPicture->nWidth,
                pPicture->nHeight);
        if (lc->nLbcLossyComMod > 0) {
            u32 seg_width = 16, seg_height = 4;
            u32 bit_depth = 8;
            u32 cmp_ratio = 0;
            u32 seg_tar_bits, seg_tar_bits_out;
            int y_mode_bits, c_mode_bits, y_data_bits, c_data_bits;
            u32 segline_tar_bits = 0, segline_tar_bits_out;
            int frm_width = pPicture->nWidth;

            if (lc->b10BitVideoFlag == 1) {
                bit_depth = 10;
            }

            if (lc->nLbcLossyComMod == 1) //1.5x
                    {
                cmp_ratio = 667;
            } else if (lc->nLbcLossyComMod == 2) //2x
                    {
                cmp_ratio = 500;
            } else if (lc->nLbcLossyComMod == 3) //2.5x
                    {
                cmp_ratio = 400;
            }

            config.info.fb.lbc_en = 1;
            config.info.fb.lbc_info.is_lossy = lc->bIsLossy;
            config.info.fb.lbc_info.rc_en = lc->bRcEn;

            if (config.info.fb.lbc_info.is_lossy) {
                seg_tar_bits = ((seg_width * seg_height * bit_depth * cmp_ratio * 3
                        / 2000) / ALIGN) * ALIGN;
                if (config.info.fb.lbc_info.rc_en == 0) {
                    segline_tar_bits = ((frm_width + seg_width - 1) / seg_width)
                            * seg_tar_bits;
                } else if (config.info.fb.lbc_info.rc_en == 1) {
                    segline_tar_bits = ((frm_width + seg_width - 1) / seg_width
                            * seg_width * seg_height * bit_depth * cmp_ratio * 3
                            / 2000 + ALIGN - 1) / ALIGN * ALIGN;
                }
            } else {
                y_mode_bits = seg_width / MB_WTH
                        * (CODE_MODE_Y_BITS * 2 + BLK_MODE_BITS);
                c_mode_bits = 2 * (seg_width / 2 / MB_WTH * CODE_MODE_C_BITS);
                y_data_bits = seg_width * seg_height * bit_depth;
                c_data_bits = seg_width * seg_height * bit_depth
                        / 2+ 2 * (seg_width / 2 / MB_WTH) * C_DTS_BITS;
                seg_tar_bits = (y_data_bits + c_data_bits + y_mode_bits
                        + c_mode_bits + ALIGN - 1) / ALIGN * ALIGN;
                segline_tar_bits = ((frm_width + seg_width - 1) / seg_width
                        * seg_width / seg_width) * seg_tar_bits;
            }

        #if 1
            seg_tar_bits_out = seg_tar_bits;
            segline_tar_bits_out = segline_tar_bits;
        #else
            seg_tar_bits_out = seg_tar_bits / 8;
            segline_tar_bits_out = segline_tar_bits / 8;
        #endif

            config.info.fb.lbc_info.pitch = segline_tar_bits_out;
            config.info.fb.lbc_info.seg_bit = seg_tar_bits_out;
        } else
            config.info.fb.lbc_en = 0;
#endif
    ret = submitLayer(0, lc->fdDisplay, lc->count, &config, 1);

//    char displayChars[4]  = "";
//    if (display_uci_get_config("region_scale", displayChars, sizeof(displayChars)) != -1)
//        lc->regionScale = atoi(displayChars);
//    else
//    {
//        lc->regionScale = 100;
//        logv("get display region scale fail,use default value %d !",lc->regionScale);
//    }
//    args[0] = 0;
//    lc->nScreenWidth  = ioctl(lc->fdDisplay, DISP_GET_SCN_WIDTH, (void *)args);
//    lc->nScreenHeight = ioctl(lc->fdDisplay, DISP_GET_SCN_HEIGHT,(void *)args);
    if(0 != ret)
    	loge("fail to set layer info!");

    return 0;
}

//* Description: set initial param -- video whether have hdr info or not
static int LayerSetHdrInfo(LayerCtrl *l, const FbmBufInfo *fbmInfo)
{
	if (!fbmInfo)
	{
		loge("fbmInfo is null");
		return -1;
	}
	LayerContext* lc;
	lc = (LayerContext*)l;
	lc->bHdrVideoFlag = fbmInfo->bHdrVideoFlag;
	lc->b10BitVideoFlag = fbmInfo->b10bitVideoFlag;
	lc->bAfbcModeFlag = fbmInfo->bAfbcModeFlag;
	return 0;
}

static int __LayerReset(LayerCtrl* l)
{
    LayerContext* lc;
    int i;

    logd("LayerReset.");

    lc = (LayerContext*)l;

    struct disp_layer_config2 config;
    unsigned long args[4];
    memset(&config.info, 0, sizeof(struct disp_layer_info2));
    config.channel = 0;
    config.enable = 0;
    config.layer_id = lc->nLayerID;
    //* set layerInfo to the driver.
    args[0] = 0;
    args[1] = (unsigned long)(&config);
    args[2] = 1;

    ioctl(lc->fdDisplay, DISP_LAYER_SET_CONFIG2, (void*)args);
    usleep(20*1000);//make sure display disabled
    logd("layer reset,display disabled...");

    for(i=0; i<lc->nGpuBufferCount; i++)
    {
        CdxIonPfree(lc->mBufferInfo[i].pPicture.pData0);
        CdxIonPfree(lc->mBufferInfo[i].pPicture.pMetaData);
        lc->mBufferInfo[i].nUsedFlag = 0;
        lc->picNodes[i].bUsed        = 0;
        lc->picNodes[i].bValid       = 0;
        lc->picNodes[i].dequeueable  = 1;
        lc->picNodes[i].pPicture     = NULL;
    }
    INIT_LIST_HEAD(&lc->picNodeListHead);
    lc->bLayerInitialized = 0;
    return 0;
}


static void __LayerRelease(LayerCtrl* l)
{
    LayerContext* lc;
    int i;

    lc = (LayerContext*)l;

    logv("Layer release");
    struct disp_layer_config2 config;
    unsigned long args[4];
    memset(&config.info, 0, sizeof(struct disp_layer_info2));
    config.channel = 0;
    config.enable = 0;
    config.layer_id = lc->nLayerID;
    //* set layerInfo to the driver.
    args[0] = 0;
    args[1] = (unsigned long)(&config);
    args[2] = 1;

    ioctl(lc->fdDisplay, DISP_LAYER_SET_CONFIG2, (void*)args);
    usleep(20*1000);//make sure display disabled
    logd("layer release,display disabled...");

    for(i=0; i<lc->nGpuBufferCount; i++)
    {
        CdxIonPfree(lc->mBufferInfo[i].pPicture.pData0);
        CdxIonPfree(lc->mBufferInfo[i].pPicture.pMetaData);
        lc->mBufferInfo[i].nUsedFlag = 0;
        lc->picNodes[i].bUsed        = 0;
        lc->picNodes[i].bValid       = 0;
        lc->picNodes[i].dequeueable  = 1;
        lc->picNodes[i].pPicture     = NULL;
    }
    INIT_LIST_HEAD(&lc->picNodeListHead);
}

static void __LayerDestroy(LayerCtrl* l)
{
    logd("layer destroy");
    LayerContext* lc;
    lc = (LayerContext*)l;

    if (destroySyncTimeline(0, lc->fdDisplay) < 0){
        loge("destroySyncTimeline failed!");
    }
    if(lc->fdDisplay>=0)
        close(lc->fdDisplay);
    lc->fdDisplay=-1;
    CdxIonClose();
    lc->running = 0;
    pthread_mutex_lock(&lc->mutex);
    pthread_cond_signal(&lc->cond_queue);
    pthread_mutex_unlock(&lc->mutex);

    pthread_join(lc->renderThread, NULL);
    pthread_mutex_destroy(&lc->mutex);
    pthread_cond_destroy(&lc->cond_dequeue);
    pthread_cond_destroy(&lc->cond_queue);
    free(lc);
}


static int __LayerSetDisplayBufferSize(LayerCtrl* l, int nWidth, int nHeight)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

    lc->nWidth         = nWidth;
    lc->nHeight        = nHeight;

    if(lc->bVideoWithTwoStreamFlag == 1)
    {
        //* display the whole buffer region when 3D
        //* as we had init align-edge-region to black. so it will be look ok.
        int nScaler = 2;
        lc->nDisplayHeight = lc->nDisplayHeight*nScaler;
    }

    return 0;
}

//* Description: set initial param -- display region
static int __LayerSetDisplayRegion(LayerCtrl* l, int nLeftOff, int nTopOff,
                                        int nDisplayWidth, int nDisplayHeight)
{
    LayerContext* lc;

    lc = (LayerContext*)l;
    logv("Layer set display region, leftOffset = %d, topOffset = %d, "
    	"displayWidth = %d, displayHeight = %d",
            nLeftOff, nTopOff, nDisplayWidth, nDisplayHeight);

    if(nDisplayWidth == 0 && nDisplayHeight == 0)
    {
        return -1;
    }

    lc->nDisplayWidth = nDisplayWidth;
    lc->nDisplayHeight = nDisplayHeight;
    lc->nLeftOff = nLeftOff;
    lc->nTopOff = nTopOff;

    if(lc->bVideoWithTwoStreamFlag == 1)
    {
        //* display the whole buffer region when 3D
        //* as we had init align-edge-region to black. so it will be look ok.
        int nScaler = 2;
        lc->nDisplayHeight = lc->nHeight*nScaler;
    }

    return 0;
}

//* Description: set initial param -- screen display region
static int __LayerSetScreenRegion(LayerCtrl* l, int x, int y,
                                        int width, int height)
{
    LayerContext* lc;

    lc = (LayerContext*)l;
    logd("Layer set screen region, leftOffset = %d, topOffset = %d, "
    	"screenWidth = %d, screenHeight = %d",
            x, y, width, height);

    if(width == 0 || height == 0)
    {
        return -1;
    }

    lc->screen_x = x;
    lc->screen_y = y;
    lc->screen_w = width;
    lc->screen_h = height;

    return 0;
}

static int switchBuffer(LayerContext* lc)
{
    int64_t start=CdxGetNowUs();
    int i=0, j=0, err=0;
    int width = SWITCH_BUFFER_SIZE;
    int height = SWITCH_BUFFER_SIZE;
    int YSize = width * height;
    void* pDataBuf = NULL;

    //set buffer display region of switchBuffer
    __LayerSetDisplayRegion(lc, 0, 0, width, height);

    //allocate buffers
    if(setLayerBuffer(lc, width, height) != 0)
    {
        loge("can not alloc %dx%d buffer to replace the last video larger buffers, switchBuffer failed", SWITCH_BUFFER_SIZE, SWITCH_BUFFER_SIZE);
        return -1;
    }

    for(i = 0; i < lc->nGpuBufferCount; i++)
    {
        pDataBuf = lc->mBufferInfo[i].pPicture.pData0;
        if(lc->eDisplayPixelFormat == PIXEL_FORMAT_P010_UV ||
           lc->eDisplayPixelFormat == PIXEL_FORMAT_P010_VU)
        {
            memset(pDataBuf, 0x10, YSize*2);
            memset((char*)pDataBuf+YSize*2, 0x80, YSize);
        }
        else
        {//go here
            memset(pDataBuf, 0x10, YSize);
            memset((char*)pDataBuf+YSize, 0x80, YSize/2);
        }

        if (SetLayerParam(lc, &lc->mBufferInfo[i].pPicture) != 0) {
            loge("switchBuffer: can not render frame");
        }
        //wait for display driver to replace the last play buffer.
        usleep(10*1000);
    }
    __LayerRelease(lc);
    logw("switchBuffer cost time : %lldus", CdxGetNowUs()-start);
    return 0;
}

//* Description: set initial param -- display pixelFormat
static int __LayerSetDisplayPixelFormat(LayerCtrl* l, enum EPIXELFORMAT ePixelFormat)
{
    LayerContext* lc;

    lc = (LayerContext*)l;
    logd("Layer set expected pixel format, format = %d", (int)ePixelFormat);

    if(ePixelFormat == PIXEL_FORMAT_NV12 ||
       ePixelFormat == PIXEL_FORMAT_NV21 ||
       ePixelFormat == PIXEL_FORMAT_YV12)           //* add new pixel formats supported by gpu here.
    {
        lc->eDisplayPixelFormat = ePixelFormat;
    }
    else
    {
        logv("receive pixel format is %d, not match.", lc->eDisplayPixelFormat);
        return -1;
    }

    return 0;
}

//* Description: set initial param -- deinterlace flag
static int __LayerSetDeinterlaceFlag(LayerCtrl* l,int bFlag)
{
    LayerContext* lc;
    (void)bFlag;
    lc = (LayerContext*)l;

    return 0;
}

//* Description: set buffer timestamp -- set this param every frame
static int __LayerSetBufferTimeStamp(LayerCtrl* l, int64_t nPtsAbs)
{
    LayerContext* lc;
    (void)nPtsAbs;

    lc = (LayerContext*)l;

    return 0;
}

static int __LayerGetRotationAngle(LayerCtrl* l)
{
    LayerContext* lc;
    int nRotationAngle = 0;

    lc = (LayerContext*)l;

    return 0;
}

static int __LayerCtrlShowVideo(LayerCtrl* l)
{
    LayerContext* lc;
    int               i;

    lc = (LayerContext*)l;

    lc->bLayerShowed = 1;

    return 0;
}

static int __LayerCtrlHideVideo(LayerCtrl* l)
{
    LayerContext* lc;
    int               i;

    lc = (LayerContext*)l;

    lc->bLayerShowed = 0;

    return 0;
}

static int __LayerCtrlIsVideoShow(LayerCtrl* l)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

     return lc->bLayerShowed;
}

static int  __LayerCtrlHoldLastPicture(LayerCtrl* l, int bHold)
{
    logd("LayerCtrlHoldLastPicture, bHold = %d", bHold);

    LayerContext* lc;
    lc = (LayerContext*)l;

    return 0;
}

static VPictureNode* getDequeueableNode(LayerContext* lc) {
    VPictureNode* node = NULL;
    list_for_each_entry(node, &lc->picNodeListHead, list){
        if(node->dequeueable){
            list_del(&node->list);
            node->bUsed = 0;
            return node;
        }
    }
    return NULL;
}

static int LayerDequeueBufferLocked(LayerCtrl* l, VideoPicture** ppVideoPicture, int bInitFlag)
{
    LayerContext* lc;
    int i = 0;
    VPictureNode*     nodePtr;
    BufferInfoT bufInfo;
    VideoPicture* pPicture = NULL;

    lc = (LayerContext*)l;

    if(lc->bLayerInitialized == 0)
    {
        if (lc->nWidth * lc->nHeight > 1500000){//1080p
            switchBuffer(lc);
        }
        if(setLayerBuffer(lc, lc->nWidth, lc->nHeight) != 0)
        {
            loge("can not initialize layer.");
            return -1;
        }

        lc->bLayerInitialized = 1;
    }

    if(bInitFlag == 1)
    {
        for(i = 0; i < lc->nGpuBufferCount; i++)
        {
            if(lc->mBufferInfo[i].nUsedFlag == 0)
            {
                //* set the buffer address
                pPicture = *ppVideoPicture;
                pPicture = &lc->mBufferInfo[i].pPicture;

                lc->mBufferInfo[i].nUsedFlag = 1;
                break;
            }
        }
    }
    else
    {
        struct list_head *p;
        i = 0;
        list_for_each(p, &lc->picNodeListHead) {
            i++;
        }
        if(i >= lc->nLayerBufferNum)
        {
            VPictureNode* node = getDequeueableNode(lc);
            while (!node) {
                pthread_cond_wait(&lc->cond_dequeue, &lc->mutex);
                node = getDequeueableNode(lc);
            }
            pPicture = node->pPicture;
        }

    }

    *ppVideoPicture = pPicture;
    if(pPicture)
    {
        logv("** dequeue  pPicture(%p), id(%d)", pPicture, pPicture->nBufId);
        return 0;
    }
    else
    {
        logv("** dequeue  pPicture(%p)", pPicture);
        return -1;
    }
}

static int __LayerDequeueBuffer(LayerCtrl* l, VideoPicture** ppVideoPicture, int bInitFlag)
{
    int result;
    LayerContext* lc = (LayerContext*)l;
    pthread_mutex_lock(&lc->mutex);
    result = LayerDequeueBufferLocked(l, ppVideoPicture, bInitFlag);
    pthread_mutex_unlock(&lc->mutex);
    return result;
}

// this method should block here,
static int LayerQueueBufferLocked(LayerCtrl* l, VideoPicture* pBuf, int bValid)
{
    LayerContext* lc  = NULL;

    int               i;
    VPictureNode*     newNode;
    VPictureNode*     nodePtr;
    BufferInfoT    bufInfo;

    lc = (LayerContext*)l;

    if(pBuf)
        logv("** queue , pPicture(%p), id(%d)", pBuf, pBuf->nBufId);
    else
        logv("** queue , pPicture(%p)", pBuf);

    if(lc->bLayerInitialized == 0)
    {
        if(setLayerBuffer(lc, lc->nWidth, lc->nHeight) != 0)
        {
            loge("can not initialize layer.");
            return -1;
        }

        lc->bLayerInitialized = 1;
    }

    if (!list_empty(&lc->picNodeListHead)) {
        nodePtr = list_entry(lc->picNodeListHead.prev, struct VPictureNode_t, list); // get the lastest node
        // if the picture ready to join list to display is the same to the last node picture in list, ignore it
        // it usually happens at seeking with g2d running.
        if (nodePtr->pPicture == pBuf) {
            logw("the picture ready to join list to display is the same to the last node picture in list, ignore it!");
            pthread_cond_signal(&lc->cond_queue);
            return -1;
        }
    }

    newNode = NULL;
    for(i = 0; i< NUM_OF_PICTURE_NODES; i++)
    {
        if(lc->picNodes[i].bUsed == 0)
        {
            newNode = &lc->picNodes[i];
            newNode->bUsed = 1;
            newNode->pPicture = pBuf;
            newNode->dequeueable = 1;
            newNode->bValid = bValid;
            break;
        }
    }
    if(i == NUM_OF_PICTURE_NODES)
    {
        loge("*** picNode is full when queue buffer");
        return -1;
    }

    list_add_tail(&newNode->list,&lc->picNodeListHead);
    pthread_cond_signal(&lc->cond_queue);
    return 0;
}

static int fd_pic = -1;
static int dump_index = 0;
#define DUMP_NUM 11

//#define LC_DUMP_PIC
#ifdef LC_DUMP_PIC
static int LC_DumpPicOnce(uint8_t *data, int len)
{
	if (++dump_index != DUMP_NUM)
	{
		return 0;
	}
	
	fd_pic = open("/mnt/UDISK/ws/miracast/pic.dat", O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
	assert(fd_pic >= 0);
	write(fd_pic, data, len);
	fsync(fd_pic);
	close(fd_pic);
}
#endif

static int __LayerQueueBuffer(LayerCtrl* l, VideoPicture* pBuf, int bValid)
{
    int result;
    LayerContext* lc = (LayerContext*)l;

#ifdef LC_DUMP_PIC
    LC_DumpPicOnce(pBuf->pData0, pBuf->nWidth*pBuf->nHeight*3/2);
#endif
    pthread_mutex_lock(&lc->mutex);
    result = LayerQueueBufferLocked(l, pBuf, bValid);
    pthread_mutex_unlock(&lc->mutex);
    return result;
}

static int __LayerSetDisplayBufferCount(LayerCtrl* l, int nBufferCount)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

    logv("LayerSetBufferCount: count = %d",nBufferCount);

    lc->nGpuBufferCount = nBufferCount;

    if(lc->nGpuBufferCount > GPU_BUFFER_NUM)
        lc->nGpuBufferCount = GPU_BUFFER_NUM;

    return lc->nGpuBufferCount;
}

static int __LayerGetBufferNumHoldByGpu(LayerCtrl* l)
{
    LayerContext* lc;
    lc = (LayerContext*)l;
    return lc->nLayerBufferNum;
}

static int __LayerGetDisplayFPS(LayerCtrl* l)
{
    (void)l;
    return 60;
}

static void __LayerResetNativeWindow(LayerCtrl* l,void* pNativeWindow)
{
    logd("LayerResetNativeWindow : %p ",pNativeWindow);

    LayerContext* lc;
    VideoPicture mPicBufInfo;

    lc = (LayerContext*)l;
    lc->bLayerInitialized = 0;

    return ;
}

static VideoPicture* __LayerGetBufferOwnedByGpu(LayerCtrl* l)
{
    LayerContext* lc;
    VideoPicture* pPicture = NULL;
    lc = (LayerContext*)l;

    VPictureNode* nodePtr;
    if(!list_empty(&lc->picNodeListHead)){
        nodePtr = list_entry(lc->picNodeListHead.next, struct VPictureNode_t, list);
        list_del(lc->picNodeListHead.next);
        pPicture = nodePtr->pPicture;
        nodePtr->bUsed = 0;
    }
    return pPicture;
}

static int __LayerSetVideoWithTwoStreamFlag(LayerCtrl* l, int bVideoWithTwoStreamFlag)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

    logv("LayerSetIsTwoVideoStreamFlag, flag = %d",bVideoWithTwoStreamFlag);
    lc->bVideoWithTwoStreamFlag = bVideoWithTwoStreamFlag;

    return 0;
}

static int __LayerSetIsSoftDecoderFlag(LayerCtrl* l, int bIsSoftDecoderFlag)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

    logv("LayerSetIsSoftDecoderFlag, flag = %d",bIsSoftDecoderFlag);
    lc->bIsSoftDecoderFlag = bIsSoftDecoderFlag;

    return 0;
}

//* Description: the picture buf is secure
static int __LayerSetSecure(LayerCtrl* l, int bSecure)
{
    logv("__LayerSetSecure, bSecure = %d", bSecure);
    //*TODO
    LayerContext* lc;

    lc = (LayerContext*)l;

    lc->bProtectFlag = bSecure;

    return 0;
}

static int __LayerReleaseBuffer(LayerCtrl* l, VideoPicture* pPicture)
{
    logv("***LayerReleaseBuffer");
    LayerContext* lc;

    lc = (LayerContext*)l;

    CdxIonPfree(pPicture->pData0);
    CdxIonPfree(pPicture->pMetaData);
    return 0;
}

static int __LayerControl(LayerCtrl* l, int cmd, void *para)
{
    LayerContext *lc = (LayerContext*)l;

    CDX_UNUSE(para);

    switch(cmd)
    {
		case CDX_LAYER_CMD_SET_HDR_INFO:
		{
			LayerSetHdrInfo(l, (const FbmBufInfo *)para);
			break;
		}
        case CDX_LAYER_CMD_SET_VIDEO_RESOLUTION_CHANGED:
        {
            lc->bResolutionChange = *(int*)para;
            if (lc->bResolutionChange == 1) {
                lc->bResolutionChanging = 1;
            }
            logw("resolution changed %d", lc->bResolutionChange);
            break;
        }
#if TINA_LINUX_SUPPORT
        case CDX_LAYER_CMD_SET_VIDEOCODEC_LBC_MODE:
        {
            logd("get the fbm buf info");
            FbmBufInfo* fbmBufInfo = (FbmBufInfo*)para;
            lc->b10BitVideoFlag = fbmBufInfo->b10bitVideoFlag;
            lc->nLbcLossyComMod = fbmBufInfo->nLbcLossyComMod;
            lc->bIsLossy        = fbmBufInfo->bIsLossy;
            lc->bRcEn           = fbmBufInfo->bRcEn;
            logd("b10BitPicFlag = %d, nLbcLossyComMod = %d, bIsLossy = %u, bRcEn = %u", lc->b10BitVideoFlag, lc->nLbcLossyComMod, lc->bIsLossy, lc->bRcEn);
            break;
        }
#endif
#ifdef CONF_SUPPORT_G2D
        case CDX_LAYER_CMD_SET_DISP_BUFFER_NUM:
        {
            if (para == NULL)
                lc->nLayerBufferNum = GetConfigParamterInt("pic_4list_num", 3) + 2;
            else
                lc->nLayerBufferNum = *(int*)para;
            logd("update nLayerBufferNum %d\n", lc->nLayerBufferNum);
            break;
        }
#endif
        default:
            break;
    }
    return 0;
}

static int __LayerSetMiracastMode(LayerCtrl* l, int isMiracast){
    LayerContext* lc;
    lc = (LayerContext*)l;
    lc->bIsMiracast = isMiracast;
    return 0;
}

static int __LayerSetRotateDegree(LayerCtrl* l, int degree){
    LayerContext* lc;
    lc = (LayerContext*)l;
    if (lc->nDegree == degree)
        return -1;
    lc->nDegree = degree;
    lc->bScreenRotating = 1;
    lc->cropLT = 0;

    /*
    if (degree != 0){
        DispLayerRelease(l);
    }
    GpuLayerRelease(degree == 0 ? 1 : 0, degree == 0 ? 4 : 1);
    */

    return 0;
}

static int __LayerRotateScreen(LayerCtrl* l, VideoPicture* pBuf){
    LayerContext* lc;
    lc = (LayerContext*)l;

    loge("__LayerRotateScreen degree=%d start", lc->nDegree);
    int ret = 0;
    /*
    if(lc->nDegree != 0){//gpu, destroy disp layer
        //displayerrelease(l);
        ret = SetGpuLayerParam(lc, pBuf);
    }else{//de, fill fb0 with 0
        ret = SetLayerParam(lc, pBuf);
    }

    loge("__LayerRotateScreen degree=%d end", lc->nDegree);
    */

    if(ret != 0){
        loge("can not rotate screen");
        return -1;
    }
    return 0;
}

static int __LayerDisplaySetFullScreen(LayerCtrl* l, int fullScreen){
    LayerContext* lc;
    lc = (LayerContext*)l;
    loge("__LayerDisplaySetFullScreen fullScreen=%d", fullScreen);
    lc->bFullScreen = fullScreen;
    return 0;
}

static int __LayerSetZorder(LayerCtrl* l, int zorder){
    LayerContext* lc;
    lc = (LayerContext*)l;
    lc->zorder = zorder;
    return 0;
}

static int __LayerSetLayerID(LayerCtrl* l, int layerId){
    LayerContext* lc;
    lc = (LayerContext*)l;
    lc->nLayerID = layerId;
    return 0;
}

static int __LayerGetCropResolution(LayerCtrl* l, int * width, int * height){
    LayerContext* lc;
    lc = (LayerContext*)l;
    if(lc->nDegree == 90 || lc->nDegree == 270) {
        *width = lc->crop_height;
        *height = lc->crop_width;
    }
    else {
        *width = lc->crop_width;
        *height = lc->crop_height;
    }
    lc->crop_width = 0;
    lc->crop_height = 0;
    return 0;
}


static LayerControlOpsT mLayerControlOps =
{
    release:                    __LayerRelease                   ,

    setSecureFlag:              __LayerSetSecure                 ,
    setDisplayBufferSize:       __LayerSetDisplayBufferSize      ,
    setDisplayBufferCount:      __LayerSetDisplayBufferCount     ,
    setDisplayRegion:           __LayerSetDisplayRegion          ,
    setScreenRegion:            __LayerSetScreenRegion           ,
    setDisplayPixelFormat:      __LayerSetDisplayPixelFormat     ,
    setVideoWithTwoStreamFlag:  __LayerSetVideoWithTwoStreamFlag ,
    setIsSoftDecoderFlag:       __LayerSetIsSoftDecoderFlag      ,
    setBufferTimeStamp:         __LayerSetBufferTimeStamp        ,

    resetNativeWindow :         __LayerResetNativeWindow         ,
    getBufferOwnedByGpu:        __LayerGetBufferOwnedByGpu       ,
    getDisplayFPS:              __LayerGetDisplayFPS             ,
    getBufferNumHoldByGpu:      __LayerGetBufferNumHoldByGpu     ,

    ctrlShowVideo :             __LayerCtrlShowVideo             ,
    ctrlHideVideo:              __LayerCtrlHideVideo             ,
    ctrlIsVideoShow:            __LayerCtrlIsVideoShow           ,
    ctrlHoldLastPicture :       __LayerCtrlHoldLastPicture       ,

    dequeueBuffer:              __LayerDequeueBuffer             ,
    queueBuffer:                __LayerQueueBuffer               ,
    releaseBuffer:              __LayerReleaseBuffer             ,
    reset:                      __LayerReset                     ,
    destroy:                    __LayerDestroy                   ,
    control:                    __LayerControl                   ,
    setMiracastMode:            __LayerSetMiracastMode           ,
    setRotateDegree:            __LayerSetRotateDegree           ,
    rotateScreen:               __LayerRotateScreen              ,
    displaySetFullScreen:       __LayerDisplaySetFullScreen      ,
    displaySetZorder:           __LayerSetZorder                 ,
    displaySetLayerID:          __LayerSetLayerID                ,
    getCropResolution:          __LayerGetCropResolution
};

static VPictureNode* GetLastPictureToRender(LayerContext *lc) {
    VPictureNode* node;

    if(list_empty(&lc->picNodeListHead)){
        return NULL;
    }
    node = list_entry(lc->picNodeListHead.prev, struct VPictureNode_t, list);//always get the lastest node to display
    if(node->bValid){
        node->dequeueable = 0;// the node which is displaying cannot be dequeued from the list
        return node;
    }
    logd("no valid frame to render, repeating last");
    return NULL;

}

static VPictureNode* GetPictureToRender(LayerContext *lc) {
    VPictureNode* node;

    if(list_empty(&lc->picNodeListHead)){
        return NULL;
    }

    //1. get hdmi freq
    int disp_mode = getDisplayOutputMode(0, lc->fdDisplay);
    int disp_freq = getDispFreq(disp_mode);
    logv("display output mode is %d, freq is %dhz", disp_mode, disp_freq);
    int disp_interval;
    if (disp_freq > 0) {
        disp_interval = 1000/disp_freq;//ms
    } else {
        disp_interval = 33;//ms
        logw("we donot know disp freq, just guess 30HZ, here not enough test!");
    }

    if (lc->bResolutionChange) {
        logw("resolution changing, repeating last.");
        lc->currentPicNode = NULL;
        return NULL;
    }
    int64_t lastPts = -1;
    node = lc->currentPicNode;
    if (node == NULL || node->list.next == NULL || node->list.prev == NULL) {
        logd("currentPicNode is null!");
        node = list_entry(lc->picNodeListHead.next, struct VPictureNode_t, list);
        #if 0
        return node;
        #else
        if(node->bValid)
        {
            return node;
        }
        logd("no valid frame to render");
        return NULL;
        #endif
    } else {
        if(!list_is_last(&lc->currentPicNode->list, &lc->picNodeListHead)){
            node = list_entry(lc->currentPicNode->list.next, struct VPictureNode_t, list);
            lastPts = lc->currentPicNode->pPicture->nPts;
        } else {
            logv("no new frame to render, repeating last ");
            return NULL;
        }
    }

    list_for_each_entry_from(node, &lc->picNodeListHead, list) {
        if (node->bValid) {
            if (lc->currentPicNode) {
                if (node->pPicture != NULL && lc->currentPicNode->pPicture != NULL) {
                    if (node->pPicture->nPts >= lastPts) {
                        lastPts = node->pPicture->nPts;
                    } else {
                        logw("it maybe a seek action, just return this node, lastPts= %lldus, next pts = %lldus",
                             lastPts, node->pPicture->nPts);
                        return node;
                    }
                    int64_t pts_diff = node->pPicture->nPts / 1000 - lc->currentPicNode->pPicture->nPts / 1000;  // ms
                    logv("trying to get picture to render, pts diff is %lldms, pts is %lldus", pts_diff,
                         node->pPicture->nPts);
                    if (pts_diff > disp_interval / 2 + 1) {
                        return node;
                    }
                }
            } else {  // the first picture
                return node;
            }
        }
    }

    logv("no valid frame to render, repeating last");
    return NULL;
}

static void Render(LayerContext* lc) {
    //1. sync_wait the second lastest fence
    if (0 <= lc->preFenceFd) {
        int timeout = 500; /* 500 ms */
        timeout = sync_wait(lc->preFenceFd, timeout); // queue/dequeue could work simultaneously
        if (0 > timeout) {
            loge("error: fence(%d) timeout!", lc->preFenceFd);
        }
        close(lc->preFenceFd);
        lc->preFenceFd = -1;
        if (lc->prePicNode != NULL)
            lc->prePicNode->dequeueable = 1;
    }
    //2. get picture to render
    pthread_mutex_lock(&lc->mutex);
    if (!lc->running) {
        logw("render will exit1");
        pthread_mutex_unlock(&lc->mutex);
        return;
    }
    VPictureNode* node = GetPictureToRender(lc);
    while (node == NULL || node == lc->currentPicNode) {
        pthread_cond_wait(&lc->cond_queue, &lc->mutex); // switch out to queue/dequeue
        if (!lc->running) {
            logw("render will exit2");
            pthread_mutex_unlock(&lc->mutex);
            return;
        }
        node = GetPictureToRender(lc); //GetLastPictureToRender(lc);
    }
    node->dequeueable = 0;  // the node which is displaying cannot be dequeued from the list
    //3. acquire the new fence
    struct sync_info sync;
    createSyncPoint(0, lc->fdDisplay, &sync);
    lc->count = sync.count;
    if (lc->currentPicNode != NULL && node->pPicture != NULL && lc->currentPicNode->pPicture != NULL) {
        logv("send picture to Render, pts diff= %lldms, pts = %lldms",
             node->pPicture->nPts / 1000 - lc->currentPicNode->pPicture->nPts / 1000, node->pPicture->nPts / 1000);
    }
    //4. set_layer_config
    if (SetLayerParam(lc, node->pPicture) != 0) {
        loge("can not render frame");
    }

    lc->preFenceFd = lc->currentFenceFd;
    lc->currentFenceFd = sync.fd;
    lc->prePicNode = lc->currentPicNode;
    lc->currentPicNode = node;

    pthread_cond_signal(&lc->cond_dequeue); // almost not work
    pthread_mutex_unlock(&lc->mutex);
}

static void* RenderThread(void* arg) {
    LayerContext *lc = (LayerContext *)arg;

    while (lc->running) {
        Render(lc);
    }
    close(lc->preFenceFd);
    close(lc->currentFenceFd);
    logd("render thread exit");
    return NULL;
}

LayerCtrl* WFD_LayerCreate_DE()
{
    LayerContext* lc;
    unsigned long args[4];
    struct disp_layer_info2 layerInfo;
    int screen_id;

    logd("cdx sink for tina h133, LayerCreate.");

    lc = (LayerContext*)malloc(sizeof(LayerContext));
    if(lc == NULL)
    {
        loge("malloc memory fail.");
        return NULL;
    }
    memset(lc, 0, sizeof(LayerContext));

    lc->fdDisplay = openDispDev();// open node path /dev/disp
    if(lc->fdDisplay < 1)
    {
        loge("open disp failed");
        free(lc);
        return NULL;
    }
    if (createSyncTimeline(0, lc->fdDisplay) < 0){
        logw("create sync time line failed");
        if(lc->fdDisplay>=0)
            close(lc->fdDisplay);
        lc->fdDisplay=-1;
        free(lc);
        return NULL;
    }

    lc->base.ops = &mLayerControlOps;
    lc->eDisplayPixelFormat = PIXEL_FORMAT_YV12;

    memset(&layerInfo, 0, sizeof(struct disp_layer_info2));
    char displayChars[4]  = "";
//    if (uci_get_config("ratio", displayChars, sizeof(displayChars), DISPLAY_UCI_CONFIG_PATH, "display") != -1) {
//        lc->regionScale = atoi(displayChars);
//        logw("get display region value %d !", lc->regionScale);
//    } else {
        lc->regionScale = 100;
        logw("get display region scale fail,use default value %d !", lc->regionScale);
//    }

    //  get screen size.
    args[0] = 0;
    lc->nScreenWidth  = ioctl(lc->fdDisplay, DISP_GET_SCN_WIDTH, (void *)args);
    lc->nScreenHeight = ioctl(lc->fdDisplay, DISP_GET_SCN_HEIGHT,(void *)args);
    logd("screen:w %d, screen:h %d ,region scale value %d !", lc->nScreenWidth, lc->nScreenHeight,lc->regionScale);

    lc->nLayerBufferNum = GetConfigParamterInt("pic_4list_num", 3);

    CdxIonOpen();
    lc->screen_x = 0;
    lc->screen_y = 0;
    lc->screen_w = 0;
    lc->screen_h = 0;

    lc->zorder = 5;
    lc->nLayerID = 0;

    // create and start rendering thread
    pthread_mutex_init(&lc->mutex, NULL);
    pthread_cond_init(&lc->cond_dequeue, NULL);
    pthread_cond_init(&lc->cond_queue, NULL);
    lc->running = 1;
    lc->currentFenceFd = -1;
    lc->preFenceFd = -1;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    INIT_LIST_HEAD(&lc->picNodeListHead);
    pthread_create(&lc->renderThread, &attr, RenderThread, lc);

    return &lc->base;
}
