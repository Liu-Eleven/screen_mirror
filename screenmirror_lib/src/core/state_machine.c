#include "screenmirror_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * 创建状态机
 */
StateMachine* state_machine_create(void)
{
    StateMachine *sm = (StateMachine *)malloc(sizeof(StateMachine));
    if (sm == NULL) {
        return NULL;
    }

    memset(sm, 0, sizeof(StateMachine));
    sm->current_state = MIRROR_STATE_IDLE;

    if (pthread_mutex_init(&sm->lock, NULL) != 0) {
        free(sm);
        return NULL;
    }

    return sm;
}

/**
 * 销毁状态机
 */
void state_machine_destroy(StateMachine *sm)
{
    if (sm == NULL) {
        return;
    }

    pthread_mutex_destroy(&sm->lock);
    free(sm);
}

/**
 * 检查状态转移是否合法
 */
bool state_machine_can_transition(StateMachine *sm, MirrorState new_state)
{
    if (sm == NULL) {
        return false;
    }

    MirrorState current = sm->current_state;

    /* 定义合法的状态转移 */
    switch (current) {
        case MIRROR_STATE_IDLE:
            return (new_state == MIRROR_STATE_DISCOVERING ||
                    new_state == MIRROR_STATE_CONNECTING);

        case MIRROR_STATE_DISCOVERING:
            return (new_state == MIRROR_STATE_IDLE ||
                    new_state == MIRROR_STATE_CONNECTING);

        case MIRROR_STATE_CONNECTING:
            return (new_state == MIRROR_STATE_CONNECTED ||
                    new_state == MIRROR_STATE_IDLE ||
                    new_state == MIRROR_STATE_ERROR);

        case MIRROR_STATE_CONNECTED:
            return (new_state == MIRROR_STATE_STREAMING ||
                    new_state == MIRROR_STATE_IDLE ||
                    new_state == MIRROR_STATE_ERROR);

        case MIRROR_STATE_STREAMING:
            return (new_state == MIRROR_STATE_PAUSED ||
                    new_state == MIRROR_STATE_IDLE ||
                    new_state == MIRROR_STATE_ERROR);

        case MIRROR_STATE_PAUSED:
            return (new_state == MIRROR_STATE_STREAMING ||
                    new_state == MIRROR_STATE_IDLE ||
                    new_state == MIRROR_STATE_ERROR);

        case MIRROR_STATE_ERROR:
            return (new_state == MIRROR_STATE_IDLE);

        default:
            return false;
    }
}

/**
 * 状态转移
 */
int state_machine_transition(StateMachine *sm, MirrorState new_state)
{
    if (sm == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&sm->lock);

    if (!state_machine_can_transition(sm, new_state)) {
        printf("[STATE] Invalid transition from %d to %d\n",
               sm->current_state, new_state);
        pthread_mutex_unlock(&sm->lock);
        return MIRROR_ERR_UNKNOWN;
    }

    printf("[STATE] Transitioning from %d to %d\n",
           sm->current_state, new_state);

    sm->current_state = new_state;

    pthread_mutex_unlock(&sm->lock);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 获取当前状态
 */
MirrorState state_machine_get_state(StateMachine *sm)
{
    if (sm == NULL) {
        return MIRROR_STATE_IDLE;
    }

    pthread_mutex_lock(&sm->lock);
    MirrorState state = sm->current_state;
    pthread_mutex_unlock(&sm->lock);

    return state;
}
