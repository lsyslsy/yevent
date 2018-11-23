#include <sys/epoll.h>
#include "event.h"

// select specific
int maxfd = FD_SETSIZE;
typedef struct epoll_fds {
  struct epoll_event *event_array;
  int epollfd;
} epoll_fds;

static void select_free_event(epoll_fds *api_state);

int select_create_event(eventloop *ep)
{
  struct epoll_fds *main_fds = malloc(sizeof(epoll_fds));
  if (main_fds == NULL)
    goto err;
   ep->api_state = main_fds;
   main_fds->event_array = 0;
   main_fds->event_array = malloc(ep->fd_size * sizeof(struct epoll_event));
   main_fds->epollfd = epoll_create1(0);
   if (main_fds->epollfd == -1) {
     perror("epoll_create1");
     goto err;
     }
  return 0;
 err:
  if (main_fds)
    select_free_event(main_fds);
  return  -1;
}

static void select_free_event(epoll_fds *api_state)
{
  free(api_state->event_array);
  free(api_state);
}



static int select_poll(eventloop *ep, struct timeval *tvp)
{
  int numevents = 0;
  epoll_fds *api_state = ep->api_state;
  int millseconds = tvp->tv_sec * 1000 + tvp->tv_usec/1000;
  int ret = epoll_wait(api_state->epollfd, api_state->event_array, ep->fd_size, millseconds);
  if (ret == 0) {               /* nothing happen */
    goto end_err;
  } else if (ret < 0) {         /* ret = -1,some error happen */
    numevents = -1;
    goto end_err;
  } else  {
    int fired = 0;
    for (int i = 0; i < ret; i++) {
      int flag = YEVENT_NONE;
      // can read
      struct epoll_event *pf = &api_state->event_array[i];
      if (pf->events & EPOLLIN) {
        flag |= YEVENT_READ;
      }
      // can write
      if (pf->events & EPOLLOUT) {
        flag |= YEVENT_WRITE;
      }
      // something happen with this fd
      if (flag != YEVENT_NONE) {
        ep->changed_events[fired].flag = flag;
        ep->changed_events[fired].fd = pf->data.fd;
        fired ++;
      }
    }
    numevents = fired;
  }

 end_err:
  return numevents;
}

// todo deal with fd already exists
static int select_add(eventloop *ep, int fd, int flag)
{
  epoll_fds *api_state = ep->api_state;
  struct epoll_event ev = {0};
  ev.data.fd = fd;
  if (flag & YEVENT_READ) {
    ev.events |= EPOLLIN;
  }
  if (flag & YEVENT_WRITE) {
    ev.events |= EPOLLOUT;
  }
  // return 0 if success , -1 if error
  int ret = epoll_ctl(api_state->epollfd, EPOLL_CTL_ADD, fd, &ev);
  if (ret == -1) {
    perror("epoll_ctl");
  }
    return ret;
}

void select_del(eventloop *ep, int fd, int flag)
{
}
