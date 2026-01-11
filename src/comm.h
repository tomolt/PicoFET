#include <stddef.h>

// Communication with host

struct comm_func;
struct comm {
	struct comm_func *f;
};

struct comm_func {
	// Establish the communication.
	void   (*comm_open)     (struct comm *t);
	// Read bytes from the host, non-blocking.
	// Returns the number of bytes received.
	size_t (*comm_read_nb)  (struct comm *t, void *buf, size_t max);
	// Write bytes to the host, blocking.
	// Output is buffered, and must be flushed to reach the host.
	void   (*comm_write)    (struct comm *t, const void *buf, size_t max);
	// Flush the output buffer.
	void   (*comm_flush_out)(struct comm *t);
};
