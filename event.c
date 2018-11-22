#include "event.h"


//#define USE_SELECT

#ifdef USE_SELECT
  #include "select.c"
#else
  #include "poll.c"
#endif
void free_event_loop(eventloop *ep)
{
  free(ep->events);
  free(ep->changed_events);
  select_free_event(ep->api_state);
  free(ep);
}


// create base event loop
eventloop *create_event_loop()
{
  eventloop *ep = malloc(sizeof(eventloop));
  if (ep == NULL) return NULL;
  ep->fd_size = EVENT_SET_SIZE;
  ep->max_fd = -1;
  ep->events = NULL;
  ep->changed_events = NULL;
  ep->api_state = NULL;

  ep->events = malloc(EVENT_SET_SIZE * sizeof(event));
  ep->changed_events = malloc(EVENT_SET_SIZE * sizeof(event));
  if (ep->events == NULL) goto err;
  if (ep->changed_events == NULL) goto err;

  if (select_create_event(ep) == -1)
    goto err;

  return ep;

 err:
  if (ep) {
    free_event_loop(ep);
  }
  return NULL;
}


int resize_event_set(int size)
{
}

// may replace the old callback
int event_add(eventloop *ep, int fd, int flag, event_callback read_call, event_callback write_call)
{
  if (fd > ep->fd_size) {
    printf("exceed max fd size");
    return -1;
  }
  event *e = &ep->events[fd];
  // 似乎不该在这里 TODO
  e->fd = fd;
  if (flag & YEVENT_READ) {
    if (e->flag & YEVENT_READ) {
      printf("event alreay add read %d\n", fd);
    }
    e->flag |= YEVENT_READ;
    e->read_call = read_call;
    select_add(ep, fd, YEVENT_READ);
   }
  if (flag & YEVENT_WRITE) {
    if (e->flag & YEVENT_WRITE) {
      printf("event alreay add write %d\n", fd);
    }
    e->flag |= YEVENT_WRITE;
    e->write_call = write_call;
    select_add(ep, fd, YEVENT_WRITE);
   }
  if (fd > ep->max_fd)
    ep->max_fd = fd;
  return 0;
}

static int event_del()
{
}

int event_loop(eventloop *ep)
{
  struct timeval tvp;
  while(1) {
    tvp.tv_sec = 1;
    tvp.tv_usec = 0;
    // get changed events
    int n = select_poll(ep, &tvp);
    // process the events
    if (n < 0) {
      printf("some error\n");
    } else if (n == 0) {
      printf("no data\n");
    } else {
      // if some event happens, then call the callback
      for (int i = 0; i < n; i++) {
        int fd = ep->changed_events[i].fd;
        int flag = ep->changed_events[i].flag;
        event *e = &ep->events[fd];

        // first process read
        if (e->flag & flag & YEVENT_READ) {
          e->read_call(ep, fd, flag, e->private_data);
        }
        if (e->flag & flag & YEVENT_WRITE) {
          e->write_call(ep, fd, flag, e->private_data);
        }
      }
    }
  }
  return 0;
}
void print_stdin(eventloop *ep, int fd, int flag, void *private_data)
{
  char buf[4096];
  int n = 0;
  n = read(fd, buf, sizeof(buf));
  if (n == 0) {
    printf("end of file\n");
    exit(1);
  } else if (n < 0) {
    printf("some error\n");
  } else {
    buf[n] = '\0';
    printf("read from: %s", buf);
  }
}

                                
int main()
{
  eventloop *ep = create_event_loop();
  event_add(ep, 0, YEVENT_READ, print_stdin, NULL);
  event_loop(ep);
}
