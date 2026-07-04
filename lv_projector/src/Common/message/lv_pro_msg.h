#ifndef LV_PRO_MSG_H_
#define LV_PRO_MSG_H_

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef int msg_type_t;

typedef struct LvMessage {
    msg_type_t type;
    void *data;
    bool free_data;
    struct LvMessage *next;
} LvMessage;

typedef struct {
    LvMessage *head;
    LvMessage *tail;
    pthread_mutex_t lock;
} MessageQueue;

MessageQueue *lv_pro_msg_create_queue();
void lv_pro_msg_enqueue(MessageQueue *queue, msg_type_t type, void *data, bool free_data);
LvMessage *lv_pro_msg_dequeue(MessageQueue *queue);
void lv_pro_msg_free_queue(MessageQueue *queue);
void lv_pro_msg_free(LvMessage *msg);
LvMessage *lv_pro_msg_find(MessageQueue *queue, uint32_t type);

#endif /* LV_PRO_MSG_H_ */