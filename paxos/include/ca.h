#ifndef _CA_H_
#define _CA_H_

#define __CARRAY_TYPE(name, type) \
	typedef struct {              \
		int h, t, s, c;           \
		type* a;                  \
	} carray_##name##_t;

#define __CARRAY_IMPL(name, type, scope)                           \
	scope carray_##name##_t* carray_##name##_new(int size) {       \
		carray_##name##_t* carray;                                 \
		carray = calloc(1, sizeof(struct carray_##name##_t));      \
		if (!carray) return NULL;                                  \
		carray->s = size;                                          \
		carray->array = calloc(size, sizeof(type*));               \
		return carray;                                             \
	}                                                              \
	                                                               \
	scope void carray_##name##_free(carray_##name##_t* carray) {   \
		free(a->array);                                            \
		free(a);                                                   \
	}

#define CARRAY_INIT(name, type)              \
	__CARRAY_TYPE(name, type)                \
	__CARRAY_IMPL(name, type, static inline) \

	// int
	// carray_empty(struct carray* a)
	// {
	// 	return a->count == 0;
	// }
	//
	// int
	// carray_size(struct carray* a)
	// {
	// 	return a->size;
	// }
	//
	// int
	// carray_push_back(struct carray* a, void* p)
	// {
	// 	if (carray_full(a))
	// 		carray_grow(a);
	// 	a->array[a->tail] = p;
	// 	a->tail = (a->tail + 1) % a->size;
	// 	a->count++;
	// 	return 0;
	// }
	//
	// void*
	// carray_pop_front(struct carray* a)
	// {
	// 	void* p;
	// 	if (carray_empty(a)) return NULL;
	// 	p = a->array[a->head];
	// 	a->head = (a->head + 1) % a->size;
	// 	a->count--;
	// 	return p;
	// }
	//
	// void
	// carray_foreach(struct carray* a, void (*carray_cb)(void*))
	// {
	// 	int i;
	// 	for (i = 0; i < a->count; ++i)
	// 		carray_cb(carray_at(a, i));
	// }
	//
	// static int
	// carray_full(struct carray* a)
	// {
	// 	return a->count == a->size;
	// }
	//
	// static void
	// carray_grow(struct carray* a)
	// {
	// 	int i;
	// 	struct carray* tmp = carray_new(a->size * 2);
	// 	for (i = 0; i < a->count; i++)
	// 		carray_push_back(tmp, carray_at(a, i));
	// 	free(a->array);
	// 	a->head = 0;
	// 	a->tail = tmp->tail;
	// 	a->size = tmp->size;
	// 	a->array = tmp->array;
	// 	free(tmp);
	// }
	//
	// static void*
	// carray_at(struct carray* a, int i)
	// {
	// 	return a->array[(a->head+i) % a->size];
	// }
	//
	
#endif 
