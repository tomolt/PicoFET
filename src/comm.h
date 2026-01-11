#include <stddef.h>

struct comm_func;
struct comm {
	struct comm_func *f;
};

struct comm_func {
	void   (*open)     (struct comm *t);
	size_t (*read_nb)  (struct comm *t, void *buf, size_t max);
	void   (*write)    (struct comm *t, const void *buf, size_t max);
	void   (*flush_out)(struct comm *t);
};
