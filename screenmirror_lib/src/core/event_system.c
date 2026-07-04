#include "screenmirror_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * 创建事件系统
 */
EventSystem* event_system_create(void)
{
    EventSystem *sys = (EventSystem *)malloc(sizeof(EventSystem));
    if (sys == NULL) {
        return NULL;
    }

    memset(sys, 0, sizeof(EventSystem));

    if (pthread_mutex_init(&sys->lock, NULL) != 0) {
        free(sys);
        return NULL;
    }

    sys->observer_count = 0;

    return sys;
}

/**
 * 销毁事件系统
 */
void event_system_destroy(EventSystem *sys)
{
    if (sys == NULL) {
        return;
    }

    pthread_mutex_destroy(&sys->lock);
    free(sys);
}

/**
 * 订阅事件
 */
int event_system_subscribe(EventSystem *sys, MirrorEventCallback callback,
                          void *user_data)
{
    if (sys == NULL || callback == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&sys->lock);

    if (sys->observer_count >= MAX_OBSERVERS) {
        pthread_mutex_unlock(&sys->lock);
        return MIRROR_ERR_OUT_OF_MEMORY;
    }

    sys->observers[sys->observer_count].callback = callback;
    sys->observers[sys->observer_count].user_data = user_data;
    sys->observer_count++;

    printf("[EVENT] Observer subscribed (total: %d)\n", sys->observer_count);

    pthread_mutex_unlock(&sys->lock);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 发送事件到所有订阅者
 */
void event_system_emit(EventSystem *sys, const MirrorEvent *event)
{
    if (sys == NULL || event == NULL) {
        return;
    }

    pthread_mutex_lock(&sys->lock);

    printf("[EVENT] Emitting event type: %d\n", event->type);

    for (int i = 0; i < sys->observer_count; i++) {
        if (sys->observers[i].callback) {
            sys->observers[i].callback(event, sys->observers[i].user_data);
        }
    }

    pthread_mutex_unlock(&sys->lock);
}
