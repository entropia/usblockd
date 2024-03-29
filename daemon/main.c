#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <libusb.h>
#include <mosquitto.h>

#include "log.h"
#include "../common/constants.h"

#define USB_VID 0xca05
#define USB_PID 0x0001
#define LOCK_TOPIC "/lock/club"
#define STATUS_TOPIC "/public/eden/clubstatus"
#define MQTT_RETRY_TIME 30

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
		pdie("topic subscribe failed: %s", mosquitto_strlogm(ret));
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
				logm("got unknown status \"%c\"", status);
				break;
		}
	} else
		logm("got empty status message");
}

struct mosquitto *mqtt_init(void) {
	struct mosquitto *mosq = mosquitto_new(NULL, true, NULL);
	if(!mosq) {
		perror("creating mosquitto object failed");

		return NULL;
	}

	mosquitto_connect_callback_set(mosq, mqtt_connect_cb);
	mosquitto_message_callback_set(mosq, mqtt_msg_cb);

	return mosq;
}

libusb_context *usb;
libusb_device_handle *usb_handle;

int usb_init(void) {
	int ret = libusb_init(&usb);
	if(ret) {
		logm("libusb_init failed: %s", libusb_error_name(ret));
		return -1;
	}

	usb_handle = libusb_open_device_with_vid_pid(usb, USB_VID, USB_PID);
	if(!usb_handle) {
		logm("couldn't find USB device");
		return -1;
	}

	return 0;
}

void usb_push_status(void) {
	int ret = libusb_control_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
			USB_REQ_STATUS, club_open ? 1 : 0, 0, NULL, 0, 500);

	if(ret < 0)
		die("usb_push_status failed: %s", libusb_error_name(ret));
}

void usb_force_lock(bool lock) {
	int ret = libusb_control_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
			USB_REQ_LOCK, lock ? 1 : 0, 0, NULL, 0, 500);

	if(ret < 0)
		die("usb_force_lock failed: %s", libusb_error_name(ret));
}

void usb_set_power(bool on) {
	int ret = libusb_control_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
			USB_REQ_POWER, on ? 1 : 0, 0, NULL, 0, 500);

	if(ret < 0)
		die("usb_set_power failed: %s", libusb_error_name(ret));
}

int usb_get_status(){
    uint16_t value = 0;
	int ret = libusb_control_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
			USB_GET_STATE, 0, 0, (unsigned char*)&value, 1, 500);

	if(ret < 0) {
        die("usb_get_status failed: %s", libusb_error_name(ret));
    }

    return value;
}

int main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	int ret;

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	printf("usblockd rev " GIT_REV " starting up\n");

	// initialize USB
	if(usb_init() < 0)
		die("USB initialization failed");

	// initialize MQTT
	mosquitto_lib_init();
	struct mosquitto *mosq = mqtt_init();

	const char *host = getenv_or_die("MQTT_HOST");
	time_t last_mqtt_try = time(NULL);

	const char *username = getenv("MQTT_USERNAME");
	const char *password = getenv("MQTT_PASSWORD");

	if (username != NULL && password != NULL) {
		mosquitto_username_pw_set(mosq, username, password);
	}

	ret = mosquitto_connect(mosq, host, 1883, 10);
	if(ret != MOSQ_ERR_SUCCESS) {
		if(ret == MOSQ_ERR_ERRNO)
			perror("mosquitto_connect failed with errno");
		else
			logm("mosquitto_connect failed");

		logm("retrying in %u seconds", MQTT_RETRY_TIME);
	}

	// register signal handlers
	struct sigaction sa;
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);

	// initialize hardware
	usb_push_status();
	usb_set_power(true);

	printf("initialization done, entering main loop\n");

	while(1) {
		// handle MQTT
		//
		// As mosquitto has no external API for checking for
		// connectedness, we have to run mosquitto_loop and check for
		// NO_CONN.

		ret = mosquitto_loop(mosq, 1000, 1);
		if(ret != MOSQ_ERR_SUCCESS) {
			if(ret == MOSQ_ERR_NO_CONN || ret == MOSQ_ERR_CONN_LOST) {
				sleep(1);

				time_t now = time(NULL);

				if(now - last_mqtt_try > MQTT_RETRY_TIME) {
					logm("MQTT is not connected, trying reconnect");

					last_mqtt_try = now;

					ret = mosquitto_reconnect(mosq);
					if(ret != MOSQ_ERR_SUCCESS)
						logm("reconnect failed, next try in %u seconds", MQTT_RETRY_TIME);
					else
						logm("reconnect succeeded");
				}
			} else {
				const char *errstr = mosquitto_strerror(ret);
				die("mosquitto loop failed: %s", errstr);
			}
		} else {
			/*
			 * we only push the status when MQTT is working and let
			 * the hardware time out on its own otherwise
			 */
			usb_push_status();
		}

        if (usb_get_status() == 1) {
            logm("door lock button pressed, send MQTT");
            mosquitto_publish(mosq, NULL, LOCK_TOPIC, 5, "close", 0, false);
            sleep(1);
        }

		// handle external requests
		if(action != IDLE) {
			if(action == LOCK) {
				logm("door lock requested");

				usb_force_lock(true);
			} else if(action == UNLOCK)
				usb_force_lock(false);
			else if(action == POWERCYCLE) {
				logm("reader powercycling requested");

				usb_set_power(false);
				sleep(2);
				usb_set_power(true);
			}

			action = IDLE;
		}
	}
}
