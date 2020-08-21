#include "list_double.h"

#include <stdlib.h>

pure_list_p pure_list_init()
{
	pure_list_p p_ret = (pure_list_p)calloc(1, sizeof(pure_list));
	p_ret->n_len = 0;
	return p_ret;
}

void pure_list_fini(pure_list_p p_list)
{
	pure_list_item_p p_tmp;
	pure_list_item_p p_prev;

	if (p_list->n_len > 0)
	{
		p_tmp = p_list->end;
		while (p_tmp)
		{
			p_prev = p_tmp->prev;
			
			if (p_tmp)
			{
				free(p_tmp->p_data);
			}
			free(p_tmp);

			p_tmp = p_prev;
		}
	}

	free(p_list);
}

void pure_list_fini_with_freefunc(pure_list_p p_list, void func(void *p_data))
{
	pure_list_item_p p_tmp;
	pure_list_item_p p_prev;

	if (p_list->n_len > 0)
	{
		p_tmp = p_list->end;
		while (p_tmp)
		{
			p_prev = p_tmp->prev;

			if (p_tmp)
			{
				func(p_tmp->p_data);
			}

			free(p_tmp);

			p_tmp = p_prev;
		}
	}

	free(p_list);
}

int pure_list_item_add(pure_list_p p_list,void *p_data)
{
	pure_list_item_p p_tmp;

	if (p_list == 0)
		return -1;

	p_tmp = p_list->head;
	
	while (p_tmp)
	{
		if (p_tmp->p_data == p_data)
			return - 2;

		p_tmp = p_tmp->next;
	}

	if (p_list->head == 0 && p_list->end == 0)
	{
		// list empty
		p_tmp = (pure_list_item_p)calloc(1, sizeof(pure_list_item));
		p_tmp->p_data = p_data;
		p_list->head = p_tmp;
		p_list->end = p_tmp;
	}
	else
	{
		p_list->end->next = (pure_list_item_p)calloc(1, sizeof(pure_list_item));
		p_list->end->next->p_data = p_data;
		p_list->end->next->prev = p_list->end;
		p_list->end = p_list->end->next;
	}
	p_list->n_len++;

	return p_list->n_len;
}

void pure_list_item_del(pure_list_p p_list, void *p_data)
{
	pure_list_item_p p_tmp;

	if (p_list == 0)
		return ;

	p_tmp = p_list->head;

	while (p_tmp)
	{
		if (p_tmp->p_data == p_data)
		{
			break;
		}

		p_tmp = p_tmp->next;
	}

	if (p_tmp == p_list->head)
	{
		p_list->head = p_tmp->next;
	}
	
	if (p_tmp == p_list->end)
	{
		p_list->end = p_tmp->prev;
	}

	if (p_tmp->prev)
		p_tmp->prev->next = p_tmp->next;
	if (p_tmp->next)
		p_tmp->next->prev = p_tmp->prev;
	free(p_tmp);

	p_list->n_len--;
}

pure_list_item_p pure_list_item_get(pure_list_p p_list, void *p_data)
{
	pure_list_item_p p_tmp;

	if (p_list == 0)
		return 0;

	p_tmp = p_list->head;

	while (p_tmp)
	{
		if (p_tmp->p_data == p_data)
		{
			break;
		}

		p_tmp = p_tmp->next;
	}

	return p_tmp;
}

int pure_list_get_len(pure_list_p p_list)
{
	if (p_list)
	{
		return p_list->n_len;
	}
	return 0;
}

void *pure_list_get_first(pure_list_p p_list)
{
	pure_list_item_p p_tmp;

	if (p_list == 0 || p_list->head == 0)
		return 0;

	return p_list->head->p_data;
}

void* pure_eventlist_item_get_byhandle(pure_list_p p_list, long l_handle)
{
	pure_list_item_p p_tmp;

	if (p_list == 0)
		return 0;

	p_tmp = p_list->head;

	while (p_tmp)
	{
		if (((pure_event_handle_ptr)p_tmp->p_data)->l_handle == l_handle)
		{
			break;
		}

		p_tmp = p_tmp->next;
	}

	return p_tmp->p_data;
}

void* pure_iocontextlist_item_get_bysyshandle(pure_list_p p_list, pure_sys_handle syshandle)
{
	pure_list_item_p p_tmp;

	if (p_list == 0)
		return 0;

	p_tmp = p_list->head;

	while (p_tmp)
	{
		if (((pure_event_context_p)p_tmp->p_data)->syshandle_file == syshandle)
		{
			break;
		}

		p_tmp = p_tmp->next;
	}

	return p_tmp->p_data;
}