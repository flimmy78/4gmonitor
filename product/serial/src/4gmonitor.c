
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "common.h"
#include "log.h"

#include "4gmonitor.h"
#include "serial.h"



static  st4gmonitorEnv_t env = {
	.sdev = "/dev/ttyS1", 
	.sbuad = 115200,
	.sfd = -1,
};


int check_4g_usbdev_exsit();
int check_4g_usbdev_mode_is_yyy();
int switch_4g_usbdev_to_xxx();
int check_4g_usbdev_mode_is_xxx();
int check_4g_network_ok();
int dial_4g();
int check_4g_network_ok();




int		fgmonitor_init(void *_th, void *_fet, const char *_dev, int _buad) {
	env.th = _th;
	env.fet = _fet;
	
	timer_init(&env.work_timer, fgmonitor_run);
	timer_init(&env.step_timer, fghandler_run);

	lockqueue_init(&env.msgq);


	/* serial */
	strcpy(env.sdev, _dev);
	env.sbuad = _buad;
	int ret =  serial_open(env.sdev, env.sbuad);
	if (ret < 0) {
		log_err("[%d] open serial %s(%d) error: %d", __LINE__, env.sdev, env.sbuad, ret);
		return -1;
	}
	env.sfd = ret;
  file_event_reg(env.fet, env.sfd, fgmonitor_serial_in, NULL, NULL);

	/* io */
  file_event_reg(env.fet, 0, fgmonitor_std_in, NULL, NULL);
	return 0;
}

int		fgmonitor_step() {
	timer_cancel(env.th, &env.step_timer);
	timer_set(env.th, &env.step_timer, 10);
	return 0;
}

int		fgmonitor_push_msg(int eid, void *param, int len) {
	if (eid == eid) {
		stEvent_t *e = MALLOC(sizeof(stEvent_t));
		if (e == NULL) {
			 return -1;
		}
		e->eid = eid;
		e->param = param;
		lockqueue_push(&env.msgq, e);
		fgmonitor_step();
	}
	return 0;
}

void	fgmonitor_run(struct timer *timer) {
	fgmonitor_push_msg(IE_TIMER_CHECK, NULL, 0);
	timer_set(env.th, &env.work_timer, 30);
}
void	fghandler_run(struct timer *timer) {
	stEvent_t *e = NULL;
	if (lockqueue_pop(&env.msgq, (void **)&e) && e != NULL) {
		fgmonitor_handler_event(e);
		FREE(e);
		fgmonitor_step();
	}
}


int fgmonitor_handler_event(stEvent_t *event) {
	log_info("[%d] fgmonitor module now not support event handler, only free the event!!!", __LINE__);
	if (event->eid == IE_TIMER_CHECK) {
		if (!check_4g_usbdev_exsit()) {
			return -1;
		}

		if (check_4g_usbdev_mode_is_yyy()) {
			switch_4g_usbdev_to_xxx();
		}

		if (!check_4g_usbdev_mode_is_xxx()) {
			return -2;
		}

		if (!check_4g_network_ok()) {
			dial_4g();
		}

		if (!check_4g_network_ok()) {
			log_warn("4g network not ok!");
		} 
	}
	return 0;
}



void	fgmonitor_std_in(void *arg, int fd) {
	char buf[1024];

	int ret = read(0, buf, sizeof(buf));
	if (ret <= 0) {
		log_warn("what happend?");
		return;
	}

	int size = ret;
	buf[size] = 0;

	serial_write(env.sfd, buf, size, 80);

	log_debug(">$");
}

void	fgmonitor_serial_in(void *arg, int fd) {
	char buf[1024];
	int ret = serial_read(env.sfd, buf, sizeof(buf), 80);
	if (ret <= 0) {
		log_warn("[%d] can't recv any bytes from serial : %d", __LINE__, ret);
		return;
	}

	buf[ret] = 0;
	log_info("%s", buf);
	return;
}


int check_4g_usbdev_exsit() {
	//Bus 001 Device 005: ID 0a12:0001
	/*
	const char *pid1 = "0a12:0001";
	const char *pid2 = "0a12:0002";
	char sbuf[128];

	
	sprintf(sbuf, "lsusb | grep %s | wc -l", pid1);
	int x = scmd(sbuf);
	if (x != 0) {
		return 1;
	}

	sprintf(sbuf, "lsusb | grep %s | wc -l", pid2);
	int x = scmd(sbuf);
	if (x != 0) {
		return 1;
	}
	*/

	return 0;
}
int check_4g_usbdev_mode_is_yyy() {
	/*
	const char pid1 = "0a12:0002";
	char sbuf[128];
	sprintf(sbuf, "lsusb | grep %s | wc -l", pid1);
	int x = scmd(sbuf);
	return x ? 1 : 0;
	*/
	return 0;
}
int check_4g_usbdev_mode_is_xxx() {
	/*
	const char pid1 = "0a12:0001";
	char sbuf[128];
	sprintf(sbuf, "lsusb | grep %s | wc -l", pid1);
	int x = scmd(sbuf);
	return x ? 1 : 0;
	*/
	return 0;
}
int switch_4g_usbdev_to_xxx() {
	return 0;
}
int check_4g_network_ok() {
	/*
	char sbuf[128];
	sprintf(sbuf, "ifconfig usb0 | grep ...", pid1);
	int x = scmd(sbuf);
	*/
	return 0;
}
int dial_4g() {
	return 0;
}


