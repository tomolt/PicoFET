#ifndef PICOFET_CMD_H_
#define PICOFET_CMD_H_

struct jtdev; // declared somewhere else
struct comm; // declared somewhere else

#define FET_BUFFER_CAPACITY (64 * 1024)

extern unsigned char fet_buffer[FET_BUFFER_CAPACITY];

void send_status(struct comm *t, int status);
void command_loop(struct jtdev *p, struct comm *t);

#endif
