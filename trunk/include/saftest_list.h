#ifndef _SAFTEST_LIST_INCLUDED
#define _SAFTEST_LIST_INCLUDED

/* forward declaration */
struct saftest_list_s;

struct saftest_list_element_s {
    struct saftest_list_s         *list;
    struct saftest_list_element_s *next;
    struct saftest_list_element_s *prev;
    void                          *data;
};
typedef struct saftest_list_element_s *saftest_list_element;

struct saftest_list_s {
    saftest_list_element head;
    saftest_list_element tail;
    int                  count;
};
typedef struct saftest_list_s *saftest_list;

/**
 * Allocates a new list.
 */
extern saftest_list saftest_list_create(void);

/**
 * Frees all list memory associated with the list2.
 *
 * IMPORTANT: This function does not free any element data pointers, just the list
 * and list element objects.
 */
extern void saftest_list_delete(saftest_list *list);

/**
 * Frees all memory associated with the list2.
 *
 * This function calls func() to free any element data.
 */
extern void saftest_list_delete_deep(saftest_list *list, void (*func)(void *data));

/**
 * Allocates a new list element,
 * assigns it's data to the pointer provided
 * and puts it at the tail of the list.
 *
 * If list is NULL, it just creates the list element without putting it in the
 * list. This is useful when the element is subsequently going to be inserted.
 */
extern saftest_list_element saftest_list_element_create(saftest_list list, void *data);

/**
 * Removes the specified element from the list and
 * frees the memory used by it.
 */
extern void saftest_list_element_delete(saftest_list_element *element);

extern void saftest_list_element_delete_deep(saftest_list_element *element,
                                         void (*func)(void *data));

/**
 * This will go through each element in the list looking for matching
 * elements as determined by the caller-specified function.  This function
 * must return TRUE or FALSE.  If it returns TRUE, the element will be
 * deleted from the list.  It is the responsibility of the func to free any
 * data underneath the element if the function is returning TRUE. The key
 * is a data pointer that can be passed to the caller-specified function.
 * This is useful for passing information to or between each execution of
 * the caller-specified function.
 */ 
extern void saftest_list_element_delete_if(const saftest_list list,
                                      int (*func)(void *data, void *key),
                                      void *key);

/**
 * Sets the data pointer for the list element.
 */
extern void saftest_list_element_set_data(saftest_list_element element, void *data);

/**
 * Gets the data pointer for the list element.
 */
extern void * saftest_list_element_get_data(const saftest_list_element element);

/**
 * Takes the first element off the list and returns it.
 */
extern saftest_list_element saftest_list_dequeue(saftest_list list);

/**
 * Appends the specified element to the tail of the list.
 */
extern void saftest_list_enqueue(saftest_list list, saftest_list_element element);

/**
 * Inserts the new_element at the position in the
 * list just after the mark_element. If mark_element is NULL,
 * new_element is inserted at the head of the list.
 */
extern void saftest_list_insert(saftest_list list, saftest_list_element mark_element,
                            saftest_list_element new_element);

/**
 * Removes the specified item from the list but
 * does not free the memory for it.
 */
extern void saftest_list_remove(saftest_list list, saftest_list_element element);

/**
 * Returns the first element in the list or NULL if the list is empty.
 */
extern saftest_list_element saftest_list_first(const saftest_list list);

/**
 * Returns the last element in the list or NULL if the list is empty.
 */
extern saftest_list_element saftest_list_last(const saftest_list list);

/**
 * Returns the next element in the list or NULL if this is the last element.
 */
extern saftest_list_element saftest_list_next(const saftest_list_element element);

/**
 * Returns the previous element in the list or NULL if this is the first
 * element.
 */
extern saftest_list_element saftest_list_previous(const saftest_list_element element);

/**
 * Returns the first element that has the specified data pointer.
 */
extern saftest_list_element saftest_list_find_data(const saftest_list list, void *data);

/**
 * Returns the first element that causes the test_func to return non-zero.
 *
 * If start_element is specified, that is the first element we start looking
 * from. The key argument is just passed through to the test_func().
 */
extern saftest_list_element saftest_list_find(const saftest_list list,
                                      int (*test_func)(void *data, void *key),
                                      void *key,
                                      saftest_list_element start_element);

/**
 * Returns a new list with elements that causes the test_func to return non-zero.
 */
extern saftest_list saftest_list_find_all(const saftest_list list,
                                  int (*test_func)(void *data, void *key),
                                  void *key);

/**
 * Performs the function on each element in the list.
 */
extern void saftest_list_each(const saftest_list list,
                          void (*func)(void *data, void *key),
                          void *key);

/**
 * Returns a new list with elements whose data values are the results of the map_func().
 *
 * It is the responsibility of the caller to ensure that the memory used for
 * the data values for the new list elements are freed, perhaps via
 * saftest_list_delete_deep(). Any failures within this function will result in
 * saftest_list_delete_deep() being called with the delete_func().
 * If the map_func returns NULL, it is treated as a failure.
 */
extern saftest_list saftest_list_map(const saftest_list list,
                             void *(*map_func)(void *data, void *key),
                             void (*delete_func)(void *data),
                             void *key);

/**
 * Returns the number of elements in the list.
 */
extern unsigned int saftest_list_size(const saftest_list list);

/**
 * Creates a copy of the list but does not duplicate the element data.
 */
extern saftest_list saftest_list_copy(const saftest_list list);

/**
 * Creates a new list containing all elements that are in both list1
 * and list2.
 *
 * If test_func is NULL, then the element data pointers are compared.
 * If test_func is non-NULL, it is used to decide when elements are equal.
 * A non-zero return from the test_func means the two elements are equal.
 * The key is just passed through to the test_func.
 */
extern saftest_list saftest_list_intersection(const saftest_list list1,
                                      const saftest_list list2,
                                      int (*test_func)(void *data1, void *data2,
                                                       void *key),
                                      void *key);

/**
 * Creates a new list containing all elements that are in list1 that are not
 * in list2.
 *
 * If test_func is NULL, then the element data pointers are compared.
 * If test_func is non-NULL, it is used to decide when elements are equal.
 * A non-zero return from the test_func means the two elements are equal.
 * The key is just passed through to the test_func.
 */
extern saftest_list saftest_list_difference(const saftest_list list1,
                                    const saftest_list list2,
                                    int (*test_func)(void *data1, void *data2,
                                                     void *key),
                                    void *key);

#endif /* _SAFTEST_LIST_INCLUDED */
