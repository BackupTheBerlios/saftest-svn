#include "saftest_common.h"
#include "saftest_list.h"

saftest_list
saftest_list_create(void)
{
    saftest_list list2;
    list2 = calloc(sizeof(struct saftest_list_s), 1);
    return(list2);
}

void
saftest_list_delete(saftest_list *list)
{
    saftest_list_element element;

    if ((NULL != list) && (NULL != *list)) {
        while (NULL != (element = saftest_list_dequeue(*list))) {
            free(element);
        }
        free(*list);
        *list = NULL;
    }
}

void
saftest_list_delete_deep(saftest_list *list, void (*func)(void *data))
{
    saftest_list_element element;

    if ((NULL != list) && (NULL != *list)) {
        while (NULL != (element = saftest_list_dequeue(*list))) {
            (func)(saftest_list_element_get_data(element));
            free(element);
        }
        free(*list);
        *list = NULL;
    }
}

saftest_list_element
saftest_list_element_create(saftest_list list, void *data)
{
    saftest_list_element element;
    element = calloc(sizeof(struct saftest_list_element_s), 1);
    if (NULL == element) {
        return(NULL);
    }
    saftest_list_element_set_data(element, data);
    if (NULL != list) {
        saftest_list_enqueue(list, element);
    }
    return(element);
}

void
saftest_list_element_delete(saftest_list_element *element)
{
    if ((NULL != element) && (NULL != *element)) {
        if (NULL != (*element)->list) {
            saftest_list_remove((*element)->list, *element);
        }
        free(*element);
        *element = NULL;
    }
}

void
saftest_list_element_delete_deep(saftest_list_element *element,
                             void (*func)(void *data))
{
    if ((NULL != element) && (NULL != *element)) {
        if (NULL != (*element)->list) {
            saftest_list_remove((*element)->list, *element);
        }
        (func)(saftest_list_element_get_data(*element));
        free(*element);
        *element = NULL;
    }
}

void
saftest_list_element_delete_if(const saftest_list list,
                           int (*func)(void *data, void *key),
                           void *key)
{
    saftest_list_element element;
    saftest_list_element next_element;

    element = saftest_list_first(list);
    while (NULL != element) {
        next_element = saftest_list_next(element);
        if (TRUE == (func)(saftest_list_element_get_data(element), key)) {
            /*
             * It is the func's responsibility to free the data under
             * the element if it is returning TRUE.
             */
            saftest_list_element_delete(&element);
        }
        element = next_element;
    }
}

void
saftest_list_element_set_data(saftest_list_element element, void *data)
{
    element->data = data;
}

void *
saftest_list_element_get_data(const saftest_list_element element)
{
    if (NULL == element) {
        return(NULL);
    }
    return(element->data);
}

saftest_list_element
saftest_list_dequeue(saftest_list list)
{
    saftest_list_element element;

    element = list->head;

    if (NULL != element) {
        list->head = element->next;
        if (list->tail == element) {
            list->tail = NULL;
        } else if (NULL != list->head) {
            list->head->prev = NULL;
        }
        list->count--;
        element->next = NULL;
        element->prev = NULL;
        element->list = NULL;
    }

    return(element);
}

void
saftest_list_enqueue(saftest_list list, saftest_list_element element)
{
    if (NULL != element->list) {
        abort();
    }
    if (list->tail != NULL) {
        list->tail->next = element;
    }
    element->prev = list->tail;
    element->next = NULL;
    element->list = list;
    list->tail = element;
    if (NULL == list->head) {
        list->head = element;
    }
    list->count++;

    return;
}

void
saftest_list_insert(saftest_list list, saftest_list_element mark_element,
                saftest_list_element new_element)
{
    if (NULL != new_element->list) {
        abort();
    }
    if (NULL == mark_element) {
        if (NULL != list->head) {
            list->head->prev = new_element;
        }
        if (NULL == list->tail) {
            list->tail = new_element;
        }
        new_element->next = list->head;
        new_element->prev = NULL;
        new_element->list = list;
        list->head = new_element;
    }
    else {
        if (list->tail == mark_element) {
            list->tail = new_element;
        }
        if (NULL != mark_element->next) {
            mark_element->next->prev = new_element;
        }
        new_element->next = mark_element->next;
        new_element->prev = mark_element;
        new_element->list = list;
        mark_element->next = new_element;
    }
    list->count++;

    return;
}

void
saftest_list_remove(saftest_list list, saftest_list_element element)
{
    int           middle_req = 1;

    if (NULL == element) {
        abort();
    }
    if (element->list != list) {
        abort();
    }
    if (element == list->head) {
        list->head = element->next;
        if (NULL != list->head) {
            list->head->prev = NULL;
        }
        middle_req = 0;
    }
    if (element == list->tail) {
        list->tail = element->prev;
        if (NULL != list->tail) {
            list->tail->next = NULL;
        }
        middle_req = 0;
    }
    if (middle_req) {
        if (NULL != element->next) {
            element->next->prev = element->prev;
        }
        if (NULL != element->prev) {
            element->prev->next = element->next;
        }
    }

    list->count--;
    element->next = NULL;
    element->prev = NULL;
    element->list = NULL;

    return;
}

saftest_list_element
saftest_list_first(const saftest_list list)
{
    return(list->head);
}

saftest_list_element
saftest_list_last(const saftest_list list)
{
    return(list->tail);
}

saftest_list_element
saftest_list_next(const saftest_list_element element)
{
    return(element->next);
}

saftest_list_element
saftest_list_previous(const saftest_list_element element)
{
    return(element->prev);
}

saftest_list_element
saftest_list_find_data(const saftest_list list, void *data)
{
    saftest_list_element element;
    element = saftest_list_first(list);
    while (NULL != element) {
        if (saftest_list_element_get_data(element) == data) {
            return(element);
        }
        element = saftest_list_next(element);
    }
    return(NULL);
}

saftest_list_element
saftest_list_find(const saftest_list list,
              int (*test_func) (void *data, void *key),
              void *key,
              saftest_list_element start_element)
{
    saftest_list_element element;
    if (NULL != start_element) {
        element = start_element;
    }
    else {
        element = saftest_list_first(list);
    }
    while (NULL != element) {
        if ((test_func)(saftest_list_element_get_data(element), key) != 0) {
            return(element);
        }
        element = saftest_list_next(element);
    }
    return(NULL);
}

saftest_list
saftest_list_find_all(const saftest_list list,
                  int (*test_func) (void *data, void *key),
                  void *key)
{
    saftest_list         result;
    saftest_list_element element;
    void             *data;
    saftest_list_element new_element;

    result = saftest_list_create();
    if (NULL == result) {
        return(NULL);
    }

    element = saftest_list_first(list);
    while (NULL != element) {
        data = saftest_list_element_get_data(element);
        if ((test_func)(data, key) != 0) {
            new_element = saftest_list_element_create(result, data);
            if (NULL == new_element) {
                saftest_list_delete(&result);
                return(NULL);
            }
        }
        element = saftest_list_next(element);
    }
    return(result);
}

void
saftest_list_each(const saftest_list list, void (*func)(void *data, void *key), 
              void *key)
{
    saftest_list_element element;

    element = saftest_list_first(list);
    while (NULL != element) {
        (func)(saftest_list_element_get_data(element), key);
        element = saftest_list_next(element);
    }
}

saftest_list
saftest_list_map(const saftest_list list, void *(*map_func)(void *data, void *key),
             void (*delete_func)(void *data), void *key)
{
    saftest_list         result;
    saftest_list_element element;
    void             *data;
    saftest_list_element new_element;

    result = saftest_list_create();
    if (NULL == result) {
        return(NULL);
    }

    element = saftest_list_first(list);
    while (NULL != element) {
        new_element = saftest_list_element_create(result, NULL);
        if (NULL == new_element) {
            saftest_list_delete_deep(&result, delete_func);
            return(NULL);
        }
        data = (map_func)(saftest_list_element_get_data(element), key);
        if (NULL == data) {
            saftest_list_delete_deep(&result, delete_func);
            return(NULL);
        }
        saftest_list_element_set_data(new_element, data);
        element = saftest_list_next(element);
    }
    return(result);
}

unsigned int
saftest_list_size(const saftest_list list)
{
    return(list->count);
}

saftest_list
saftest_list_copy(const saftest_list list)
{
    saftest_list list_copy;
    saftest_list_element element;
    saftest_list_element element_copy;

    list_copy = saftest_list_create();
    if (NULL == list_copy) {
        return(NULL);
    }
    element = saftest_list_first(list);
    while (NULL != element) {
        element_copy = saftest_list_element_create(list_copy, element->data);
        if (NULL == element_copy) {
            saftest_list_delete(&list_copy);
            return(NULL);
        }
        element = saftest_list_next(element);
    }
    return(list_copy);
}

saftest_list
saftest_list_intersection(const saftest_list list1, const saftest_list list2,
                      int (*test_func)(void *data1, void *data2, void *key),
                      void *key)
{
    saftest_list         result;
    saftest_list_element element1;
    saftest_list_element element2;
    void             *data1;
    void             *data2;
    saftest_list_element element;
    int              found;

    result = saftest_list_create();
    if (NULL == result) {
        return(NULL);
    }

    element1 = saftest_list_first(list1);
    while (NULL != element1) {

        element2 = saftest_list_first(list2);
        while (NULL != element2) {

            data1 = saftest_list_element_get_data(element1);
            data2 = saftest_list_element_get_data(element2);

            found = FALSE;

            if (NULL == test_func) {
                if (data1 == data2) {
                    found = TRUE;
                }
            }
            else {
                if ((test_func)(data1, data2, key) != 0) {
                    found = TRUE;
                }
            }

            if (TRUE == found) {
                element = saftest_list_element_create(result, data1);
                if (NULL == element) {
                    saftest_list_delete(&result);
                    return(NULL);
                }
            }

            element2 = saftest_list_next(element2);
        }

        element1 = saftest_list_next(element1);
    }
    return(result);
} /* saftest_list_intersection() */

saftest_list
saftest_list_difference(const saftest_list list1, const saftest_list list2,
                    int (*test_func)(void *data1, void *data2, void *key),
                    void *key)
{
    saftest_list         result;
    saftest_list_element element1;
    saftest_list_element element2;
    void             *data1;
    void             *data2;
    saftest_list_element element;
    int              found;

    result = saftest_list_create();
    if (NULL == result) {
        return(NULL);
    }

    element1 = saftest_list_first(list1);
    while (NULL != element1) {

        found = FALSE;

        element2 = saftest_list_first(list2);
        while (NULL != element2) {

            data1 = saftest_list_element_get_data(element1);
            data2 = saftest_list_element_get_data(element2);

            if (NULL == test_func) {
                if (data1 == data2) {
                    found = TRUE;
                    break;
                }
            }
            else {
                if ((test_func)(data1, data2, key) != 0) {
                    found = TRUE;
                    break;
                }
            }
            element2 = saftest_list_next(element2);
        }

        if (FALSE == found) {
            element = saftest_list_element_create(result, data1);
            if (NULL == element) {
                saftest_list_delete(&result);
                return(NULL);
            }
        }

        element1 = saftest_list_next(element1);
    }
    return(result);
}
