#include <sys/select.h>
#include "event.h"

// select specific
int maxfd = FD_SETSIZE;
typedef struct select_fds {
  fd_set readfds;
  fd_set writefds;
  //fd_set exceptfds;
  fd_set _readfds;
  fd_set _writefds;
} select_fds;


static void select_free_event(select_fds *api_state)
{
  free(api_state);
}

int select_create_event(eventloop *ep)
{
  struct select_fds *main_fds = malloc(sizeof(select_fds));
  if (main_fds == NULL)
    return -1;
  FD_ZERO(&main_fds->readfds);
  FD_ZERO(&main_fds->writefds);
  ep->api_state = main_fds;
  return 0;
}



static int select_poll(eventloop *ep, struct timeval *tvp)
{
  int numevents = 0;
  select_fds *api_state = ep->api_state;
  // backup the fdsets, select may change them
  memcpy(&api_state->_readfds, &api_state->readfds, sizeof(fd_set));
  memcpy(&api_state->_writefds, &api_state->writefds, sizeof(fd_set));

  // select the fds
  int ret = select(ep->max_fd+1, &api_state->_readfds, &api_state->_writefds, NULL, tvp);
  if (ret == 0) {               /* nothing happen */
    goto end_err;
  } else if (ret < 0) {         /* ret = -1,some error happen */
    numevents = -1;
    goto end_err;
  } else  {
    int fired = 0;
    int flag = YEVENT_NONE;
    for (int fd = 0; fd < ep->max_fd + 1; fd++) {
      if (fired > ret)
        break;
      // can read
      if (FD_ISSET(fd, &api_state->readfds)) {
        flag |= YEVENT_READ;
      }
      // can write
      if (FD_ISSET(fd, &api_state->readfds)) {
        flag |= YEVENT_WRITE;
      }
      // something happen with this fd
      if (flag != YEVENT_NONE) {
        ep->changed_events[fired].flag = flag;
        ep->changed_events[fired].fd = fd;
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
  select_fds *api_state = ep->api_state;
  if (flag & YEVENT_READ) {
    FD_SET(fd, &api_state->readfds);
  }
  if (flag & YEVENT_WRITE) {
    FD_SET(fd, &api_state->writefds);
  }
  return 0;
}

void select_del(eventloop *ep, int fd, int flag)
{
  select_fds *api_state = ep->api_state;
  if (flag & YEVENT_READ) {
    FD_CLR(fd, &api_state->readfds);
  }
  if (flag & YEVENT_WRITE) {
    FD_CLR(fd, &api_state->writefds);
  }
}
