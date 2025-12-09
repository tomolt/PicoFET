#ifndef TRANSPORT_H_
#define TRANSPORT_H_

struct transport_func;
struct transport {
	const struct transport_func *f;
};

struct transport_func {
	int (*send)(struct transport *t, const char *data, int len);
	int (*recv)(struct transport *t, char *data, int len);
	int (*flush)(struct transport *t);
};

#endif
