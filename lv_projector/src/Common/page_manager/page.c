#include <assert.h>
#include "lv_common.h"
#include "page.h"
#include "sys_msg.h"
#include "message/lv_pro_msg.h"

static MessageQueue *m_page_msg_queue;
static page_id_t current_page_id = PAGE_NONE;
static page_id_t last_page_id = PAGE_NONE;
static pthread_mutex_t page_lock = PTHREAD_MUTEX_INITIALIZER;;

static page_interface_t *find_page(page_id_t id)
{
    LvMessage *msg;
    if (m_page_msg_queue) {
        msg = lv_pro_msg_find(m_page_msg_queue, id);

        if (msg)
            return msg->data;
    }
    return NULL;
}

page_id_t set_current_page_id(page_id_t id)
{
    current_page_id = id;
}

page_id_t get_current_page_id(void)
{
    return current_page_id;
}

page_id_t get_last_page_id(void)
{
    return last_page_id;
}

int create_page(page_id_t id)
{
    page_interface_t *page = find_page(id);
    if(page) {
        if (page->ops.on_create) {
            int tmp_id = last_page_id;
            last_page_id = current_page_id;
            current_page_id = id;
            if (last_page_id == current_page_id) {
                last_page_id = tmp_id;
            }
            page->ops.on_create();
            //printf("create_page cur_id %d, last_id %d\n", current_page_id, last_page_id);
            return 0;
        }
    }
    return -1;
}

void destory_page(page_id_t id)
{
    page_interface_t *page = find_page(id);
    if(page) {
        if (page->ops.on_destroy) {
            page->ops.on_destroy();
        }
    }
}

void show_page(page_id_t id)
{
    page_interface_t *page = find_page(id);
    if(page) {
        if (page->ops.on_show) {
            int tmp_id = last_page_id;
            last_page_id = current_page_id;
            current_page_id = id;
            if (last_page_id == current_page_id) {
                last_page_id = tmp_id;
            }
            page->ops.on_show();
        }
    }
}

void hide_page(page_id_t id)
{
    page_interface_t *page = find_page(id);
    if(page) {
        if (page->ops.on_hide) {
            page->ops.on_hide();
        }
    }
}

static void __switch_page(page_interface_t *page)
{
    printf("switch_page cur_id %d, next_id %d\n", current_page_id, page->info.id);

    pthread_mutex_lock(&page_lock);

    int tmp_id = current_page_id;
    if (current_page_id != page->info.id) {
        if (page->info.id != PAGE_NONE)
            create_page(page->info.id);

        if (tmp_id != PAGE_NONE)
            destory_page(tmp_id);
    }
    pthread_mutex_unlock(&page_lock);
}

static void __switch_page_create_hide(page_interface_t *page)
{
    printf("switch_page_create_hide cur_id %d, next_id %d\n", current_page_id, page->info.id);

    pthread_mutex_lock(&page_lock);

    if (current_page_id != page->info.id) {
        if (current_page_id != PAGE_NONE)
            hide_page(current_page_id);

        if (page->info.id != PAGE_NONE)
            create_page(page->info.id);
    }
    pthread_mutex_unlock(&page_lock);
}

static void __switch_page_show_destory(page_interface_t *page)
{
    printf("switch_page_show_destory cur_id %d, next_id %d\n", current_page_id, page->info.id);

    pthread_mutex_lock(&page_lock);

    int tmp_id = current_page_id;
    if (current_page_id != page->info.id) {
        if (page->info.id != PAGE_NONE)
            show_page(page->info.id);

        if (tmp_id != PAGE_NONE)
            destory_page(tmp_id);
    }
    pthread_mutex_unlock(&page_lock);
}

void switch_page(page_id_t id)
{
    page_interface_t *page = find_page(id);

    if (page) {
        system_send_msg(MSG_TYPE_PAGE_CREATE_DESTORY, page);
    }
    return;
}

void switch_page_create_hide(page_id_t id)
{
    page_interface_t *page = find_page(id);

    if (page) {
        system_send_msg(MSG_TYPE_PAGE_CREATE_HIDE, page);
    }
    return;
}

void switch_page_show_destory(page_id_t id)
{
    page_interface_t *page = find_page(id);

    if (page) {
        system_send_msg(MSG_TYPE_PAGE_SHOW_DESTORY, page);
    }
    return;
}

void page_message_process(system_msg_t *msg)
{
    if (!msg) return;

    switch (msg->type) {
    case MSG_TYPE_PAGE_CREATE_DESTORY:
        if (msg->data)
            __switch_page(msg->data);
        break;
    case MSG_TYPE_PAGE_CREATE_HIDE:
        if (msg->data)
            __switch_page_create_hide(msg->data);
        break;
    case MSG_TYPE_PAGE_SHOW_DESTORY:
        if (msg->data)
            __switch_page_show_destory(msg->data);
        break;
#if 0
    case MSG_TYPE_PAGE_DESTORY:
        break;
    case MSG_TYPE_PAGE_SHOW:
        break;
    case MSG_TYPE_PAGE_HIDE:
        break;
#endif
    default:
        break;
    }
}

void reg_page(page_interface_t *page)
{
    if (m_page_msg_queue == NULL)
        m_page_msg_queue = lv_pro_msg_create_queue();

    if (!m_page_msg_queue || !page)
        return;

    lv_pro_msg_enqueue(m_page_msg_queue, page->info.id, page, false);
}

void unreg_page(page_interface_t *page)
{
}

void page_manage_init(void)
{
    if (m_page_msg_queue == NULL)
        m_page_msg_queue = lv_pro_msg_create_queue();
}

void page_manage_exit(void)
{

}
