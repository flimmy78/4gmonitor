#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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


int fgmonitor_usbdev_exsit();
int fgmonitor_usbdev_switch_to_ecm();
int fgmonitor_usbdev_mode_is_ecm();
int fgmonitor_usbdev_network_ok();
int fgmonitor_dial();

int fgmonitor_is_printable_buf(char *buf, int size);

//////////////////////////////////////////////////////////////////////////
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
	timer_set(env.th, &env.work_timer, 30 * 1000);
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
		if (!fgmonitor_usbdev_exsit()) {
			log_err("usb 4g device not exsit, please plug in the device");
			return -1;
		}

		
		int switch_cnt = 0;
re_switch:
		if (!fgmonitor_usbdev_mode_is_ecm()) {
			if (switch_cnt >= 3) {
				log_err("usb 4g device can't be switch to correct mode!!!");
				return -2;
			}
			switch_cnt++;
			if (fgmonitor_usbdev_switch_to_ecm() != 0) {
				log_warn("usb 4g device work mode not correct, trying switch it to correct mode failed");
				return -2;
			} 
			goto re_switch;
		} 
			

		int dial_cnt = 0;
re_dial:
		if (!fgmonitor_usbdev_network_ok()) {
			if (dial_cnt >= 3) {	
				log_err("usb 4g device can dialing correctlly!!!, maybe arrears!");
				return -3;
			}
			dial_cnt++;
			if (!fgmonitor_dial()) {
				log_warn("usb 4g device offline, trying to dialing failed!");
				return -3;
			}
			goto re_dial;
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

	//serial_write(env.sfd, buf, size, 80);

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
	if (fgmonitor_is_printable_buf(buf, ret)) {
		log_info("serialbuf: %s", buf);
	} else {
		log_debug_hex("serial buf:", buf, ret);
	}
	return;
}

//////////////////////////////////////////////////////////////////////////
int fgmonitor_usbdev_exsit() {
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
int fgmonitor_usbdev_switch_to_ecm() {
	return 0;
}
int fgmonitor_usbdev_mode_is_ecm() {
	/*
	const char pid1 = "0a12:0002";
	char sbuf[128];
	sprintf(sbuf, "lsusb | grep %s | wc -l", pid1);
	int x = scmd(sbuf);
	return x ? 1 : 0;
	*/
	return 0;
}
int fgmonitor_usbdev_network_ok() {
	/*
	char sbuf[128];
	sprintf(sbuf, "ifconfig usb0 | grep ...", pid1);
	int x = scmd(sbuf);
	*/
	return 0;
}
int fgmonitor_dial() {
	return 0;
}




///////////////////////////////////////////////////////////
int fgmonitor_is_printable_buf(char *buf, int size) {
	int i = 0;
	for (i = 0; i < size; i++) {
		if (!isprint(buf[i]&0xff)) {
			return 0;
		}
	}

	return 1;
}

