#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <libusb.h>
#include <mosquitto.h>

#include "log.h"
#include "../common/constants.h"

#define USB_VID 0xca05
#define USB_PID 0x0001
#define STATUS_TOPIC "/public/eden/clubstatus"

volatile bool club_open = false;

void usb_push_status(void);

void set_status(bool status) {
	bool old_status = club_open;

	club_open = status;

	if(old_status != status)
		usb_push_status();
}

enum action {
	LOCK, UNLOCK, IDLE, POWERCYCLE
};

volatile enum action action = IDLE;

void sighandler(int sig) {
	switch(sig) {
		case SIGUSR1:
			action = LOCK;
			break;
		case SIGUSR2:
			action = UNLOCK;
			break;
		case SIGHUP:
			action = POWERCYCLE;
			break;
	}
}

char *getenv_or_die(const char *name) {
	char *ret = getenv(name);

	if(!ret)
		die("mandatory environment variable %s not defined", name);

	return ret;
}

void mqtt_connect_cb(struct mosquitto *mosq, void *userdata, int result) {
	(void) userdata;

	if(result != 0)
		pdie("MQTT connect failed with error %d", result);

	int ret = mosquitto_subscribe(mosq, NULL, STATUS_TOPIC, 1);
	if(ret != MOSQ_ERR_SUCCESS)
		pdie("topic subscribe failed: %s", mosquitto_strerror(ret));
}

void mqtt_msg_cb(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
	(void) mosq;
	(void) userdata;

	if(message->payloadlen) {
		char status = ((char*)message->payload)[0];
		switch(status) {
			case '0':
				set_status(false);
				break;
			case '1':
				set_status(true);
				break;
			default:
				error("got unknown status \"%c\"", status);
				break;
		}
	} else
		error("got empty status message");
}

struct mosquitto *mqtt_init(void) {
	const char *host = getenv_or_die("MQTT_HOST");

	mosquitto_lib_init();

	struct mosquitto *mosq = mosquitto_new(NULL, true, NULL);
	if(!mosq) {
		perror("creating mosquitto object failed");

		return NULL;
	}

	mosquitto_connect_callback_set(mosq, mqtt_connect_cb);
	mosquitto_message_callback_set(mosq, mqtt_msg_cb);

	int ret = mosquitto_connect(mosq, host, 1883, 10);
	if(ret != MOSQ_ERR_SUCCESS) {
		if(ret == MOSQ_ERR_ERRNO)
			perror("mosquitto_connect failed with errno");
		else
			error("mosquitto_connect failed");

		return NULL;
	}

	return mosq;
}

libusb_context *usb;
libusb_device_handle *usb_handle;

int usb_init(void) {
	int ret = libusb_init(&usb);
	if(ret) {
		error("libusb_init failed: %s", libusb_error_name(ret));
		return -1;
	}

	usb_handle = libusb_open_device_with_vid_pid(usb, USB_VID, USB_PID);
	if(!usb_handle) {
		error("couldn't find USB device");
		return -1;
	}

	return 0;
}

void usb_push_status(void) {
	int ret = libusb_control_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR,
			USB_REQ_STATUS, club_open ? 1 : 0, 0, NULL, 0, 500);

	if(ret < 0)
		die("usb_push_status failed: %s", libusb_error_name(ret));
}

void usb_force_lock(bool lock) {
	printf("lock forced to %s\n", lock ? "true" : "false");

	int ret = libusb_control_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR,
			USB_REQ_LOCK, lock ? 1 : 0, 0, NULL, 0, 500);

	if(ret < 0)
		die("usb_force_lock failed: %s", libusb_error_name(ret));
}

void usb_set_power(bool on) {
	int ret = libusb_control_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR,
			USB_REQ_POWER, on ? 1 : 0, 0, NULL, 0, 500);

	if(ret < 0)
		die("usb_set_power failed: %s", libusb_error_name(ret));
}

int main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	printf("usblockd rev " GIT_REV " starting up\n");

	if(usb_init() < 0)
		die("USB initialization failed");

	struct mosquitto *mosq = mqtt_init();
	if(!mosq)
		die("creating mosquitto object failed");

	struct sigaction sa;
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);

	while(1) {
		int ret;

		ret = mosquitto_loop(mosq, 1000, 1);
		if(ret != MOSQ_ERR_SUCCESS) {
			const char *errstr = mosquitto_strerror(ret);
			die("mosquitto loop failed: %s", errstr);
		}

		usb_push_status();

		if(action != IDLE) {
			if(action == LOCK)
				usb_force_lock(true);
			else if(action == UNLOCK)
				usb_force_lock(false);
			else if(action == POWERCYCLE) {
				usb_set_power(false);
				sleep(2);
				usb_set_power(true);
			}

			action = IDLE;
		}
	}
}
