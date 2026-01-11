#include <stddef.h>

struct transport_func;
struct transport {
	struct transport_func *f;
};

struct transport_func {
	void   (*open)     (struct transport *t);
	size_t (*read_nb)  (struct transport *t, void *buf, size_t max);
	void   (*write)    (struct transport *t, const void *buf, size_t max);
	void   (*flush_out)(struct transport *t);
};
