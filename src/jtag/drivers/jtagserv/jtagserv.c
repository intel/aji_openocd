/*
 *   Driver for USB-JTAG, Altera JTAGSERV and compatibles
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

/*
 * The following information is originally from Kolja Waschk's USB-JTAG,
 * where it was obtained by reverse engineering an Altera JTAGSERV.
 * See http://www.ixo.de/info/usb_jtag/ for JTAGSERV block diagram and
 * usb_jtag-20080705-1200.zip#usb_jtag/host/openocd for protocol.
 *
 * The same information is also on the UrJTAG mediawiki, with some additional
 * notes on bits marked as "unknown" by usb_jtag.
 * (http://sourceforge.net/apps/mediawiki/urjtag/index.php?
 *    title=Cable_Altera_JTAGSERV)
 * USB-JTAG, Altera JTAGSERV are typically implemented as a Cypress
 * EZ-USB FX2LP followed by a CPLD.
 *            _____________    _________
 *           |             |  |         |
 *      USB__| EZ-USB FX2  |__| EPM570  |__JTAG (B_TDO,B_TDI,B_TMS,B_TCK)
 *           |_____________|  |_________|
 *            __|__________
 *           |             |
 *           | 24 MHz XTAL |
 *           |_____________|
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if IS_CYGWIN == 1
#include "windows.h"
#undef LOG_ERROR
#endif

/* project specific includes */
#include <jtag/interface.h>
#include <jtag/commands.h>
#include <helper/time_support.h>
#include "jtagserv_access.h"

/* system includes */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

/* Size of USB endpoint max packet size, ie. 64 bytes */
#define MAX_PACKET_SIZE 64
/*
 * Size of data buffer that holds bytes in byte-shift mode.
 * This buffer can hold multiple USB packets aligned to
 * MAX_PACKET_SIZE bytes boundaries.
 * BUF_LEN must be grater than or equal MAX_PACKET_SIZE.
 */
#define BUF_LEN 4096

/* JTAGSERV II specific command */
#define CMD_COPY_TDO_BUFFER	0x5F

enum gpio_steer {
	FIXED_0 = 0,
	FIXED_1,
	SRST,
	TRST,
};

struct jtagserv_info {
	enum gpio_steer pin6;
	enum gpio_steer pin8;
	int tms;
	int tdi;
	bool trst_asserted;
	bool srst_asserted;
	uint8_t buf[BUF_LEN];
	int bufidx;

	char *lowlevel_name;
	struct jtagserv_lowlevel *drv;
	char *jtagserv_device_desc;
	uint16_t jtagserv_vid, jtagserv_pid;
	uint16_t jtagserv_vid_uninit, jtagserv_pid_uninit;
	int flags;
	char *firmware_path;
};

/*
 * Global device control
 */
static struct jtagserv_info info = {
	.jtagserv_vid = 0x09fb, /* Altera */
	.jtagserv_pid = 0x6001, /* JTAGSERV */
	.lowlevel_name = "jtagserv",
	.srst_asserted = false,
	.trst_asserted = false,
	.pin6 = FIXED_1,
	.pin8 = FIXED_1,
};

/*
 * Available lowlevel drivers (FTDI, FTD2xx, ...)
 */
struct drvs_map {
	char *name;
	struct jtagserv_lowlevel *(*drv_register)(void);
};

static struct drvs_map lowlevel_drivers_map[] = {
#if BUILD_JTAGSERV
	{ .name = "jtagserv", .drv_register = jtagserv_register_libusb },
#endif
	{ NULL, NULL },
};

/*
 * Access functions to lowlevel driver, agnostic of libftdi/libftdxx
 */
static char *hexdump(uint8_t *buf, unsigned int size)
{
	unsigned int i;
	char *str = calloc(size * 2 + 1, 1);

	for (i = 0; i < size; i++)
		sprintf(str + 2*i, "%02x", buf[i]);
	return str;
}

static int jtagserv_buf_read(uint8_t *buf, unsigned size, uint32_t *bytes_read)
{
	int ret = info.drv->read(info.drv, buf, size, bytes_read);
	char *str = hexdump(buf, *bytes_read);

	LOG_DEBUG_IO("(size=%d, buf=[%s]) -> %u", size, str,
		      *bytes_read);
	free(str);
	return ret;
}

static int jtagserv_buf_write(uint8_t *buf, int size, uint32_t *bytes_written)
{
	int ret = info.drv->write(info.drv, buf, size, bytes_written);
	char *str = hexdump(buf, *bytes_written);

	LOG_DEBUG_IO("(size=%d, buf=[%s]) -> %u", size, str,
		      *bytes_written);
	free(str);
	return ret;
}

static int nb_buf_remaining(void)
{
	return BUF_LEN - info.bufidx;
}

static void jtagserv_flush_buffer(void)
{
	unsigned int retlen;
	int nb = info.bufidx, ret = ERROR_OK;

	while (ret == ERROR_OK && nb > 0) {
		ret = jtagserv_buf_write(info.buf, nb, &retlen);
		nb -= retlen;
	}
	info.bufidx = 0;
}

/*
 * Actually, the JTAGSERV offers a byte-shift mode to transmit up to 504 data
 * bits (bidirectional) in a single USB packet. A header byte has to be sent as
 * the first byte in a packet with the following meaning:
 *
 *   Bit 7 (0x80): Must be set to indicate byte-shift mode.
 *   Bit 6 (0x40): If set, the JTAGSERV will also read data, not just write.
 *   Bit 5..0:     Define the number N of following bytes
 *
 * All N following bytes will then be clocked out serially on TDI. If Bit 6 was
 * set, it will afterwards return N bytes with TDO data read while clocking out
 * the TDI data. LSB of the first byte after the header byte will appear first
 * on TDI.
 */

/* Simple bit banging mode:
 *
 *   Bit 7 (0x80): Must be zero (see byte-shift mode above)
 *   Bit 6 (0x40): If set, you will receive a byte indicating the state of TDO
 *                 in return.
 *   Bit 5 (0x20): Output Enable/LED.
 *   Bit 4 (0x10): TDI Output.
 *   Bit 3 (0x08): nCS Output (not used in JTAG mode).
 *   Bit 2 (0x04): nCE Output (not used in JTAG mode).
 *   Bit 1 (0x02): TMS Output.
 *   Bit 0 (0x01): TCK Output.
 *
 * For transmitting a single data bit, you need to write two bytes (one for
 * setting up TDI/TMS/TCK=0, and one to trigger TCK high with same TDI/TMS
 * held). Up to 64 bytes can be combined in a single USB packet.
 * It isn't possible to read a data without transmitting data.
 */

#define TCK		(1 << 0)
#define TMS		(1 << 1)
#define NCE		(1 << 2)
#define NCS		(1 << 3)
#define TDI		(1 << 4)
#define LED		(1 << 5)
#define READ		(1 << 6)
#define SHMODE		(1 << 7)
#define READ_TDO	(1 << 0)

/**
 * jtagserv_queue_byte - queue one 'bitbang mode' byte for USB Blaster
 * @abyte: the byte to queue
 *
 * Queues one byte in 'bitbang mode' to the USB Blaster. The byte is not
 * actually sent, but stored in a buffer. The write is performed once
 * the buffer is filled, or if an explicit jtagserv_flush_buffer() is called.
 */
static void jtagserv_queue_byte(uint8_t abyte)
{
	if (nb_buf_remaining() < 1)
		jtagserv_flush_buffer();
	info.buf[info.bufidx++] = abyte;
	if (nb_buf_remaining() == 0)
		jtagserv_flush_buffer();
	LOG_DEBUG_IO("(byte=0x%02x)", abyte);
}

/**
 * jtagserv_compute_pin - compute if gpio should be asserted
 * @steer: control (ie. TRST driven, SRST driven, of fixed)
 *
 * Returns pin value (1 means driven high, 0 mean driven low)
 */
bool jtagserv_compute_pin(enum gpio_steer steer)
{
	switch (steer) {
	case FIXED_0:
		return 0;
	case FIXED_1:
		return 1;
	case SRST:
		return !info.srst_asserted;
	case TRST:
		return !info.trst_asserted;
	default:
		return 1;
	}
}

/**
 * jtagserv_build_out - build bitbang mode output byte
 * @type: says if reading back TDO is required
 *
 * Returns the compute bitbang mode byte
 */
static uint8_t jtagserv_build_out(enum scan_type type)
{
	uint8_t abyte = 0;

	abyte |= info.tms ? TMS : 0;
	abyte |= jtagserv_compute_pin(info.pin6) ? NCE : 0;
	abyte |= jtagserv_compute_pin(info.pin8) ? NCS : 0;
	abyte |= info.tdi ? TDI : 0;
	abyte |= LED;
	if (type == SCAN_IN || type == SCAN_IO)
		abyte |= READ;
	return abyte;
}

/**
 * jtagserv_reset - reset the JTAG device is possible
 * @trst: 1 if TRST is to be asserted
 * @srst: 1 if SRST is to be asserted
 */
static void jtagserv_reset(int trst, int srst)
{
	uint8_t out_value;

	info.trst_asserted = trst;
	info.srst_asserted = srst;
	out_value = jtagserv_build_out(SCAN_OUT);
	jtagserv_queue_byte(out_value);
	jtagserv_flush_buffer();
}

/**
 * jtagserv_clock_tms - clock a TMS transition
 * @tms: the TMS to be sent
 *
 * Triggers a TMS transition (ie. one JTAG TAP state move).
 */
static void jtagserv_clock_tms(int tms)
{
	uint8_t out;

	LOG_DEBUG_IO("(tms=%d)", !!tms);
	info.tms = !!tms;
	info.tdi = 0;
	out = jtagserv_build_out(SCAN_OUT);
	jtagserv_queue_byte(out);
	jtagserv_queue_byte(out | TCK);
}

/**
 * jtagserv_idle_clock - put back TCK to low level
 *
 * See jtagserv_queue_tdi() comment for the usage of this function.
 */
static void jtagserv_idle_clock(void)
{
	uint8_t out = jtagserv_build_out(SCAN_OUT);

	LOG_DEBUG_IO(".");
	jtagserv_queue_byte(out);
}

/**
 * jtagserv_clock_tdi - Output a TDI with bitbang mode
 * @tdi: the TDI bit to be shifted out
 * @type: scan type (ie. does a readback of TDO is required)
 *
 * Output a TDI bit and assert clock to push it into the JTAG device :
 *  - writing out TCK=0, TMS=<old_state>=0, TDI=<tdi>
 * - writing out TCK=1, TMS=<new_state>, TDI=<tdi> which triggers the JTAG
 *    device aquiring the data.
 *
 * If a TDO is to be read back, the required read is requested (bitbang mode),
 * and the USB Blaster will send back a byte with bit0 reprensenting the TDO.
 */
static void jtagserv_clock_tdi(int tdi, enum scan_type type)
{
	uint8_t out;

	LOG_DEBUG_IO("(tdi=%d)",  !!tdi);
	info.tdi = !!tdi;

	out = jtagserv_build_out(SCAN_OUT);
	jtagserv_queue_byte(out);

	out = jtagserv_build_out(type);
	jtagserv_queue_byte(out | TCK);
}

/**
 * jtagserv_clock_tdi_flip_tms - Output a TDI with bitbang mode, change JTAG state
 * @tdi: the TDI bit to be shifted out
 * @type: scan type (ie. does a readback of TDO is required)
 *
 * This function is the same as jtagserv_clock_tdi(), but it changes also the TMS
 * while outputing the TDI. This should be the last TDI output of a TDI
 * sequence, which will change state from :
 *   - IRSHIFT -> IREXIT1
 *   - or DRSHIFT -> DREXIT1
 */
static void jtagserv_clock_tdi_flip_tms(int tdi, enum scan_type type)
{
	uint8_t out;

	LOG_DEBUG_IO("(tdi=%d)", !!tdi);
	info.tdi = !!tdi;
	info.tms = !info.tms;

	out = jtagserv_build_out(SCAN_OUT);
	jtagserv_queue_byte(out);

	out = jtagserv_build_out(type);
	jtagserv_queue_byte(out | TCK);

	out = jtagserv_build_out(SCAN_OUT);
	jtagserv_queue_byte(out);
}

/**
 * jtagserv_queue_bytes - queue bytes for the USB Blaster
 * @bytes: byte array
 * @nb_bytes: number of bytes
 *
 * Queues bytes to be sent to the USB Blaster. The bytes are not
 * actually sent, but stored in a buffer. The write is performed once
 * the buffer is filled, or if an explicit jtagserv_flush_buffer() is called.
 */
static void jtagserv_queue_bytes(uint8_t *bytes, int nb_bytes)
{
	if (info.bufidx + nb_bytes > BUF_LEN) {
		LOG_ERROR("buggy code, should never queue more that %d bytes",
			  info.bufidx + nb_bytes);
		exit(-1);
	}
	LOG_DEBUG_IO("(nb_bytes=%d, bytes=[0x%02x, ...])", nb_bytes,
		      bytes ? bytes[0] : 0);
	if (bytes)
		memcpy(&info.buf[info.bufidx], bytes, nb_bytes);
	else
		memset(&info.buf[info.bufidx], 0, nb_bytes);
	info.bufidx += nb_bytes;
	if (nb_buf_remaining() == 0)
		jtagserv_flush_buffer();
}

/**
 * jtagserv_tms_seq - write a TMS sequence transition to JTAG
 * @bits: TMS bits to be written (bit0, bit1 .. bitN)
 * @nb_bits: number of TMS bits (between 1 and 8)
 * @skip: number of TMS bits to skip at the beginning of the series
 *
 * Write a serie of TMS transitions, where each transition consists in :
 *  - writing out TCK=0, TMS=<new_state>, TDI=<???>
 *  - writing out TCK=1, TMS=<new_state>, TDI=<???> which triggers the transition
 * The function ensures that at the end of the sequence, the clock (TCK) is put
 * low.
 */
static void jtagserv_tms_seq(const uint8_t *bits, int nb_bits, int skip)
{
	int i;

	LOG_DEBUG_IO("(bits=%02x..., nb_bits=%d)", bits[0], nb_bits);
	for (i = skip; i < nb_bits; i++)
		jtagserv_clock_tms((bits[i / 8] >> (i % 8)) & 0x01);
	jtagserv_idle_clock();
}

/**
 * jtagserv_tms - write a tms command
 * @cmd: tms command
 */
static void jtagserv_tms(struct tms_command *cmd)
{
	LOG_DEBUG_IO("(num_bits=%d)", cmd->num_bits);
	jtagserv_tms_seq(cmd->bits, cmd->num_bits, 0);
}

/**
 * jtagserv_path_move - write a TMS sequence transition to JTAG
 * @cmd: path transition
 *
 * Write a serie of TMS transitions, where each transition consists in :
 *  - writing out TCK=0, TMS=<new_state>, TDI=<???>
 *  - writing out TCK=1, TMS=<new_state>, TDI=<???> which triggers the transition
 * The function ensures that at the end of the sequence, the clock (TCK) is put
 * low.
 */
static void jtagserv_path_move(struct pathmove_command *cmd)
{
	int i;

	LOG_DEBUG_IO("(num_states=%d, last_state=%d)",
		  cmd->num_states, cmd->path[cmd->num_states - 1]);
	for (i = 0; i < cmd->num_states; i++) {
		if (tap_state_transition(tap_get_state(), false) == cmd->path[i])
			jtagserv_clock_tms(0);
		if (tap_state_transition(tap_get_state(), true) == cmd->path[i])
			jtagserv_clock_tms(1);
		tap_set_state(cmd->path[i]);
	}
	jtagserv_idle_clock();
}

/**
 * jtagserv_state_move - move JTAG state to the target state
 * @state: the target state
 * @skip: number of bits to skip at the beginning of the path
 *
 * Input the correct TMS sequence to the JTAG TAP so that we end up in the
 * target state. This assumes the current state (tap_get_state()) is correct.
 */
static void jtagserv_state_move(tap_state_t state, int skip)
{
	uint8_t tms_scan;
	int tms_len;

	LOG_DEBUG_IO("(from %s to %s)", tap_state_name(tap_get_state()),
		  tap_state_name(state));
	if (tap_get_state() == state)
		return;
	tms_scan = tap_get_tms_path(tap_get_state(), state);
	tms_len = tap_get_tms_path_len(tap_get_state(), state);
	jtagserv_tms_seq(&tms_scan, tms_len, skip);
	tap_set_state(state);
}

/**
 * jtagserv_read_byteshifted_tdos - read TDO of byteshift writes
 * @buf: the buffer to store the bits
 * @nb_bits: the number of bits
 *
 * Reads back from USB Blaster TDO bits, triggered by a 'byteshift write', ie. eight
 * bits per received byte from USB interface, and store them in buffer.
 *
 * As the USB blaster stores the TDO bits in LSB (ie. first bit in (byte0,
 * bit0), second bit in (byte0, bit1), ...), which is what we want to return,
 * simply read bytes from USB interface and store them.
 *
 * Returns ERROR_OK if OK, ERROR_xxx if a read error occured
 */
static int jtagserv_read_byteshifted_tdos(uint8_t *buf, int nb_bytes)
{
	unsigned int retlen;
	int ret = ERROR_OK;

	LOG_DEBUG_IO("%s(buf=%p, num_bits=%d)", __func__, buf, nb_bytes * 8);
	jtagserv_flush_buffer();
	while (ret == ERROR_OK && nb_bytes > 0) {
		ret = jtagserv_buf_read(buf, nb_bytes, &retlen);
		nb_bytes -= retlen;
	}
	return ret;
}

/**
 * jtagserv_read_bitbang_tdos - read TDO of bitbang writes
 * @buf: the buffer to store the bits
 * @nb_bits: the number of bits
 *
 * Reads back from USB Blaster TDO bits, triggered by a 'bitbang write', ie. one
 * bit per received byte from USB interface, and store them in buffer, where :
 *  - first bit is stored in byte0, bit0 (LSB)
 *  - second bit is stored in byte0, bit 1
 *  ...
 *  - eight bit is sotred in byte0, bit 7
 *  - ninth bit is sotred in byte1, bit 0
 *  - etc ...
 *
 * Returns ERROR_OK if OK, ERROR_xxx if a read error occured
 */
static int jtagserv_read_bitbang_tdos(uint8_t *buf, int nb_bits)
{
	int nb1 = nb_bits;
	int i, ret = ERROR_OK;
	unsigned int retlen;
	uint8_t tmp[8];

	LOG_DEBUG_IO("%s(buf=%p, num_bits=%d)", __func__, buf, nb_bits);

	/*
	 * Ensure all previous bitbang writes were issued to the dongle, so that
	 * it returns back the read values.
	 */
	jtagserv_flush_buffer();

	ret = jtagserv_buf_read(tmp, nb1, &retlen);
	for (i = 0; ret == ERROR_OK && i < nb1; i++)
		if (tmp[i] & READ_TDO)
			*buf |= (1 << i);
		else
			*buf &= ~(1 << i);
	return ret;
}

/**
 * jtagserv_queue_tdi - short description
 * @bits: bits to be queued on TDI (or NULL if 0 are to be queued)
 * @nb_bits: number of bits
 * @scan: scan type (ie. if TDO read back is required or not)
 *
 * Outputs a serie of TDI bits on TDI.
 * As a side effect, the last TDI bit is sent along a TMS=1, and triggers a JTAG
 * TAP state shift if input bits were non NULL.
 *
 * In order to not saturate the USB Blaster queues, this method reads back TDO
 * if the scan type requests it, and stores them back in bits.
 *
 * As a side note, the state of TCK when entering this function *must* be
 * low. This is because byteshift mode outputs TDI on rising TCK and reads TDO
 * on falling TCK if and only if TCK is low before queuing byteshift mode bytes.
 * If TCK was high, the USB blaster will queue TDI on falling edge, and read TDO
 * on rising edge !!!
 */
static void jtagserv_queue_tdi(uint8_t *bits, int nb_bits, enum scan_type scan)
{
	int nb8 = nb_bits / 8;
	int nb1 = nb_bits % 8;
	int nbfree_in_packet, i, trans = 0, read_tdos;
	uint8_t *tdos = calloc(1, nb_bits / 8 + 1);
	static uint8_t byte0[BUF_LEN];

	/*
	 * As the last TDI bit should always be output in bitbang mode in order
	 * to activate the TMS=1 transition to EXIT_?R state. Therefore a
	 * situation where nb_bits is a multiple of 8 is handled as follows:
	 * - the number of TDI shifted out in "byteshift mode" is 8 less than
	 *   nb_bits
	 * - nb1 = 8
	 * This ensures that nb1 is never 0, and allows the TMS transition.
	 */
	if (nb8 > 0 && nb1 == 0) {
		nb8--;
		nb1 = 8;
	}

	read_tdos = (scan == SCAN_IN || scan == SCAN_IO);
	for (i = 0; i < nb8; i += trans) {
		/*
		 * Calculate number of bytes to fill USB packet of size MAX_PACKET_SIZE
		 */
		nbfree_in_packet = (MAX_PACKET_SIZE - (info.bufidx%MAX_PACKET_SIZE));
		trans = MIN(nbfree_in_packet - 1, nb8 - i);

		/*
		 * Queue a byte-shift mode transmission, with as many bytes as
		 * is possible with regard to :
		 *  - current filling level of write buffer
		 *  - remaining bytes to write in byte-shift mode
		 */
		if (read_tdos)
			jtagserv_queue_byte(SHMODE | READ | trans);
		else
			jtagserv_queue_byte(SHMODE | trans);
		if (bits)
			jtagserv_queue_bytes(&bits[i], trans);
		else
			jtagserv_queue_bytes(byte0, trans);
		if (read_tdos) {
			if (info.flags & COPY_TDO_BUFFER)
				jtagserv_queue_byte(CMD_COPY_TDO_BUFFER);
			jtagserv_read_byteshifted_tdos(&tdos[i], trans);
		}
	}

	/*
	 * Queue the remaining TDI bits in bitbang mode.
	 */
	for (i = 0; i < nb1; i++) {
		int tdi = bits ? bits[nb8 + i / 8] & (1 << i) : 0;
		if (bits && i == nb1 - 1)
			jtagserv_clock_tdi_flip_tms(tdi, scan);
		else
			jtagserv_clock_tdi(tdi, scan);
	}
	if (nb1 && read_tdos) {
		if (info.flags & COPY_TDO_BUFFER)
			jtagserv_queue_byte(CMD_COPY_TDO_BUFFER);
		jtagserv_read_bitbang_tdos(&tdos[nb8], nb1);
	}

	if (bits)
		memcpy(bits, tdos, DIV_ROUND_UP(nb_bits, 8));
	free(tdos);

	/*
	 * Ensure clock is in lower state
	 */
	jtagserv_idle_clock();
}

static void jtagserv_runtest(int cycles, tap_state_t state)
{
	LOG_DEBUG_IO("%s(cycles=%i, end_state=%d)", __func__, cycles, state);

	jtagserv_state_move(TAP_IDLE, 0);
	jtagserv_queue_tdi(NULL, cycles, SCAN_OUT);
	jtagserv_state_move(state, 0);
}

static void jtagserv_stableclocks(int cycles)
{
	LOG_DEBUG_IO("%s(cycles=%i)", __func__, cycles);
	jtagserv_queue_tdi(NULL, cycles, SCAN_OUT);
}

/**
 * jtagserv_scan - launches a DR-scan or IR-scan
 * @cmd: the command to launch
 *
 * Launch a JTAG IR-scan or DR-scan
 *
 * Returns ERROR_OK if OK, ERROR_xxx if a read/write error occured.
 */
static int jtagserv_scan(struct scan_command *cmd)
{
	int scan_bits;
	uint8_t *buf = NULL;
	enum scan_type type;
	int ret = ERROR_OK;
	static const char * const type2str[] = { "", "SCAN_IN", "SCAN_OUT", "SCAN_IO" };
	char *log_buf = NULL;

	type = jtag_scan_type(cmd);
	scan_bits = jtag_build_buffer(cmd, &buf);

	if (cmd->ir_scan)
		jtagserv_state_move(TAP_IRSHIFT, 0);
	else
		jtagserv_state_move(TAP_DRSHIFT, 0);

	log_buf = hexdump(buf, DIV_ROUND_UP(scan_bits, 8));
	LOG_DEBUG_IO("%s(scan=%s, type=%s, bits=%d, buf=[%s], end_state=%d)", __func__,
		  cmd->ir_scan ? "IRSCAN" : "DRSCAN",
		  type2str[type],
		  scan_bits, log_buf, cmd->end_state);
	free(log_buf);

	jtagserv_queue_tdi(buf, scan_bits, type);

	ret = jtag_read_buffer(buf, cmd);
	if (buf)
		free(buf);
	/*
	 * jtagserv_queue_tdi sends the last bit with TMS=1. We are therefore
	 * already in Exit1-DR/IR and have to skip the first step on our way
	 * to end_state.
	 */
	jtagserv_state_move(cmd->end_state, 1);
	return ret;
}

static void jtagserv_usleep(int us)
{
	LOG_DEBUG_IO("%s(us=%d)",  __func__, us);
	jtag_sleep(us);
}

static void jtagserv_initial_wipeout(void)
{
	static uint8_t tms_reset = 0xff;
	uint8_t out_value;
	uint32_t retlen;
	int i;

	out_value = jtagserv_build_out(SCAN_OUT);
	for (i = 0; i < BUF_LEN; i++)
		info.buf[i] = out_value | ((i % 2) ? TCK : 0);

	/*
	 * Flush JTAGSERV queue fifos
	 *  - empty the write FIFO (128 bytes)
	 *  - empty the read FIFO (384 bytes)
	 */
	jtagserv_buf_write(info.buf, BUF_LEN, &retlen);
	/*
	 * Put JTAG in RESET state (five 1 on TMS)
	 */
	jtagserv_tms_seq(&tms_reset, 5, 0);
	tap_set_state(TAP_RESET);
}

static int jtagserv_execute_queue(void)
{
	struct jtag_command *cmd;
	static int first_call = 1;
	int ret = ERROR_OK;

	if (first_call) {
		first_call--;
		jtagserv_initial_wipeout();
	}

	for (cmd = jtag_command_queue; ret == ERROR_OK && cmd != NULL;
	     cmd = cmd->next) {
		switch (cmd->type) {
		case JTAG_RESET:
			jtagserv_reset(cmd->cmd.reset->trst, cmd->cmd.reset->srst);
			break;
		case JTAG_RUNTEST:
			jtagserv_runtest(cmd->cmd.runtest->num_cycles,
				       cmd->cmd.runtest->end_state);
			break;
		case JTAG_STABLECLOCKS:
			jtagserv_stableclocks(cmd->cmd.stableclocks->num_cycles);
			break;
		case JTAG_TLR_RESET:
			jtagserv_state_move(cmd->cmd.statemove->end_state, 0);
			break;
		case JTAG_PATHMOVE:
			jtagserv_path_move(cmd->cmd.pathmove);
			break;
		case JTAG_TMS:
			jtagserv_tms(cmd->cmd.tms);
			break;
		case JTAG_SLEEP:
			jtagserv_usleep(cmd->cmd.sleep->us);
			break;
		case JTAG_SCAN:
			ret = jtagserv_scan(cmd->cmd.scan);
			break;
		default:
			LOG_ERROR("BUG: unknown JTAG command type 0x%X",
				  cmd->type);
			ret = ERROR_FAIL;
			break;
		}
	}

	jtagserv_flush_buffer();
	return ret;
}

/**
 * jtagserv_init - Initialize the Altera device
 *
 * Initialize the device :
 *  - open the USB device
 *  - pretend it's initialized while actual init is delayed until first jtag command
 *
 * Returns ERROR_OK if USB device found, error if not.
 */
static int jtagserv_init(void)
{
	int ret, i;

	for (i = 0; lowlevel_drivers_map[i].name; i++) {
		if (info.lowlevel_name) {
			if (!strcmp(lowlevel_drivers_map[i].name, info.lowlevel_name)) {
				info.drv = lowlevel_drivers_map[i].drv_register();
				if (!info.drv) {
					LOG_ERROR("Error registering lowlevel driver \"%s\"",
						  info.lowlevel_name);
					return ERROR_JTAG_DEVICE_ERROR;
				}
				break;
			}
		} else {
			info.drv = lowlevel_drivers_map[i].drv_register();
			if (info.drv) {
				info.lowlevel_name = strdup(lowlevel_drivers_map[i].name);
				LOG_INFO("No lowlevel driver configured, using %s", info.lowlevel_name);
				break;
			}
		}
	}

	if (!info.drv) {
		LOG_ERROR("No lowlevel driver available");
		return ERROR_JTAG_DEVICE_ERROR;
	}

	/*
	 * Register the lowlevel driver
	 */
	info.drv->jtagserv_vid = info.jtagserv_vid;
	info.drv->jtagserv_pid = info.jtagserv_pid;
	info.drv->jtagserv_vid_uninit = info.jtagserv_vid_uninit;
	info.drv->jtagserv_pid_uninit = info.jtagserv_pid_uninit;
	info.drv->jtagserv_device_desc = info.jtagserv_device_desc;
	info.drv->firmware_path = info.firmware_path;

	info.flags |= info.drv->flags;

	ret = info.drv->open(info.drv);

	/*
	 * Let lie here : the TAP is in an unknown state, but the first
	 * execute_queue() will trigger a jtagserv_initial_wipeout(), which will
	 * put the TAP in RESET.
	 */
	tap_set_state(TAP_RESET);
	return ret;
}

/**
 * jtagserv_quit - Release the Altera device
 *
 * Releases the device :
 *   - put the device pins in 'high impedance' mode
 *   - close the USB device
 *
 * Returns always ERROR_OK
 */
static int jtagserv_quit(void)
{
	uint8_t byte0 = 0;
	unsigned int retlen;

	jtagserv_buf_write(&byte0, 1, &retlen);
	return info.drv->close(info.drv);
}

COMMAND_HANDLER(jtagserv_handle_device_desc_command)
{
	if (CMD_ARGC != 1)
		return ERROR_COMMAND_SYNTAX_ERROR;

	info.jtagserv_device_desc = strdup(CMD_ARGV[0]);

	return ERROR_OK;
}

COMMAND_HANDLER(jtagserv_handle_vid_pid_command)
{
	if (CMD_ARGC > 4) {
		LOG_WARNING("ignoring extra IDs in jtagserv_vid_pid "
					"(maximum is 2 pairs)");
		CMD_ARGC = 4;
	}

	if (CMD_ARGC >= 2) {
		COMMAND_PARSE_NUMBER(u16, CMD_ARGV[0], info.jtagserv_vid);
		COMMAND_PARSE_NUMBER(u16, CMD_ARGV[1], info.jtagserv_pid);
	} else {
		LOG_WARNING("incomplete jtagserv_vid_pid configuration");
	}

	if (CMD_ARGC == 4) {
		COMMAND_PARSE_NUMBER(u16, CMD_ARGV[2], info.jtagserv_vid_uninit);
		COMMAND_PARSE_NUMBER(u16, CMD_ARGV[3], info.jtagserv_pid_uninit);
	} else {
		LOG_WARNING("incomplete jtagserv_vid_pid configuration");
	}

	return ERROR_OK;
}

COMMAND_HANDLER(jtagserv_handle_pin_command)
{
	uint8_t out_value;
	const char * const pin_name = CMD_ARGV[0];
	enum gpio_steer *steer = NULL;
	static const char * const pin_val_str[] = {
		[FIXED_0] = "0",
		[FIXED_1] = "1",
		[SRST] = "SRST driven",
		[TRST] = "TRST driven",
	};

	if (CMD_ARGC > 2) {
		LOG_ERROR("%s takes exactly one or two arguments", CMD_NAME);
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (!strcmp(pin_name, "pin6"))
		steer = &info.pin6;
	if (!strcmp(pin_name, "pin8"))
		steer = &info.pin8;
	if (!steer) {
		LOG_ERROR("%s: pin name must be \"pin6\" or \"pin8\"",
			  CMD_NAME);
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (CMD_ARGC == 1) {
		LOG_INFO("%s: %s is set as %s\n", CMD_NAME, pin_name,
			 pin_val_str[*steer]);
	}

	if (CMD_ARGC == 2) {
		const char * const pin_value = CMD_ARGV[1];
		char val = pin_value[0];

		if (strlen(pin_value) > 1)
			val = '?';
		switch (tolower((unsigned char)val)) {
		case '0':
			*steer = FIXED_0;
			break;
		case '1':
			*steer = FIXED_1;
			break;
		case 't':
			*steer = TRST;
			break;
		case 's':
			*steer = SRST;
			break;
		default:
			LOG_ERROR("%s: pin value must be 0, 1, s (SRST) or t (TRST)",
				pin_value);
			return ERROR_COMMAND_SYNTAX_ERROR;
		}

		if (info.drv) {
			out_value = jtagserv_build_out(SCAN_OUT);
			jtagserv_queue_byte(out_value);
			jtagserv_flush_buffer();
		}
	}
	return ERROR_OK;
}

COMMAND_HANDLER(jtagserv_firmware_command)
{
	if (CMD_ARGC != 1)
		return ERROR_COMMAND_SYNTAX_ERROR;

	info.firmware_path = strdup(CMD_ARGV[0]);

	return ERROR_OK;
}


static const struct command_registration jtagserv_command_handlers[] = {
	{
		.name = "jtagserv_device_desc",
		.handler = jtagserv_handle_device_desc_command,
		.mode = COMMAND_CONFIG,
		.help = "set the USB device description of the JTAGSERV",
		.usage = "description-string",
	},
	{
		.name = "jtagserv_vid_pid",
		.handler = jtagserv_handle_vid_pid_command,
		.mode = COMMAND_CONFIG,
		.help = "the vendor ID and product ID of the uninitialized device " \
			"for JTAGSERV",
		.usage = "vid pid vid_uninit pid_uninit",
	},
	{
		.name = "jtagserv_pin",
		.handler = jtagserv_handle_pin_command,
		.mode = COMMAND_ANY,
		.help = "show or set pin state for the unused GPIO pins",
		.usage = "(pin6|pin8) (0|1|s|t)",
	},
		{
		.name = "jtagserv_firmware",
		.handler = &jtagserv_firmware_command,
		.mode = COMMAND_CONFIG,
		.help = "configure the JTAGSERV firmware location",
		.usage = "path/to/blaster_xxxx.hex",
	},
	COMMAND_REGISTRATION_DONE
};

static struct jtag_interface jtagserv_interface = {
	.supported = DEBUG_CAP_TMS_SEQ,
	.execute_queue = jtagserv_execute_queue,
};

struct adapter_driver jtagserv_adapter_driver = {
	.name = "jtagserv",
	.transports = jtag_only,
	.commands = jtagserv_command_handlers,

	.init = jtagserv_init,
	.quit = jtagserv_quit,

	.jtag_ops = &jtagserv_interface,
};
