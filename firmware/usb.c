#include <stdlib.h>

#include <libopencm3/usb/usbd.h>

#include "state.h"
#include "lock.h"
#include "usb.h"
#include "constants.h"

static usbd_device *usbd_dev;

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0xff,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0xca05,
	.idProduct = 0x0001,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

const struct usb_interface_descriptor iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = 0xFF,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,
};

const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
		.altsetting = &iface,
}};

const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

const char *usb_strings[] = {
	"Entropia e.V.",
	"Tuerschloss",
	"1",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static int control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
	(void)buf;
	(void)len;
	(void)complete;
	(void)usbd_dev;

	if(req->bRequest == USB_REQ_STATUS) {
		if(req->wValue & 1)
			set_state(OPEN);
		else
			set_state(CLOSED);

		return 1;
	} else if(req->bRequest == USB_REQ_LOCK) {
		if(req->wValue & 1)
			lock();
		else
			unlock();
	}

	return 0;
}


void init_usb(void) {
	usbd_dev = usbd_init(&stm32f103_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_control_callback(
			usbd_dev,
			USB_REQ_TYPE_VENDOR,
			USB_REQ_TYPE_TYPE,
			control_request);

}

void usb_poll(void) {
	usbd_poll(usbd_dev);
}
