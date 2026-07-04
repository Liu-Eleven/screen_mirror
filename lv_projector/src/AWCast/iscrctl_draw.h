#ifndef ISRCCTL_DRAW_H
#define ISRCCTL_DRAW_H

#ifdef __cplusplus
extern "C"
{
#endif

enum ScrCtlCommandE
{
    DLNA_EVENT_QUIT = 10,
};

typedef struct SrcctlDrawS SrcctlDrawT;

struct SrcctlDrawOpsS {
    int (*video_show)(SrcctlDrawT *);

	int (*entry_show)(SrcctlDrawT *, const char * /*movie_name*/);

	int (*entry_hide)(SrcctlDrawT *);

	int (*process_bar_show)(SrcctlDrawT *, int /*duration*/, int /*position*/);

	int (*process_bar_hide)(SrcctlDrawT *);

	int (*volume_bar_show)(SrcctlDrawT *, int /*vol*/);
	
	int (*volume_bar_hide)(SrcctlDrawT *);

	int (*buffering_start)(SrcctlDrawT *);
	
	int (*buffering_end)(SrcctlDrawT *);

    int (*control)(SrcctlDrawT *, int /*cmd*/, void * /*param*/);
};

struct SrcctlDrawS
{
	struct SrcctlDrawOpsS *ops;
};

static inline int SrcctlDraw_VideoShow(SrcctlDrawT *scd)
{
    return scd->ops->video_show(scd);
}

static inline int SrcctlDraw_EntryShow(SrcctlDrawT *scd, const char *movie_name)
{
	return scd->ops->entry_show(scd, movie_name);
}

static inline int SrcctlDraw_EntryHide(SrcctlDrawT *scd)
{
	return scd->ops->entry_hide(scd);
}

static inline int SrcctlDraw_ProcessBarShow(SrcctlDrawT *scd, int duration, int position)
{
	return scd->ops->process_bar_show(scd, duration, position);
}

static inline int SrcctlDraw_ProcessBarHide(SrcctlDrawT *scd)
{
	return scd->ops->process_bar_hide(scd);
}

static inline int SrcctlDraw_VolumeBarShow(SrcctlDrawT *scd, int vol)
{
	return scd->ops->volume_bar_show(scd, vol);
}

static inline int SrcctlDraw_VolumeBarHide(SrcctlDrawT *scd)
{
	return scd->ops->volume_bar_hide(scd);
}

static inline int SrcctlDraw_BufferingStart(SrcctlDrawT *scd)
{
	return scd->ops->buffering_start(scd);
}

static inline int SrcctlDraw_BufferingEnd(SrcctlDrawT *scd)
{
	return scd->ops->buffering_end(scd);
}

static inline int SrcctlDraw_Control(SrcctlDrawT *scd, int cmd, void *param)
{
    return scd->ops->control(scd, cmd, param);
}

SrcctlDrawT *LV_SrcctlDraw_Instance();

#ifdef __cplusplus
} // extern "C"
#endif

#endif
