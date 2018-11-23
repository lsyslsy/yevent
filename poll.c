#include <poll.h>
#include "event.h"

// select specific
int maxfd = FD_SETSIZE;
typedef struct select_fds {
  struct pollfd *fdarray;
} select_fds;

static void select_free_event(select_fds *api_state);

int select_create_event(eventloop *ep)
{
  struct select_fds *main_fds = malloc(sizeof(select_fds));
  if (main_fds == NULL)
    goto err;
   ep->api_state = main_fds;
   main_fds->fdarray = 0;
   main_fds->fdarray = malloc(ep->fd_size * sizeof(struct pollfd));
  return 0;
 err:
  if (main_fds)
    select_free_event(main_fds);
  return  -1;
}

static void select_free_event(select_fds *api_state)
{
  free(api_state->fdarray);
  free(api_state);
}



static int select_poll(eventloop *ep, struct timeval *tvp)
{
  int numevents = 0;
  select_fds *api_state = ep->api_state;
  // set the pollfd array
  int count = 0;
  for (int fd = 0; fd < ep->max_fd + 1; fd++) {
    event *e = &ep->events[fd];
    short poll_event = 0;
    if (e->flag & YEVENT_READ)
      poll_event |= POLL_IN;
    if (e->flag & YEVENT_WRITE)
      poll_event |= POLL_OUT;
    // fd got thing to write
    if (poll_event) {
      struct pollfd *pf = &api_state->fdarray[count];
      pf->fd = fd;
      pf->events = poll_event;
      pf->revents = 0;
      count++;
      // printf("count %d %d %x\n", count, pf->events, pf->fd);
    }
  }
  int millseconds = tvp->tv_sec * 1000 + tvp->tv_usec/1000;
  int ret = poll(api_state->fdarray, count, millseconds);
  if (ret == 0) {               /* nothing happen */
    goto end_err;
  } else if (ret < 0) {         /* ret = -1,some error happen */
    numevents = -1;
    goto end_err;
  } else  {
    int fired = 0;
    for (int i = 0; i < count; i++) {
      int flag = YEVENT_NONE;
      if (fired > ret)
        break;
      // can read
      struct pollfd *pf = &api_state->fdarray[i];
      if (pf->revents & POLL_IN) {
        flag |= YEVENT_READ;
      }
      // can write
      if (pf->revents & POLL_OUT) {
        flag |= YEVENT_WRITE;
      }
      // something happen with this fd
      if (flag != YEVENT_NONE) {
        ep->changed_events[fired].flag = flag;
        ep->changed_events[fired].fd = pf->fd;
        fired ++;
      }
    }
    numevents = fired;
  }

 end_err:
  return numevents;
}

static int select_add(eventloop *ep, int fd, int flag)
{
    return 0;
}

void select_del(eventloop *ep, int fd, int flag)
{
}
