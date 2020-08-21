#ifndef PURE_LIST_DOUBLE_H_
#define PURE_LIST_DOUBLE_H_

#include "type.h"

pure_list_p pure_list_init();

void pure_list_fini(pure_list_p p_list);

void pure_list_fini_with_freefunc(pure_list_p p_list, void func(void *p_data));

int pure_list_item_add(pure_list_p p_list,void *p_data);

void pure_list_item_del(pure_list_p p_list,void *p_data);

pure_list_item_p pure_list_item_get(pure_list_p p_list,void *p_data);

int pure_list_get_len(pure_list_p p_list);

void *pure_list_get_first(pure_list_p p_list);

//event_handle_list
void* pure_eventlist_item_get_byhandle(pure_list_p p_list, long l_handle);

//io_context list
void* pure_iocontextlist_item_get_bysyshandle(pure_list_p p_list, pure_sys_handle syshandle);



#endif //PURE_LIST_DOUBLE_H_