#ifndef __IO2TTY_H_
#define __IO2TTY_H_

#include "timer.h"
#include "file_event.h"
#include "lockqueue.h"


typedef enum em4gmonitorMsg{
	IE_NONE = 0x00,
	IE_TIMER_CHECK = 0x01,
}em4gmonitorMsg_t;


typedef struct st4gmonitorEnv {
	struct file_event_table *fet;
	struct timer_head *th;
	struct timer step_timer;
	struct timer work_timer;

	stLockQueue_t msgq;


	char	sdev[64];
	int		sbuad;
	int		sfd;

}st4gmonitorEnv_t;




int		fgmonitor_init(void *_th, void *_fet, const char *_dev, int _buad);
int		fgmonitor_push_msg(int eid, void *param, int len);

int		fgmonitor_step();
void	fgmonitor_run(struct timer *timer);
void	fghandler_run(struct timer *timer);
void	fgmonitor_serial_in(void *arg, int fd);
void	fgmonitor_std_in(void *arg, int fd);
int		fgmonitor_handler_event(stEvent_t *event);






#endif
