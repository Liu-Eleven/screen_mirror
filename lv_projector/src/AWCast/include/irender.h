#ifndef I_RENDER_H
#define I_RENDER_H

typedef struct IRenderS RenderT;
typedef int (*RenderCallback)(int ext, void *para);

struct IRenderOpsS {
    /* type: video/audio/image */
    int (*set_url)(RenderT *, char * /*url*/, char * /*title*/, int /*type*/);

    int (*play)(RenderT *);

    int (*stop)(RenderT *);

    int (*pause)(RenderT *);

    int (*rotate)(RenderT *, int);

    int (*resume)(RenderT *);

    int (*seekto)(RenderT *, int);

    int (*get_position)(RenderT *); /* ms */

    int (*get_duration)(RenderT *); /* ms */

    int (*get_state)(RenderT *);

    int (*set_mute)(RenderT *, int);

    int (*get_mute)(RenderT *);

    int (*set_volume)(RenderT *, int);

    int (*get_volume)(RenderT *);

    int (*destroy)(RenderT *);

    int (*setCallback)(RenderT *, RenderCallback);

    int (*setAdUrl)(RenderT *, char *);
};

struct IRenderS {
    struct IRenderOpsS *ops;
};

enum RenderStateE {
    RENDER_STATE_IDEL = 0,
    RENDER_STATE_PREPAING = 1,
    RENDER_STATE_PREPARED = 2,
    RENDER_STATE_PLAYING = 3,
    RENDER_STATE_PAUSE = 4,
    RENDER_STATE_STOP = 5,
};

enum RENDER_MESSAGE {
    RENDER_MESSAGE_PLAYBACK_COMPLETE = 0,
};

#ifdef __cplusplus
extern "C" {
#endif

static inline int RenderSetUrl(RenderT *render, char *url, char *title, int type) {
    return render->ops->set_url(render, url, title, type);
}

static inline int RenderPlay(RenderT *render) {
    return render->ops->play(render);
}

static inline int RenderStop(RenderT *render) {
    return render->ops->stop(render);
}

static inline int RenderRotate(RenderT *render, int degree) {
    return render->ops->rotate(render, degree);
}

static inline int RenderPause(RenderT *render) {
    return render->ops->pause(render);
}

static inline int RenderResume(RenderT *render) {
    return render->ops->resume(render);
}

static inline int RenderSeekto(RenderT *render, int pos_ms) {
    return render->ops->seekto(render, pos_ms);
}

static inline int RenderGetPosition(RenderT *render) {
    return render->ops->get_position(render);
}

static inline int RenderGetDuration(RenderT *render) {
    return render->ops->get_duration(render);
}

static inline int RenderGetState(RenderT *render) {
    return render->ops->get_state(render);
}

static inline int RenderSetMute(RenderT *render, int mute) {
    return render->ops->set_mute(render, mute);
}

static inline int RenderGetMute(RenderT *render) {
    return render->ops->get_mute(render);
}

static inline int RenderSetVolume(RenderT *render, int volume) {
    return render->ops->set_volume(render, volume);
}

static inline int RenderGetVolume(RenderT *render) {
    return render->ops->get_volume(render);
}

static inline int RenderDestroy(RenderT *render) {
    return render->ops->destroy(render);
}

static inline int RenderSetCallBack(RenderT *render, RenderCallback renderCallback) {
    return render->ops->setCallback(render, renderCallback);
}

static inline int RenderSetAdUrl(RenderT *render, char *ad_url) {
    return render->ops->setAdUrl(render, ad_url);
}
/* impl list */
extern RenderT *CedarRender_Instance();

#ifdef __cplusplus
}
#endif

#endif
