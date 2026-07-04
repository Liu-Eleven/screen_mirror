
#ifndef PAGE_H
#define PAGE_H

typedef struct {
    int (*on_create)(void);
    int (*on_destroy)(void);
    int (*on_show)(void);
    int (*on_hide)(void);
} page_ops_t;

typedef struct {
    page_id_t id;
    void *user_data;
} page_info_t;

typedef struct {
    page_ops_t ops;
    page_info_t info;
} page_interface_t;

void page_manage_init(void);
void page_manage_exit(void);
void reg_page(page_interface_t *page);
void unreg_page(page_interface_t *page);
void switch_page(page_id_t id);
void switch_page_create_hide(page_id_t id);
void switch_page_show_destory(page_id_t id);

page_id_t set_current_page_id(page_id_t id);
page_id_t get_current_page_id(void);
page_id_t get_last_page_id(void);

int create_page(page_id_t id);
void destory_page(page_id_t id);
void show_page(page_id_t id);
void hide_page(page_id_t id);

#define REG_PAGE(name)                     \
    do {                                   \
        extern void REGISTER_##name(void); \
        REGISTER_##name();                 \
    } while (0)

#endif  // PAGE_H
