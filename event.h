#ifndef EVENT_H
#define EVENT_H


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define YEVENT_READ 0x01
#define YEVENT_WRITE 0x02
#define YEVENT_NONE 0x00

#define ERROR_INTR 0x01

#define EVENT_SET_SIZE  1024

struct eventloop;
// void func(int flag, fd)
typedef void (*event_callback)(struct eventloop *ep, int fd, int flag, void *private_data);

typedef struct event {
  int fd;
  int flag;
  event_callback read_call;
  event_callback write_call;
  void *private_data;
} event;

// common event loop
typedef struct eventloop {
  // pointer to implementation private data for poll, select ...
  void * api_state;
  int max_fd;
  event *events;                /* current monitor event, size is fd_count */
  event *changed_events;        /* current changed events */
  int fd_size;
} eventloop;

#endif
