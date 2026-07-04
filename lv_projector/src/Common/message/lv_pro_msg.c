#include "lv_pro_msg.h"

MessageQueue *lv_pro_msg_create_queue()
{
    MessageQueue *queue = (MessageQueue *)malloc(sizeof(MessageQueue));
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_init(&queue->lock, NULL);
    return queue;
}

void lv_pro_msg_enqueue(MessageQueue *queue, msg_type_t type, void *data, bool free_data)
{
    if (queue == NULL) {
        return;
    }
    pthread_mutex_lock(&queue->lock);
    LvMessage *new_msg = (LvMessage *)malloc(sizeof(LvMessage));
    new_msg->type = type;
    new_msg->data = data;
    new_msg->free_data = free_data;
    new_msg->next = NULL;

    if (queue->tail) {
        queue->tail->next = new_msg;
    } else {
        queue->head = new_msg;
    }
    queue->tail = new_msg;

    pthread_mutex_unlock(&queue->lock);
}

LvMessage *lv_pro_msg_dequeue(MessageQueue *queue)
{
    if (queue == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&queue->lock);
    if (!queue->head) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }

    LvMessage *msg = queue->head;
    queue->head = msg->next;
    if (!queue->head) {
        queue->tail = NULL;
    }

    pthread_mutex_unlock(&queue->lock);
    return msg;
}

LvMessage *lv_pro_msg_find(MessageQueue *queue, uint32_t type)
{
    if (queue == NULL)
        return NULL;

    pthread_mutex_lock(&queue->lock);
    if (!queue->head) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }

    LvMessage *msg = queue->head;

    while (msg) {
        if (msg->type == type) {
            pthread_mutex_unlock(&queue->lock);
            return msg;
        }

        if (msg == queue->tail)
            break;

        msg = msg->next;
    }

    pthread_mutex_unlock(&queue->lock);
    return NULL;
}

void lv_pro_msg_free_queue(MessageQueue *queue)
{
    while (queue->head) {
        LvMessage *msg = lv_pro_msg_dequeue(queue);
        if (msg->free_data) {
            if (msg->data) free(msg->data);
        }
        free(msg);
    }

    pthread_mutex_destroy(&queue->lock);
    free(queue);
}

void lv_pro_msg_free(LvMessage *msg)
{
    if (msg->free_data) {
        if (msg->data) free(msg->data);
    }
    free(msg);
}
