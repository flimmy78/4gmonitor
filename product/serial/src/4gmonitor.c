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

int scmd(char *cmd, char *buf, int size);

int fgmonitor_is_printable_buf(char *buf, int size);

//////////////////////////////////////////////////////////////////////////
int		fgmonitor_init(void *_th, void *_fet, const char *_dev, int _buad) {
	env.th = _th;
	env.fet = _fet;

	strcpy(env.sdev, _dev);
	env.sbuad = _buad;
	
	timer_init(&env.work_timer, fgmonitor_run);
	timer_init(&env.step_timer, fghandler_run);

	lockqueue_init(&env.msgq);

	timer_set(env.th, &env.work_timer, 10);

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
		e->data = param;
		e->len = len;
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
	//log_info("[%d] fgmonitor module now not support event handler, only free the event!!!", __LINE__);

	if (event->eid != IE_TIMER_CHECK) {
		return -1;
	}

	if (!fgmonitor_usbdev_exsit()) {
		log_err("usb 4g device not exsit, please plug in the device");
		return -2;
	}

		
	if (!fgmonitor_usbdev_mode_is_ecm()) {
		log_err("usb 4g deice mode is not corrent, try to switch it ..");
		fgmonitor_usbdev_switch_to_ecm();
		return -3;
	}


	if (!fgmonitor_usbdev_network_ok()) {
		log_warn("usb 4g device offline, trying to dialing ...");
		fgmonitor_dial();
		return -4;
	}

	log_info("usb 4g ok!");

	return 0;
}




//////////////////////////////////////////////////////////////////////////
int fgmonitor_usbdev_exsit() {
	//Bus 001 Device 005: ID 0a12:0001

	/*
	const char *pids[] = {
		"19d2:1432",
		"19d2:1433",
		"19d2:1476",
		"19d2:1509",
	};
	*/

	char cmd[256];
	char buf[256];
	
	sprintf(cmd, "lsusb | grep \"%s\" | wc -l", "19d2:");
	scmd(cmd, buf, sizeof(buf));
	if (buf[0] == '0') {
		return 0;
	}

	return 1;
}
int fgmonitor_usbdev_mode_is_ecm() {
	const char *ecm_pid = "19d2:1476";

	char cmd[256];
	char buf[256];
	
	sprintf(cmd, "lsusb | grep \"%s\" | wc -l", ecm_pid);
	scmd(cmd, buf, sizeof(buf));
	if (buf[0] == '0') {
		return 0;
	}

	return 1;

}

int fgmonitor_usbdev_switch_to_ecm() {
	
	const char *dev = "/dev/ttyUSB1";
	int fd = serial_open(dev, 115200);
	if (fd < 0) {
		return -1;
	}

	const char *ATCMD = "AT+ZSWITCH=L\r";
	int ret = serial_write(fd, (char *)ATCMD, strlen(ATCMD), 80);
	if (ret <= 0) {
		serial_close(fd);
		return -2;
	}

	char buf[1024];
	 ret = serial_read(fd, buf, sizeof(buf), 4080);
	if (ret <= 0) {
		serial_close(fd);
		return -3;
	}
	buf[ret] = 0;
	log_debug("======[%s] AT RETRUN:=====", ATCMD);
	log_debug("\n[%s]", buf);
	log_debug("=====================");

	serial_close(fd);


	return 0;
}
int fgmonitor_usbdev_network_ok() {
	char cmd[256];
	char buf[256];
	
	sprintf(cmd, "ping 8.8.8.8 -4 -c 1  -W %d | "
							 "grep \"100%% packet loss\" | wc -l", 8);
	scmd(cmd, buf, sizeof(buf));
	if (buf[0] == '1') {
		return 0;
	}

	return 1;
}
int fgmonitor_dial() {
	const char *dev = "/dev/ttyUSB1";
	int fd = serial_open(dev, 115200);
	if (fd < 0) {
		return -1;
	}

	const char *ATCMD = "ATI\r";
	int ret = serial_write(fd, (char *)ATCMD, strlen(ATCMD), 80);
	sleep(2);
	if (ret <= 0) {
		serial_close(fd);
		return -2;
	}
	char buf[1024];
	 ret = serial_read(fd, buf, sizeof(buf), 4080);
	if (ret <= 0) {
		serial_close(fd);
		return -3;
	}
	buf[ret] = 0;
	log_debug("======[%s] AT RETRUN:=====", "ATI");
	log_debug("\n[%s]", buf);
	log_debug("=====================");
	if (strcmp(buf, "\nERROR\n") == 0) {
		serial_close(fd);
		return -3;
	}



	ATCMD = "AT+CGDCONT=1,\"IP\",\"CMNET\"\r";
	ret = serial_write(fd, (char *)ATCMD, strlen(ATCMD), 80);
	sleep(2);
	if (ret <= 0) {
		serial_close(fd);
		return -2;
	}
	 ret = serial_read(fd, buf, sizeof(buf), 4080);
	if (ret <= 0) {
		serial_close(fd);
		return -3;
	}
	buf[ret] = 0;
	log_debug("======[%s] AT RETRUN:=====", "AT+CGDCONT=1,\"IP\",\"CMNET\"");
	log_debug("\n[%s]", buf);
	log_debug("=====================");

	log_debug_hex("BUF:", buf, ret);
	if (strcmp(buf, "\r\nERROR\n\n") == 0) {
		serial_close(fd);
		return -3;
	}


	ATCMD = "AT+ZECMCALL=1\r";
	ret = serial_write(fd, (char *)ATCMD, strlen(ATCMD), 80);
	sleep(2);
	if (ret <= 0) {
		serial_close(fd);
		return -2;
	}
	 ret = serial_read(fd, buf, sizeof(buf), 4080);
	if (ret <= 0) {
		serial_close(fd);
		return -3;
	}
	buf[ret] = 0;
	log_debug("======[%s] AT RETRUN:=====", "AT+ZECMCALL=1");
	log_debug("\n[%s]", buf);
	log_debug("=====================");

	if (strcmp(buf, "\r\nERROR\r\n") == 0) {
		serial_close(fd);
		return -3;
	}


	serial_close(fd);

	
	system("dhcpcd usb0");
	sleep(2);

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

int scmd(char *cmd, char *buf, int size) {
	FILE *fp = popen(cmd, "r");
	if (fp == NULL) {
		return -1;
	}

	char *s = fgets(buf, size, fp);
	if (s == NULL) {
		return -2;
	}
	
	int len = strlen(s);
	if (len > 1) {
		s[len-1] = 0;
	}

	log_debug("cmd : \n[%s].", cmd);
	log_debug("ret : \n[%s].", buf);

	pclose(fp);
	
	return 0;
}

