/***************************************************************************
 *   Copyright (C) 2007-2008 by Øyvind Harboe                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <jtag/jtag.h>
#include <target/embeddedice.h>
#include <jtag/minidriver.h>
#include <jtag/interface.h>
#include <helper/jep106.h>

#include "log.h"

#include "c_jtag_client_gnuaji.h"
#include "jtagservice.h"

extern void jtag_tap_add(struct jtag_tap *t);


#define IDCODE_SOCVHPS (0x4BA00477)


static struct jtagservice_record jtagservice  = {
    .hardware_count = 0,
//    .hardware_list = NULL,
//    .server_version_info_list = NULL,
//    .in_use_hardware = NULL,
    .in_use_hardware_index = 0,
    .in_use_hardware_chain_pid = 0,
    
    .device_count = 0,
//    .device_list = NULL,

    .in_use_device_tap_position = 0,
//    .in_use_device = NULL,
    .in_use_device_id = 0,
    .in_use_device_irlen = 0,
    
    .locked    = NONE,
};

//=================================
// Helpers
//=================================

/*
 * Access functions to lowlevel driver, agnostic of libftdi/libftdxx
 */
static char *hexdump(const uint8_t *buf, const unsigned int size)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
	unsigned int i;
	char *str = calloc(size * 2 + 1, 1);

	for (i = 0; i < size; i++)
		sprintf(str + 2*i, "%02x", buf[i]);
	return str;
}

//=================================
// Bits that have to be implemented for ARM
//=================================

/**
 * This is an inner loop of the open loop DCC write of data to ARM target
 * \note It is a direct copy from \see embededdedice.c
 *       \file embededdedice.c specifically requested that minidriver provides its own
 *             implementation
 */
void embeddedice_write_dcc(struct jtag_tap *tap, int reg_addr, const uint8_t *buffer,
		int little, int count)
{
	int i;
	for (i = 0; i < count; i++) {
		embeddedice_write_reg_inner(tap, reg_addr, fast_target_buffer_get_u32(buffer, little));
		buffer += 4;
	}
}

/**
 * This is an inner loop of the open loop DCC write of data to ARM target
 * \note It is a direct copy from \see arm11_dbgtap.c
 *       \file arm11_dbgtap.c specifically requested that minidriver provides its own
 *             implementation or copy the default implementation. We copied the default.
 */
int arm11_run_instr_data_to_core_noack_inner(struct jtag_tap *tap, uint32_t opcode,
		uint32_t *data, size_t count)
{
	int arm11_run_instr_data_to_core_noack_inner_default(struct jtag_tap *tap, \
			uint32_t opcode, uint32_t *data, size_t count);
	return arm11_run_instr_data_to_core_noack_inner_default(tap, opcode, data, count);
}


//=================================
// Bits that have to be implemented/copied for src/jtag/core.c
//=================================


/* copied from src/jtag/core.c */
#define JTAG_MAX_AUTO_TAPS 20

/* copied from src/jtag/core.c */
#define EXTRACT_JEP106_BANK(X) (((X) & 0xf00) >> 8)
#define EXTRACT_JEP106_ID(X)   (((X) & 0xfe) >> 1)
#define EXTRACT_MFG(X)  (((X) & 0xffe) >> 1)
#define EXTRACT_PART(X) (((X) & 0xffff000) >> 12)
#define EXTRACT_VER(X)  (((X) & 0xf0000000) >> 28)

/* copied from src/jtag/core.c */
static void jtag_examine_chain_display(enum log_levels level, const char *msg,
	const char *name, uint32_t idcode)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
	log_printf_lf(level, __FILE__, __LINE__, __func__,
		"JTAG tap: %s %16.16s: 0x%08x "
		"(mfg: 0x%3.3x (%s), part: 0x%4.4x, ver: 0x%1.1x)",
		name, msg,
		(unsigned int)idcode,
		(unsigned int)EXTRACT_MFG(idcode),
		jep106_manufacturer(EXTRACT_JEP106_BANK(idcode), EXTRACT_JEP106_ID(idcode)),
		(unsigned int)EXTRACT_PART(idcode),
		(unsigned int)EXTRACT_VER(idcode));
}

/* copied from src/jtag/core.c */
static bool jtag_examine_chain_match_tap(const struct jtag_tap *tap)
{    LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);


	if (tap->expected_ids_cnt == 0 || !tap->hasidcode)
		return true;

	/* optionally ignore the JTAG version field - bits 28-31 of IDCODE */
	uint32_t mask = tap->ignore_version ? ~(0xfU << 28) : ~0U;
	uint32_t idcode = tap->idcode & mask;

	/* Loop over the expected identification codes and test for a match */
	for (unsigned ii = 0; ii < tap->expected_ids_cnt; ii++) {
		uint32_t expected = tap->expected_ids[ii] & mask;

		if (idcode == expected)
			return true;

		/* treat "-expected-id 0" as a "don't-warn" wildcard */
		if (0 == tap->expected_ids[ii])
			return true;
	}

	/* If none of the expected ids matched, warn */
	jtag_examine_chain_display(LOG_LVL_WARNING, "UNEXPECTED",
		tap->dotted_name, tap->idcode);
	for (unsigned ii = 0; ii < tap->expected_ids_cnt; ii++) {
		char msg[32];

		snprintf(msg, sizeof(msg), "expected %u of %u", ii + 1, tap->expected_ids_cnt);
		jtag_examine_chain_display(LOG_LVL_ERROR, msg,
			tap->dotted_name, tap->expected_ids[ii]);
	}
	return false;
}

/**
 * @pre jtagservice is filled with device inforamtion. That should
 *      already been done during adapter initialization via 
 *      @c minijtagserv_init() function.
 */
int jtag_examine_chain(void)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    //AJI_ERROR status = AJI_NO_ERROR;
    int retval = ERROR_OK;
    
    struct jtag_tap *tap = jtag_tap_next_enabled(NULL);
    unsigned num_taps = jtagservice.device_count;
    unsigned autocount = 0;
    unsigned t = 0;
    for(t = 0, tap = jtag_tap_next_enabled(NULL); 
        t<num_taps; 
        ++t, tap = jtag_tap_next_enabled(tap)) {
        AJI_DEVICE device = jtagservice.device_list[t];

        if(tap == NULL) {
        	tap = calloc(1, sizeof *tap);
			if (!tap) { 
				return ERROR_FAIL;
			}

			tap->chip = alloc_printf("auto%u", autocount++);
			tap->tapname = strdup("tap");
			tap->dotted_name = alloc_printf("%s.%s", tap->chip, tap->tapname);

			tap->hasidcode = true;
			tap->idcode = device.device_id;

			tap->ir_length = device.instruction_length; 
			tap->ir_capture_mask = 0x03;
			tap->ir_capture_value = 0x01;

			tap->enabled = 1;
			jtag_tap_init(tap);
			tap->ir_length *= -1; // set to negative to indicate to jtag_validate_ircapture() that 
		                          // user had not set this ir_length.

			jtag_examine_chain_display(LOG_LVL_INFO, "tap/device found", tap->dotted_name, tap->idcode);
        } 
 		/* ensure the TAP ID matches what was expected */
		if (!jtag_examine_chain_match_tap(tap))
			retval = ERROR_JTAG_INIT_SOFT_FAIL;
    } //end for t
    return retval;
}

/*
 * No need to validate irlen because we are trusting AJI to provide us with
 * the correct value. However, need to produce a message telling
 * the user to add new tap points if the TAP point were autodetected.
 * On exit the scan chain is reset.
 */
int jtag_validate_ircapture(void)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    for (struct jtag_tap *tap = jtag_tap_next_enabled(NULL) ; 
	     tap != NULL; 
	     tap = jtag_tap_next_enabled(tap)
	) {
	    /* Negative ir_length means it was autodetected by AJI and user
	       haven't declare the Taps. 
	       The real ir_length is the positive ir_length value
	     */
		if (tap->ir_length < 0) { 
			tap->ir_length *= -1;
			LOG_WARNING("AUTO %s - use \"jtag newtap " "%s %s -irlen %d "
					"-expected-id 0x%08" PRIx32 "\"",
					tap->dotted_name, tap->chip, tap->tapname, tap->ir_length, tap->idcode);
		}
	}
	jtag_add_tlr();
	jtag_execute_queue();
	return ERROR_OK;
}
//=================================
// Callbacks
//=================================

struct jtag_callback_entry {
	struct jtag_callback_entry *next;

	jtag_callback_t callback;
	jtag_callback_data_t data0;
	jtag_callback_data_t data1;
	jtag_callback_data_t data2;
	jtag_callback_data_t data3;
};

static struct jtag_callback_entry *jtag_callback_queue_head;
static struct jtag_callback_entry *jtag_callback_queue_tail;

static void jtag_callback_queue_reset(void)
{
	jtag_callback_queue_head = NULL;
	jtag_callback_queue_tail = NULL;
}

/* add callback to end of queue */
void interface_jtag_add_callback4(jtag_callback_t callback,
		jtag_callback_data_t data0, jtag_callback_data_t data1,
		jtag_callback_data_t data2, jtag_callback_data_t data3)
{
	struct jtag_callback_entry *entry = cmd_queue_alloc(sizeof(struct jtag_callback_entry));

	entry->next = NULL;
	entry->callback = callback;
	entry->data0 = data0;
	entry->data1 = data1;
	entry->data2 = data2;
	entry->data3 = data3;

	if (jtag_callback_queue_head == NULL) {
		jtag_callback_queue_head = entry;
		jtag_callback_queue_tail = entry;
	} else {
		jtag_callback_queue_tail->next = entry;
		jtag_callback_queue_tail = entry;
	}
}


static int jtag_convert_to_callback4(jtag_callback_data_t data0,
		jtag_callback_data_t data1, jtag_callback_data_t data2, jtag_callback_data_t data3)
{
	((jtag_callback1_t)data1)(data0);
	return ERROR_OK;
}

void interface_jtag_add_callback(jtag_callback1_t callback, jtag_callback_data_t data0)
{
	jtag_add_callback4(jtag_convert_to_callback4, data0, (jtag_callback_data_t)callback, 0, 0);
}

void jtag_add_callback(jtag_callback1_t f, jtag_callback_data_t data0)
{
	interface_jtag_add_callback(f, data0);
}

void jtag_add_callback4(jtag_callback_t f, jtag_callback_data_t data0,
		jtag_callback_data_t data1, jtag_callback_data_t data2,
		jtag_callback_data_t data3)
{
	interface_jtag_add_callback4(f, data0, data1, data2, data3);
}


//=================================
// driver
//=================================

/**
 * Find the hardware cable from the jtag server
 * @pos Set jtagservice chain detail if return successful.
 */
static AJI_ERROR jtagserv_select_cable(void)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    AJI_ERROR status = AJI_NO_ERROR;

    LOG_INFO("Querying JTAG Server ...");
 
    status = c_aji_get_hardware2( 
        &(jtagservice.hardware_count), 
        jtagservice.hardware_list, 
        jtagservice.server_version_info_list, 
        JTAGSERVICE_TIMEOUT_MS
    );
    if(AJI_TOO_MANY_DEVICES == status) {
        jtagservice.hardware_list = 
            calloc(jtagservice.hardware_count, sizeof(AJI_HARDWARE));
        jtagservice.server_version_info_list = 
            calloc(jtagservice.hardware_count, sizeof(char*));
        if (   jtagservice.hardware_list == NULL 
            || jtagservice.server_version_info_list == NULL
        ) {
            return AJI_NO_MEMORY;
        }
        status = c_aji_get_hardware2( 
            &(jtagservice.hardware_count), 
            jtagservice.hardware_list, 
            jtagservice.server_version_info_list, 
            JTAGSERVICE_TIMEOUT_MS
        );
    } //end if (AJI_TOO_MANY_DEVICES)
    
    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failed to query server for hardware cable information. "
                  " Return Status is %i\n", status
        );
        return status;
    }
    if(0 == jtagservice.hardware_count) {
        LOG_ERROR("JTAG server reports that it has no hardware cable\n");
        return AJI_BAD_HARDWARE;
    }
    LOG_INFO("At present, only the first hardware cable will be used"
             " [%d cable(s) detected]", 
             jtagservice.hardware_count
    );

    jtagservice.in_use_hardware = &(jtagservice.hardware_list[0]);
    jtagservice.in_use_hardware_chain_pid = (jtagservice.in_use_hardware)->persistent_id;
    if(LOG_LEVEL_IS(LOG_LVL_INFO)) {
        AJI_HARDWARE hw = *(jtagservice.in_use_hardware);
        LOG_INFO("Cable %u: device_name=%s, hw_name=%s, server=%s, port=%s,"
                  " chain_id=%p, persistent_id=%d, chain_type=%d, features=%d,"
                  " server_version_info=%s\n", 
              1, hw.device_name, hw.hw_name, hw.server, hw.port,  
              hw.chain_id, hw.persistent_id, hw.chain_type, hw.features,
              jtagservice.server_version_info_list[0]
        );
    }
    return status;
}


/**
 * Select the TAP device to use
 * @pre The chain is already acquired, @see jtagserv_select_cable()
 * @pos jtagservice will be populated with the selected tap
 */
static AJI_ERROR jtagserv_select_tap(void)
{   LOG_DEBUG("***> IN %s(%d): %s %d\n", __FILE__, __LINE__, __FUNCTION__, jtagservice.in_use_hardware_chain_pid);
    AJI_ERROR status = AJI_NO_ERROR;

    AJI_HARDWARE hw;
    status = c_aji_find_hardware(jtagservice.in_use_hardware_chain_pid, &hw, JTAGSERVICE_TIMEOUT_MS);
    if(AJI_NO_ERROR != status){
        status = c_aji_unlock_chain(hw.chain_id); 
        return status;       
    }

    status = jtagservice_lock(&jtagservice, CHAIN, JTAGSERVICE_TIMEOUT_MS);
    if (AJI_NO_ERROR != status) { 
        return status;
    }

    status = c_aji_read_device_chain(
        hw.chain_id, 
        &(jtagservice.device_count), 
        jtagservice.device_list, 
        1
    );
    if(AJI_TOO_MANY_DEVICES == status) {
        jtagservice.device_list = calloc(jtagservice.device_count, sizeof(AJI_DEVICE)); 
        jtagservice.device_open_id_list = calloc(jtagservice.device_count, sizeof(AJI_OPEN_ID)); 
        status = c_aji_read_device_chain(
            hw.chain_id, 
            &(jtagservice.device_count), 
            jtagservice.device_list,
            1
        );
    }    
 
    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failed to query server for TAP information. "
                  " Return Status is %i\n", status
        );
        jtagservice_unlock(&jtagservice, CHAIN, JTAGSERVICE_TIMEOUT_MS);
        return status;
    }

    if(0 == jtagservice.device_count) {
        LOG_ERROR("JTAG server reports that it has no TAP attached to the cable");
        jtagservice_unlock(&jtagservice, CHAIN, JTAGSERVICE_TIMEOUT_MS);
        return AJI_NO_DEVICES;
    }
    LOG_INFO("At present, will not honour OpenOCD target selection and"
             " select the ARM SOCVHPS with IDCODE %X automatically",
             IDCODE_SOCVHPS
    );
    LOG_INFO("Found %d TAP devices", jtagservice.device_count);

    for(DWORD tap_position=0; tap_position<jtagservice.device_count; ++tap_position) {
        AJI_DEVICE device = jtagservice.device_list[tap_position];
        LOG_DEBUG("Detected device (tap_position=%d) device_id=%08X," 
                  " instruction_length=%d, features=%d, device_name=%s", 
                    tap_position+1, 
                    device.device_id, device.instruction_length, 
                    device.features, device.device_name
        );
        
        if( IDCODE_SOCVHPS == device.device_id ) {
            jtagservice.in_use_device_id = device.device_id;
            jtagservice.in_use_device_tap_position = tap_position;
            jtagservice.in_use_device_irlen = device.instruction_length;
            LOG_INFO("Found SOCVHPS device at tap_position %d", tap_position); 
        }
    } //end for tap_position
    
    if(0 == jtagservice.in_use_device_id) {
        LOG_ERROR("No SOCVHPS device found.");
        jtagservice_unlock(&jtagservice, CHAIN, JTAGSERVICE_TIMEOUT_MS);
        return AJI_FAILURE;
    }

    char idcode[19];
    sprintf(idcode, "%X", jtagservice.in_use_device_id);

    AJI_CLAIM2 claims[] = {
        { AJI_CLAIM_IR_SHARED, 0, IR_ARM_IDCODE },
    };
    DWORD claims_n = sizeof(claims) / sizeof(AJI_CLAIM2);

    // Only allowed to open one device at a time.
    // If you don't, then anytime after  c_aji_test_logic_reset() call, 
    //  you can get fatal error
    //  "*** glibc detected *** src/openocd: double free or corruption (fasttop): 0x00000000027bc7a0 ***"        
    status = c_aji_open_device_a(
                      jtagservice.in_use_hardware->chain_id,
                      jtagservice.in_use_device_tap_position,
                      &(jtagservice.device_open_id_list[jtagservice.in_use_device_tap_position]),
                      claims, claims_n, idcode
    );
    if( AJI_NO_ERROR != status) {
            LOG_ERROR("Cannot open device number %d (IDCODE=%s)",
                      jtagservice.in_use_device_tap_position, idcode
            );
    }
    
    jtagservice_unlock(&jtagservice, CHAIN, JTAGSERVICE_TIMEOUT_MS);
    return status;
}

/**
 * jtagserv_init - Contact the JTAG Server
 *
 * Returns ERROR_OK if JTAG Server found.
 * TODO: Write a TCL Command that allows me to specify the jtagserver config file
 */
static int minijtagserv_init(void)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    LOG_DEBUG("Capture server\n");
    char *quartus_jtag_client_config = getenv("QUARTUS_JTAG_CLIENT_CONFIG");
    if (quartus_jtag_client_config != NULL) {
        LOG_INFO("Configuration file, set via QUARTUS_JTAG_CLIENT_CONFIG, is '%s'\n", 
               quartus_jtag_client_config
        );
    } else {
        LOG_INFO("Environment variable QUARTUS_JTAG_CLIENT_CONFIG not set\n"); //TODO: Remove this message, useful for debug will be cause user alarm unnecessarily
    }
    
    AJI_ERROR status = AJI_NO_ERROR;
    
    status = jtagserv_select_cable();
    if (AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot select JTAG Cable. Return status is %d", status);
        jtagservice_free(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
        return ERROR_JTAG_INIT_FAILED;
    }
    
    status = jtagserv_select_tap();
    if (AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot select TAP devices. Return status is %d", status);
        return ERROR_JTAG_INIT_FAILED;
    }
    
    //This driver automanage the tap_state. Hence, this is just
    // to tell the targets which state we think our TAP interface is at.
    tap_set_state(TAP_RESET);
    return ERROR_OK;
}

static int minijtagserv_quit(void)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    //jtagservice_free(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
    return ERROR_OK;
}

int interface_jtag_execute_queue(void)
{   LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
	/* synchronously do the operation here */

	return ERROR_OK;
}

int interface_jtag_add_ir_scan(struct jtag_tap *active, const struct scan_field *fields,
		tap_state_t state)
{   LOG_INFO("***> IN %s(%d): %s", __FILE__, __LINE__, __FUNCTION__);
    {
        LOG_INFO("tap=0x%X, num_bits=%d state=(0x%d) %s:", active->idcode, fields->num_bits, state, tap_state_name(state));
    	
    	int size = DIV_ROUND_UP(fields->num_bits, 8);
    	char *value = hexdump(fields->out_value, size);
        LOG_INFO("  out_value  (size=%d, buf=[%s]) -> %u", size, value, fields->num_bits);
        free(value);
        if(fields->in_value) {
            value = hexdump(fields->in_value, size);
            LOG_INFO("  in_value   (size=%d, buf=[%s]) -> %u", size, value, fields->num_bits);
            free(value);
        } else {
            LOG_INFO("  in_value  : <NONE>");
        }
        if(fields->check_value) {
            value = hexdump(fields->check_value, size);
            LOG_INFO("  check_value (size=%d, buf=[%s]) -> %u", size, value, fields->num_bits);
            free(value);

            value = hexdump(fields->check_mask, size);
            LOG_INFO("  check_mask  (size=%d, buf=[%s]) -> %u", size, value, fields->num_bits);
            free(value);
        } else {
            LOG_INFO(" check_value : <NONE>");
            LOG_INFO(" check_mask  : <NONE>");
        }
        printf("\n");
    }
	/* synchronously do the operation here */

	return ERROR_OK;
}

int interface_jtag_add_plain_ir_scan(int num_bits, const uint8_t *out_bits,
		uint8_t *in_bits, tap_state_t state)
{   LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

    {
    int size = DIV_ROUND_UP(num_bits, 8);
	char *outbits = hexdump(out_bits, size);
    char *inbits  = hexdump(in_bits,  size);
    
	LOG_INFO("  out_bits (size=%d, buf=[%s]) -> %u", size, outbits, num_bits);
	LOG_INFO("  in_bits (size=%d, buf=[%s]) -> %u", size, inbits, num_bits);
	free(outbits);
	free(inbits);
    }

	/* synchronously do the operation here */

	return ERROR_OK;
}

int interface_jtag_add_dr_scan(struct jtag_tap *active, int num_fields,
		const struct scan_field *fields, tap_state_t state)
{   LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    {
        LOG_INFO("tap=0x%X, num_bits=%d state=(0x%d) %s:", active->idcode, fields->num_bits, state, tap_state_name(state));
    	
    	int size = DIV_ROUND_UP(fields->num_bits, 8);
    	char *value = hexdump(fields->out_value, size);
        LOG_INFO("  out_value  (size=%d, buf=[%s]) -> %u", size, value, fields->num_bits);
        free(value);
        if(fields->in_value) {
            value = hexdump(fields->in_value, size);
            LOG_INFO("  in_value   (size=%d, buf=[%s]) -> %u", size, value, fields->num_bits);
            free(value);
        } else {
            LOG_INFO("  in_value  : <NONE>");
        }
        if(fields->check_value) {
            value = hexdump(fields->check_value, size);
            LOG_INFO("  check_value (size=%d, buf=[%s]) -> %u", size, value, fields->num_bits);
            free(value);

            value = hexdump(fields->check_mask, size);
            LOG_INFO("  check_mask  (size=%d, buf=[%s]) -> %u", size, value, fields->num_bits);
            free(value);
        } else {
            LOG_INFO(" check_value : <NONE>");
            LOG_INFO(" check_mask  : <NONE>");
        }
        printf("\n");
    }
	/* synchronously do the operation here */

	return ERROR_OK;
}

int interface_jtag_add_plain_dr_scan(int num_bits, const uint8_t *out_bits,
		uint8_t *in_bits, tap_state_t state)
{   LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

    {
    int size = DIV_ROUND_UP(num_bits, 8);
	char *outbits = hexdump(out_bits, size);
    char *inbits  = hexdump(in_bits,  size);
    
	LOG_INFO("  out_bits (size=%d, buf=[%s]) -> %u", size, outbits, num_bits);
	LOG_INFO("  in_bits (size=%d, buf=[%s]) -> %u", size, inbits, num_bits);
	free(outbits);
	free(inbits);
    }

	/* synchronously do the operation here */

	return ERROR_OK;
}

int interface_jtag_add_tlr()
{   LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

	/* synchronously do the operation here */
	AJI_ERROR status = AJI_NO_ERROR;
	AJI_OPEN_ID open_id = 
        jtagservice.device_open_id_list[jtagservice.in_use_device_tap_position];
        
    status = c_aji_lock(open_id, JTAGSERVICE_TIMEOUT_MS, AJI_PACK_AUTO);
    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot lock device chain. Return status is %d\n", status);
        return ERROR_FAIL;
    }

    status = c_aji_test_logic_reset(open_id);
    if(AJI_NO_ERROR == status) {
        tap_set_state(TAP_RESET);
    } else {
        LOG_ERROR("Unexpected error setting TAPs to TLR state. Return status is %d\n", status);
    }

    AJI_ERROR status2 = c_aji_unlock(open_id);
    if(AJI_NO_ERROR != status) {
        LOG_WARNING("Unexpected error unlocking device chain. Return status is %d\n", status2);
    }

	return (status || status2) ? ERROR_FAIL : ERROR_OK;
}

int interface_jtag_add_reset(int req_trst, int req_srst)
{   LOG_INFO("***> IN %s(%d): %s req_trst=%d, req_srst=%d\n", __FILE__, __LINE__, __FUNCTION__, req_trst, req_srst);
	/* synchronously do the operation here */

	return ERROR_OK;
}

int interface_jtag_add_runtest(int num_cycles, tap_state_t state)
{   LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
	/* synchronously do the operation here */

	return ERROR_OK;
}

int interface_jtag_add_clocks(int num_cycles)
{   LOG_INFO("***> IN %s(%d): %s num_cycyles=%d\n", __FILE__, __LINE__, __FUNCTION__, num_cycles);
	/* synchronously do the operation here */

	return ERROR_OK;
}

int interface_jtag_add_sleep(uint32_t us)
{   LOG_INFO("***> IN %s(%d): %s us=%d\n", __FILE__, __LINE__, __FUNCTION__, us);

	jtag_sleep(us);
	return ERROR_OK;
}

int interface_jtag_add_pathmove(int num_states, const tap_state_t *path)
{   LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

    { 
    LOG_INFO("Transition from state %s to %s in %d steps. Path:", 
             tap_state_name(cmd_queue_cur_state),
             tap_state_name(path[num_states] -1),
             num_states
    );
    printf("  %s", tap_state_name(cmd_queue_cur_state));
    for (int i=0; i<num_states; ++i) {
        printf(" ->$s", tap_state_name(path[i]));
    }
    printf("\n");
    }

	int state_count;
	int tms = 0;

	state_count = 0;

	tap_state_t cur_state = cmd_queue_cur_state;

	while (num_states) {
		if (tap_state_transition(cur_state, false) == path[state_count])
			tms = 0;
		else if (tap_state_transition(cur_state, true) == path[state_count])
			tms = 1;
		else {
			LOG_ERROR("BUG: %s -> %s isn't a valid TAP transition",
					tap_state_name(cur_state), tap_state_name(path[state_count]));
			exit(-1);
		}

		/* synchronously do the operation here */

		cur_state = path[state_count];
		state_count++;
		num_states--;
	}


	/* synchronously do the operation here */

	return ERROR_OK;
}

int interface_add_tms_seq(unsigned num_bits, const uint8_t *seq, enum tap_state state)
{   LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
	/* synchronously do the operation here */

	return ERROR_OK;
}


static struct jtag_interface minijtagserv_interface = {
	.execute_queue = NULL,
};

struct adapter_driver minijtagserv_adapter_driver = {
	.name = "minijtagserv",
	.transports = jtag_only,
	.commands = NULL,

	.init = minijtagserv_init,
	.quit = minijtagserv_quit,
	.speed = NULL,
	.khz = NULL,
	.speed_div = NULL,
	.power_dropout = NULL,
	.srst_asserted = NULL,

	.jtag_ops = &minijtagserv_interface,
};

