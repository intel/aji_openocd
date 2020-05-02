/*
 *   Driver for USB-JTAG, Altera USB-Blaster and compatibles
 *
 *   Inspired from original code from Kolja Waschk's USB-JTAG project
 *   (http://www.ixo.de/info/usb_jtag/), and from openocd project.
 *
 *   Copyright (C) 2013 Franck Jullien franck.jullien@gmail.com
 *   Copyright (C) 2012 Robert Jarzmik robert.jarzmik@free.fr
 *   Copyright (C) 2011 Ali Lown ali@lown.me.uk
 *   Copyright (C) 2009 Catalin Patulea cat@vv.carleton.ca
 *   Copyright (C) 2006 Kolja Waschk usbjtag@ixo.de
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef OPENOCD_JTAG_DRIVERS_JTAGSERV_JTAGSERV_ACCESS_H
#define OPENOCD_JTAG_DRIVERS_JTAGSERV_JTAGSERV_ACCESS_H

/* Low level flags */
#define COPY_TDO_BUFFER		(1 << 0)

struct jtagserv_lowlevel {
	uint16_t jtagserv_vid;
	uint16_t jtagserv_pid;
	uint16_t jtagserv_vid_uninit;
	uint16_t jtagserv_pid_uninit;
	char *jtagserv_device_desc;
	struct libusb_device_handle *libusb_dev;
	char *firmware_path;

	int (*write)(struct jtagserv_lowlevel *low, uint8_t *buf, int size,
		     uint32_t *bytes_written);
	int (*read)(struct jtagserv_lowlevel *low, uint8_t *buf, unsigned size,
		    uint32_t *bytes_read);
	int (*open)(struct jtagserv_lowlevel *low);
	int (*close)(struct jtagserv_lowlevel *low);
	int (*speed)(struct jtagserv_lowlevel *low, int speed);

	void *priv;
	int flags;
};

/**
 * jtagserv_register_libusb - get a lowlevel USB Blaster II driver
 *
 * Get a lowlevel USB-Blaster driver. In the current implementation, there are 2
 * possible lowlevel drivers :
 *  - one based on libftdi,
 *  - one based on libusb, specific to the USB-Blaster II
 *
 * Returns the lowlevel driver structure.
 */
extern struct jtagserv_lowlevel *jtagserv_register_libusb(void);

#endif /* OPENOCD_JTAG_DRIVERS_JTAGSERV_JTAGSERV_ACCESS_H */
