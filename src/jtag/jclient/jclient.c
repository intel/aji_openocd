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

#ifdef INC_CONFIG_H
#include "config.h"
#endif

#include <jtag/jtag.h>
#include <target/embeddedice.h>
#include <jtag/minidriver.h>
#include <jtag/interface.h>
#include <helper/jep106.h>

#include "jtag/commands.h"
#include "log.h"

#include "aji/aji.h"
#include "aji/c_jtag_client_gnuaji.h"
#include "jtagservice.h"

extern void jtag_tap_add(struct jtag_tap *t);

// for "jclient hardware" command
#define HARDWARE_OPT_EXPECTED_ID 1

#define IDCODE_SOCVHPS (0x4BA00477)
//#define IDCODE_FE310_G002 (0x20000913)


/** 
 * Masking Manufacturer ID bit[12:1] together with
 *  bit[0]. The latter is always 1
 */
#define JTAG_IDCODE_MANUFID_W_ONE_MASK 0x00000FFF 
/**
 * ARM Manufacturer ID in bit[12:1] with
 * bit[0]=1
 */
#define JTAG_IDCODE_MANUFID_ALTERA_W_ONE 0x0DD 
/**
 * ARM Manufacturer ID in bit[12:1] with 
 * bit[0]=1
 */
#define JTAG_IDCODE_MANUFID_ARM_W_ONE 0x477  
/**
 * SiFive Manufacturer ID in bit[12:1] with
 * bit[0]=1
 */
//#define JTAG_IDCODE_MANUFID_SIFIVE_W_ONE 0x913 

 //=================================
// Global variables
//=================================
static struct jtagservice_record jtagservice;


/**
 * Store user configuration/request.
 * @pre Function must not require any initialization. It is intended to be used
 *      as global/static instance where it is automatically initialized to all zero
 *      (or zero equivalent such as NULL). Initialization is guaranteed by C99.
 */
struct jclient_parameters {
    char* hardware_name;
    char* hardware_id; //<AJI compliant hardware descriptor. @see aji_find_hardware
};
typedef struct jclient_parameters jclient_parameters;

jclient_parameters jclient_config;

AJI_ERROR jclient_parameters_free(jclient_parameters *me) {
    if (me->hardware_id) {
        free(me->hardware_id);
        me->hardware_id = NULL;
    }
    return AJI_NO_ERROR;
}


//=================================
// Helpers
//=================================

/*
 * Access functions to lowlevel driver, agnostic of libftdi/libftdxx
 */
static char *hexdump(const uint8_t *buf, const unsigned int size)
{   //LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
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
// Bits that have to be implemented/copied from src/jtag/core.c
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
{	log_printf_lf(level, __FILE__, __LINE__, __func__,
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
		tap->dotted_name, tap->idcode
	);
	for (unsigned ii = 0; ii < tap->expected_ids_cnt; ii++) {
		char msg[32];

		snprintf(msg, sizeof(msg), "expected %u of %u", ii + 1, tap->expected_ids_cnt);
		jtag_examine_chain_display(LOG_LVL_ERROR, msg,
			tap->dotted_name, tap->expected_ids[ii]);
	}
	return false;
}

/**
 * Match vtap to the list of discovered virtual JTAG/SLD nodes.
 * 
 * \param vtap 
 * \return true if found, false otherwise
 */ /*
static bool vjtag_examine_chain_match_tap(struct vjtag_tap* vtap) {
    AJI_ERROR status = AJI_NO_ERROR;

    DWORD tap_index = -1;
    status = jtagservice_device_index_by_idcode(
        vtap->parent->idcode,
        jtagservice.device_list, 
        jtagservice.device_count,
        &tap_index
    );
    if (AJI_NO_ERROR != status) {
        jtag_examine_chain_display(
            LOG_LVL_ERROR, "UNEXPECTED",
            vtap->parent->dotted_name, vtap->parent->idcode
        );
        return false;
    }

    jtag_examine_chain_display(
        LOG_LVL_INFO, "Parent Tap found:",
        vtap->parent->dotted_name, vtap->parent->idcode
    );

    DWORD node_index = -1;
    status = jtagservice_hier_id_index_by_idcode(
        vtap->idcode,
        jtagservice.hier_ids[tap_index],
        jtagservice.hier_id_n[tap_index],
        &node_index
    );

    if (AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot find virtual tap %s (0x%08l" PRIX32 "). Return status is %d (%s)",
            vtap->dotted_name, (unsigned long) vtap->expected_ids[0],
            status, c_aji_error_decode(status)
        );
        return false;
    }
    LOG_INFO("Virtual Tap/SLD node %06lX found at tap position %lu vtap position %lu",
        (unsigned long) vtap->expected_ids[0], (unsigned long) tap_index, (unsigned long) node_index
    );
    return true;
}
*/
/**
 * @pre jtagservice is filled with device inforamtion. That should
 *      already been done during adapter initialization via 
 *      @c jclient_init() function.
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
				return ERROR_JTAG_INIT_FAILED;
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
        } 

        if ((device.device_id & 1) == 0) {
            /* Zero for LSB indicates a device in bypass */
            LOG_DEBUG("TAP %s does not have valid IDCODE (idcode=0x%08l" PRIx32 ")",  tap->dotted_name, (unsigned long) device.device_id);
            tap->hasidcode = false;
            tap->idcode = 0;
        }  else {
            /* Friendly devices support IDCODE */
            tap->idcode = device.device_id;
            tap->hasidcode = true;
            jtag_examine_chain_display(LOG_LVL_INFO, "tap/device found", tap->dotted_name, tap->idcode);
        }

        /* ensure the TAP ID matches what was expected */
		if (!jtag_examine_chain_match_tap(tap))
			retval = ERROR_JTAG_INIT_SOFT_FAIL;
    } //end for t
/*
    if (AJI_NO_ERROR != retval && ERROR_JTAG_INIT_SOFT_FAIL != retval) {
        return retval;
    }

    for (struct vjtag_tap *vtap = vjtag_all_taps();
        vtap != NULL;
        vtap = (struct vjtag_tap*) vtap->next_tap) {
        vtap->idcode = vtap->expected_ids[0];
        vtap->hasidcode = true;

        if (!vjtag_examine_chain_match_tap(vtap)) {
            vtap->hasidcode = false;
            vtap->idcode = 0;
            retval = ERROR_JTAG_INIT_SOFT_FAIL;
            continue;
        }
    } */
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
 * @return AJI_NO_ERROR if everything OK
 *         AJI_NO_MEMORY if run out of memory
 *         AJI_BAD_HARDWARE if cannot find hardware cable
 *         AJI_TIMEOUT if time out occured.
 * @pos Set jtagservice chain detail if return successful.
 */
static AJI_ERROR select_cable(void)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    AJI_ERROR status = AJI_NO_ERROR;

    DWORD MYJTAGTIMEOUT = 250; //ms
    DWORD TRIES = 4 * 60; 
    for (DWORD i = 0; i < TRIES; i++) {
        LOG_INFO("Querying JTAG Server (timeout = %lu s) ...\n", (long unsigned) MYJTAGTIMEOUT * TRIES / 1000);
        if (jclient_config.hardware_id != NULL) {
            LOG_INFO("Attempting to find '%s'", jclient_config.hardware_id);
            jtagservice.hardware_count = 1;
            jtagservice.hardware_list =
                calloc(jtagservice.hardware_count, sizeof(AJI_HARDWARE));
            jtagservice.server_version_info_list =
                calloc(jtagservice.hardware_count, sizeof(char*));
            if (jtagservice.hardware_list == NULL
                || jtagservice.server_version_info_list == NULL
            ) {
                return AJI_NO_MEMORY;
            }

            status = c_aji_find_hardware_a(
                jclient_config.hardware_id,
                &(jtagservice.hardware_list[0]),
                MYJTAGTIMEOUT
            );
            /* Documentation says retry immediately after a timeout,
               as AJI is still trying to connect at the background
               after timeout so immediate retry might be successful
             */
            if (AJI_TIMEOUT == status) {
                status = c_aji_find_hardware_a(
                    jclient_config.hardware_id,
                    &(jtagservice.hardware_list[0]),
                    0
                );
            }

            if (AJI_NO_ERROR != status) {
                LOG_ERROR("Cannot find cable '%s'. Return status is %d (%s)",
                    jclient_config.hardware_id,
                    status, c_aji_error_decode(status)
                );
            }
        }
        else {
            LOG_INFO("No cable specified, so searching for cables");
            status = c_aji_get_hardware2(
                &(jtagservice.hardware_count),
                jtagservice.hardware_list,
                jtagservice.server_version_info_list,
                MYJTAGTIMEOUT
            );
            if (AJI_TOO_MANY_DEVICES == status) {
                jtagservice.hardware_list =
                    calloc(jtagservice.hardware_count, sizeof(AJI_HARDWARE));
                jtagservice.server_version_info_list =
                    calloc(jtagservice.hardware_count, sizeof(char*));
                if (jtagservice.hardware_list == NULL
                    || jtagservice.server_version_info_list == NULL
                ) {
                    return AJI_NO_MEMORY;
                }
                status = c_aji_get_hardware2(
                    &(jtagservice.hardware_count),
                    jtagservice.hardware_list,
                    jtagservice.server_version_info_list,
                    0
                );
            } //end if (AJI_TOO_MANY_DEVICES)
        } //end if (jclient_config.hardware_id != NULL)

        if (status != AJI_TIMEOUT)
            break;
        if (i == 2)
            fprintf(stderr, "Connecting to server(s) [.                   ]"
                "\b\b\b\b\b\b\b\b\b\b"
                "\b\b\b\b\b\b\b\b\b\b"
                "\b\b\b\b\b\b\b\b\b\b"
                "\b\b\b\b\b\b\b\b\b\b"
                "\b\b\b\b\b\b\b\b\b\b"
                "\b\b\b\b\b\b\b\b\b\b");
        else if ((i % 4) == 2)
            fprintf(stderr, ".");
        fflush(stderr);

    } // end for i


    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failed to query server for hardware cable information. "
                  " Return Status is %i (%s)\n", 
                  status, c_aji_error_decode(status)
        );
        return status;
    }
    if(0 == jtagservice.hardware_count) {
        LOG_ERROR("JTAG server reports that it has no hardware cable\n");
        return AJI_BAD_HARDWARE;
    }

    if (jclient_config.hardware_id == NULL) {
        LOG_INFO("At present, The first hardware cable will be used"
            " [%lu cable(s) detected]",
            (unsigned long)jtagservice.hardware_count
        );
    }

    AJI_HARDWARE hw = jtagservice.hardware_list[0];
    LOG_INFO("Cable %u: device_name=%s, hw_name=%s, server=%s, port=%s,"
              " chain_id=%p, persistent_id=%lu, chain_type=%d, features=%lu,"
              " server_version_info=%s\n", 
          1, hw.device_name, hw.hw_name, hw.server, hw.port,  
          hw.chain_id, (unsigned long) hw.persistent_id, 
          hw.chain_type, (unsigned long) hw.features,
          jtagservice.server_version_info_list[0]
    );
    
    if (jtagservice.hardware_list[0].hw_name == NULL) {
        LOG_ERROR("No hardware present because hw_name is NULL");
        return AJI_FAILURE;
    }
    jtagservice.in_use_hardware = &(jtagservice.hardware_list[0]);
    jtagservice.in_use_hardware_chain_pid = (jtagservice.in_use_hardware)->persistent_id;
    jtagservice.in_use_hardware_chain_id = (jtagservice.in_use_hardware)->chain_id;
    return status;
}

static AJI_ERROR unselect_tap(void);
static AJI_ERROR unselect_cable(void) {
    AJI_ERROR retval = AJI_NO_ERROR;
    AJI_ERROR status = AJI_NO_ERROR;

    status = unselect_tap();
    if (status) {
        retval = status;
    }

    status = jtagservice_free_cable(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
    if (status) {
        retval = status;
    }
    return retval;
}

/**
 * Select the TAP device to use
 * @pre The chain is already acquired, @see select_cable()
 * @pos jtagservice will be populated with the selected tap
 */
static AJI_ERROR select_tap(void)
{   LOG_DEBUG("***> IN %s(%d): %s in_use_hardware_chain_pid=%lu\n", __FILE__, __LINE__, __FUNCTION__, (unsigned long) jtagservice.in_use_hardware_chain_pid);
    AJI_ERROR status = AJI_NO_ERROR;

    AJI_HARDWARE hw;
    status = c_aji_find_hardware(jtagservice.in_use_hardware_chain_pid, &hw, JTAGSERVICE_TIMEOUT_MS);
    if(AJI_NO_ERROR != status){
        return status;       
    }

    status = c_aji_lock_chain(hw.chain_id, JTAGSERVICE_TIMEOUT_MS);
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
        jtagservice.device_type_list = calloc(jtagservice.device_count, sizeof(DEVICE_TYPE));

        if (jtagservice.device_list == NULL
            || jtagservice.device_open_id_list == NULL
            || jtagservice.device_type_list == NULL
        ) {
            return AJI_NO_MEMORY;
        }
        status = c_aji_read_device_chain(
            hw.chain_id, 
            &(jtagservice.device_count), 
            jtagservice.device_list,
            1
        );
    }    

    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failed to query server for TAP information. "
                  " Return Status is %i (%s)\n", 
                  status, c_aji_error_decode(status)
        );
        c_aji_unlock_chain(hw.chain_id);
        return status;
    }

    if(0 == jtagservice.device_count) {
        LOG_ERROR("JTAG server reports that it has no TAP attached to the cable");
        c_aji_unlock_chain(hw.chain_id);
        return AJI_NO_DEVICES;
    }
    LOG_INFO("At present, will not honour OpenOCD target selection and"
             " try to select the ARM SOCVHPS with IDCODE %X",
             IDCODE_SOCVHPS
    );
    LOG_INFO("Will Search through %lx TAP devices", (unsigned long) jtagservice.device_count);
/*
    //SLD discovery
    bool sld_discovery_failed = false;
    jtagservice.hier_id_n = (DWORD*) calloc(jtagservice.device_count, sizeof(DWORD));
    jtagservice.hier_ids = (AJI_HIER_ID**) calloc(jtagservice.device_count, sizeof(AJI_HIER_ID*));
    jtagservice.hub_infos = (AJI_HUB_INFO**) calloc(jtagservice.device_count, sizeof(AJI_HUB_INFO*));
    jtagservice.hier_id_open_id_list = (AJI_OPEN_ID**)calloc(jtagservice.device_count, sizeof(AJI_OPEN_ID*));
    jtagservice.hier_id_type_list = (DEVICE_TYPE**) calloc(jtagservice.device_count, sizeof(DEVICE_TYPE*));
    if (NULL == jtagservice.hier_id_n) {
        LOG_ERROR("Ran out of memory for jtagservice's hier_id_n list");
        return AJI_NO_MEMORY;
    }
    if (NULL == jtagservice.hier_ids) {
        LOG_ERROR("Ran out of memory for jtagservice's hier_ids list");
        return AJI_NO_MEMORY;
    }
    if (NULL == jtagservice.hub_infos) {
        LOG_ERROR("Ran out of memory for jtagservice's hub_infos list");
        return AJI_NO_MEMORY;
    }
    if (NULL == jtagservice.hier_id_open_id_list) {
        LOG_ERROR("Ran out of memory for jtagservice's hier_id_open_id_list");
        return AJI_NO_MEMORY;
    }
    if (NULL == jtagservice.hier_id_type_list) {
        LOG_ERROR("Ran out of memory for jtagservice's hier_id_type_list");
        return AJI_NO_MEMORY;
    }

    for (DWORD tap_position = 0; tap_position < jtagservice.device_count; ++tap_position) {
        jtagservice.hub_infos[tap_position] = (AJI_HUB_INFO*)calloc(AJI_MAX_HIERARCHICAL_HUB_DEPTH, sizeof(AJI_HUB_INFO));
        if (NULL == jtagservice.hub_infos[tap_position]) {
            LOG_ERROR("Ran out of memory for jtagservice's tap  %lu's hub_info", (unsigned long) tap_position);
            return AJI_NO_MEMORY;
        }
        jtagservice.hier_id_n[tap_position] = 10; //@TODO Find a good compromise for number of SLD so I don't have to call c_aji_get_nodes_b() twice.
        jtagservice.hier_ids[tap_position] = (AJI_HIER_ID*)calloc(jtagservice.hier_id_n[tap_position], sizeof(AJI_HIER_ID));
        status = c_aji_get_nodes_b(
            hw.chain_id, //could be jtagservice.in_use_hardware_chain_id,
            tap_position,
            jtagservice.hier_ids[tap_position],
            &(jtagservice.hier_id_n[tap_position]),
            jtagservice.hub_infos[tap_position]
        );
        if (AJI_TOO_MANY_DEVICES == status) {
            free(jtagservice.hier_ids[tap_position]);
            jtagservice.hier_ids[tap_position] = (AJI_HIER_ID*)calloc(jtagservice.hier_id_n[tap_position], sizeof(AJI_HIER_ID));
            if (NULL == jtagservice.hub_infos[tap_position]) {
                LOG_ERROR("Ran out of memory for jtagservice's tap  %lu's hier_ids", (unsigned long) tap_position);
                return AJI_NO_MEMORY;
            }

            status = c_aji_get_nodes_b(
                hw.chain_id, //could be jtagservice.in_use_hardware_chain_id,
                tap_position,
                jtagservice.hier_ids[tap_position],
                &(jtagservice.hier_id_n[tap_position]),
                jtagservice.hub_infos[tap_position]
            );
        } // end if (AJI_TOO_MANY_DEVICES)

        if (AJI_NO_ERROR != status) {
            LOG_ERROR("Problem with getting nodes for TAP position %lu. Returned %d (%s)", 
                (unsigned long) tap_position, status, c_aji_error_decode(status));
            sld_discovery_failed = true;
            continue;
        }

        jtagservice.hier_id_open_id_list[tap_position] = \
            (AJI_OPEN_ID*)calloc(jtagservice.hier_id_n[tap_position], sizeof(AJI_OPEN_ID));
        jtagservice.hier_id_type_list[tap_position] = \
            (DEVICE_TYPE*)calloc(jtagservice.hier_id_n[tap_position], sizeof(DEVICE_TYPE));
        if (NULL == jtagservice.hub_infos[tap_position]) {
            LOG_ERROR("Ran out of memory for jtagservice's tap  %lu's hier_ids_claims",
                (unsigned long)tap_position);
            return AJI_NO_MEMORY;
        }

        LOG_INFO("TAP position %lu (%lX) has %lu SLD nodes",
            (unsigned long) tap_position, 
            (unsigned long) jtagservice.device_list[tap_position].device_id,
            (unsigned long) jtagservice.hier_id_n[tap_position]
        );
        if (jtagservice.hier_id_n[tap_position]) {
            for (DWORD n = 0; n < jtagservice.hier_id_n[tap_position]; ++n) {
                LOG_INFO("    node %2lu idcode=%08lX position_n=%lu",
                    (unsigned long) n,
                    (unsigned long) (jtagservice.hier_ids[tap_position][n].idcode),
                    (unsigned long) (jtagservice.hier_ids[tap_position][n].position_n)
                    );
                jtagservice.hier_id_type_list[tap_position][n] = VJTAG; //@TODO Might have to ... 
                                     //... replace with  node specific claims
            } //end for(n in jtagservice.hier_id_n[tap_position])
        } //end if (jtagservice.hier_id_n[tap_position])
    } //end for tap_position (SLD discovery)
    if (sld_discovery_failed) {
        LOG_WARNING("Have failures in SLD discovery. See previous log entries. Continuing ...");
    }
*/
    DWORD arm_index = 0;
    bool  found_arm = false;
    for(DWORD tap_position=0; tap_position<jtagservice.device_count; ++tap_position) {
        AJI_DEVICE device = jtagservice.device_list[tap_position];
        LOG_INFO("Detected device (tap_position=%lu) device_id=%08lx," 
                  " instruction_length=%d, features=%lu, device_name=%s", 
                    (unsigned long) tap_position, 
                    (unsigned long) device.device_id, 
                    device.instruction_length, 
                    (unsigned long) device.features, 
                    device.device_name
        );
        DWORD manufacturer_with_one = device.device_id & JTAG_IDCODE_MANUFID_W_ONE_MASK;
/*
        if (JTAG_IDCODE_MANUFID_SIFIVE_W_ONE == manufacturer_with_one) {
            jtagservice.device_type_list[tap_position] = RISCV; 
            arm_index = tap_position;
            found_arm = true;
            LOG_INFO("Found SiFive device at tap_position %lu", (unsigned long)arm_index);
        } */
        if(JTAG_IDCODE_MANUFID_ARM_W_ONE == manufacturer_with_one) {
            jtagservice.device_type_list[tap_position] = ARM;
            arm_index = tap_position;
            found_arm = true;
            LOG_INFO("Found SOCVHPS device at tap_position %lu.", (unsigned long)arm_index);
        }
    } //end for tap_position

    if(!found_arm) {
        LOG_ERROR("No SOCVHPS or device found.");
        c_aji_unlock_chain(hw.chain_id);
        return AJI_FAILURE;
    }


    
    CLAIM_RECORD claims =
        jtagservice.claims[jtagservice.device_type_list[arm_index]];

    // Only allowed to open one device at a time.
    // If you don't, then anytime after  c_aji_test_logic_reset() call, 
    //  you can get fatal error
    //  "*** glibc detected *** src/openocd: double free or corruption (fasttop): 0x00000000027bc7a0 ***"
//#if PORT == WINDOWS
    status = c_aji_open_device(
        hw.chain_id,
        arm_index,
        &(jtagservice.device_open_id_list[arm_index]),
        claims.claims, claims.claims_n, jtagservice.appIdentifier
    );
//#else
//    status = c_aji_open_device_a(
//        hw.chain_id,
//        arm_index,
//        &(jtagservice.device_open_id_list[arm_index]),
//        claims.claims, claims.claims_n, jtagservice.appIdentifier
//    );
//#endif
    if(AJI_NO_ERROR != status) {
            LOG_ERROR("Cannot open device number %lu (IDCODE=%lX). Returned %d (%s)",
                      (unsigned long) arm_index,
                      (unsigned long) jtagservice.device_list[arm_index].device_id,
		      status,
		      c_aji_error_decode(status)
            );
    }
    status = c_aji_unlock_chain(hw.chain_id);
    if (AJI_NO_ERROR != status) {
        LOG_WARNING("Cannot unlock JTAG Chain ");
    }
    
    jtagservice_activate_jtag_tap(&jtagservice, 0,  arm_index);
LOG_DEBUG("***> END %s(%d): %s in_use_device_tap_position=%lu\n", __FILE__, __LINE__, __FUNCTION__, (unsigned long) jtagservice.in_use_device_tap_position);
    return status;
}

static AJI_ERROR unselect_tap(void) {
    AJI_ERROR status = AJI_NO_ERROR;
    status = jtagservice_free_tap(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
    return status;
}

AJI_ERROR reacquire_open_id(void) 
{  LOG_DEBUG("***> IN %s(%d): %s in_use_device_tap_position=%lu\n", __FILE__, __LINE__, __FUNCTION__, (unsigned long)jtagservice.in_use_device_tap_position);
jtagservice.in_use_device_tap_position = 1;
    int max_try = 5;
    int sleep_duration = 5; //seconds
    AJI_ERROR status = AJI_NO_ERROR;
    
    for(int try=0; try<max_try; ++try) {
        unselect_cable();
        if(try) {
            LOG_INFO("Sleep %d sec before reattempting cable acquisition", sleep_duration);
            sleep(sleep_duration);
        }
        LOG_INFO("Attempt to reacquire cable. Try %d of %d ... ", try+1, max_try);
        status = select_cable();
        if(AJI_NO_ERROR == status) {
            LOG_INFO(" ... Succeeded");
            break;
        }
        LOG_INFO(" ... Not successful. Return status is %d (%s)",
            status, c_aji_error_decode(status)
        );
    }
    
    if(AJI_NO_ERROR != status) {
       LOG_INFO("Give Up!");
       return status;
    }

    for(int try=0; try<max_try; ++try) {
        unselect_tap();
        if(try) {
            LOG_INFO("Sleep %d sec before reattempting tap selection", sleep_duration);
            sleep(sleep_duration);
        }
        LOG_INFO("Attempt to select tap. Try %d of %d ... ", try+1, max_try);
        status = select_tap();
        if(AJI_NO_ERROR == status) {
            LOG_INFO(" ... Succeeded");
            break;
        }
        LOG_INFO(" ... Not successful. Return status is %d (%s)",
            status, c_aji_error_decode(status)
        );
    }
    return status;
}

/**
 * init - Contact the JTAG Server
 *
 * Returns ERROR_OK if JTAG Server found.
 * TODO: Write a TCL Command that allows me to specify the jtagserver config file
 */
static int jclient_init(void)
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

    jtagservice_init(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
    AJI_ERROR status = AJI_NO_ERROR;

    status = c_jtag_client_gnuaji_init();
    if (AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot initialize C_JTAG_CLIENT library. Return status is %d (%s)",
            status, c_aji_error_decode(status)
        );
        jtagservice_free(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
        jclient_parameters_free(&jclient_config);
        return ERROR_JTAG_INIT_FAILED;
    }

jtagservice_query_main();

    status = select_cable();
    if (AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot select JTAG Cable. Return status is %d (%s)",
            status, c_aji_error_decode(status)
        );
        jtagservice_free(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
        jclient_parameters_free(&jclient_config);
        return ERROR_JTAG_INIT_FAILED;
    }
    
    status = select_tap();
    if (AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot select TAP devices. Return status is %d (%s)",
            status, c_aji_error_decode(status)
        );
        return ERROR_JTAG_INIT_FAILED;
    }
    
    //This driver automanage the tap_state. Hence, this is just
    // to tell the targets which state we think our TAP interface is at.
    tap_set_state(TAP_RESET);
    return ERROR_OK;
}

static int jclient_quit(void)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    jtagservice_free(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
    jclient_parameters_free(&jclient_config);


    c_jtag_client_gnuaji_free();

    return ERROR_OK;
}

int interface_jtag_execute_queue(void)
{   //LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

	/* synchronously do the operation here */
	static int reentry;

	assert(reentry == 0);
	reentry++;

    /* Original logic wants to make sure that the operation request
     * is completed correctly before firing off the callbacks(). 
     * Right now I am have no idea whether the commands were executed correctly
     * and is firing blind.
     */
	int retval = ERROR_OK; //default_interface_jtag_execute_queue(); 
	if (retval == ERROR_OK) {
		struct jtag_callback_entry *entry;
//LOG_DEBUG("***> IN %s(%d): %s  %s\n", __FILE__, __LINE__, __FUNCTION__, jtag_callback_queue_head == NULL? "No callbacks" : "Firing callbacks");
		for (entry = jtag_callback_queue_head; entry != NULL; entry = entry->next) {
			retval = entry->callback(entry->data0, entry->data1, entry->data2, entry->data3);
			if (retval != ERROR_OK)
				break;
		}
	}

	jtag_command_queue_reset();
	jtag_callback_queue_reset();

	reentry--;

	return retval;
}


int interface_jtag_add_ir_scan(struct jtag_tap *active, const struct scan_field *fields,
		tap_state_t state)
{   //LOG_DEBUG("***> IN %s(%d): %s", __FILE__, __LINE__, __FUNCTION__);
    /*
    {
        LOG_INFO("tap=0x%X, num_bits=%d state=(0x%d) %s:", active->idcode, fields->num_bits, state, tap_state_name(state));
    	
    	int size = DIV_ROUND_UP(fields->num_bits, 8);
    	char *value = hexdump(fields->out_value, size);
        LOG_INFO("  out_value  (size=%d, buf=[0x%s]) -> %u", size, value, fields->num_bits);
        free(value);
        if(fields->in_value) {
            value = hexdump(fields->in_value, size);
            LOG_INFO("  in_value   (size=%d, buf=[0x%s]) -> %u", size, value, fields->num_bits);
            free(value);
        } else {
            LOG_INFO("  in_value  : <NONE>");
        }
        if(fields->check_value) {
            value = hexdump(fields->check_value, size);
            LOG_INFO("  check_value (size=%d, buf=[0x%s]) -> %u", size, value, fields->num_bits);
            free(value);

            value = hexdump(fields->check_mask, size);
            LOG_INFO("  check_mask  (size=%d, buf=[0x%s]) -> %u", size, value, fields->num_bits);
            free(value);
        } else {
            LOG_INFO(" check_value : <NONE>");
            LOG_INFO(" check_mask  : <NONE>");
        }
        printf("\n");
    }
    */
	/* synchronously do the operation here */

	/* loop over all enabled TAPs. */
	
    AJI_ERROR  status = AJI_NO_ERROR;
    DWORD arm_or_ricsv_index = jtagservice.in_use_device_tap_position;

    DWORD activeIndex = 0;
    for (struct jtag_tap *t = jtag_tap_next_enabled(NULL); 
         t != NULL; 
         t = jtag_tap_next_enabled(t),
            ++activeIndex
    ) {
        if (t == active) {
            break;
        }
    }
    if (activeIndex == jtagservice.device_count) {
        LOG_ERROR("IR - Cannot find requested tap 0x%08lX for IR instruction", 
		  (unsigned long) active->idcode
        );
        return ERROR_FAIL;
    }
assert(activeIndex = arm_or_ricsv_index);  //At present, it should be the same because we cannot access other taps
/*
    if (jtag_tap_on_all_vtaps_list(active)) {
        DWORD tap_index=0;
        jtagservice_device_index_by_idcode(
            ((struct vjtag_tap*)active)->parent->idcode,
            jtagservice.device_list,   
            jtagservice.device_count, 
            &tap_index
        ); //Not checking return status because it should pass

        DWORD sld_index=0;
        jtagservice_hier_id_index_by_idcode(
            active->idcode,
            jtagservice.hier_ids[tap_index],
            jtagservice.hier_id_n[tap_index],
            &sld_index); //Not checking return status because it should pass
            
        if(jtagservice.is_sld == false
            || jtagservice.in_use_device_tap_position != tap_index 
            || jtagservice.in_use_hier_id_node_position != sld_index
        ) {
            status = jtagservice_activate_virtual_tap(&jtagservice, 0, tap_index, sld_index);
            if (AJI_NO_ERROR != status) {
                LOG_ERROR("IR - Cannot activate virtual tap %s (0x%08l" PRIX32 "). Return status is %d (%s)",
                            active->dotted_name, (unsigned long)active->expected_ids[0],
                            status, c_aji_error_decode(status)
                );
assert(0); //deliberately assert() to be able to see where the error is, if it occurs
                return ERROR_FAIL;
            }            
        } //end if jtagservice.is_sld
    } //end if if(jtag_tap_on_all_vtaps_list(active))
    */
    //FORCE BACK TO SOCVHPS
    if (
        jtagservice.in_use_device_tap_position != arm_or_ricsv_index 
    ) {
        status = jtagservice_activate_jtag_tap(&jtagservice, 0, arm_or_ricsv_index);
    
        if (AJI_NO_ERROR != status) {
            LOG_ERROR("IR - Cannot reactivate physical tap %s (0x%08l" PRIX32 "). Return status is %d (%s)",
                    active->dotted_name, (unsigned long)active->expected_ids[0],
                    status, c_aji_error_decode(status)
            );
assert(0); //deliberately assert() to be able to see where the error is, if it occurs
            return ERROR_FAIL;
        }
    } //end if(jtagservice.is_sld == true)        


	AJI_OPEN_ID open_id = jtagservice.device_open_id_list[jtagservice.in_use_device_tap_position];

	status = c_aji_lock(open_id, JTAGSERVICE_TIMEOUT_MS, AJI_PACK_NEVER);
	if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failure to lock before accessing IR register. Return Status is %d (%s). Reacquiring lock ...\n", 
            status, c_aji_error_decode(status)
        );
    	status = reacquire_open_id();
    }

	if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failure to lock before accessing IR register. Return Status is %d (%s)\n", 
            status, c_aji_error_decode(status)
        );
assert(0); //deliberately assert() to be able to see where the error is, if it occurs
        return ERROR_FAIL;
    }
  
    /*
    if(fields->out_value) {
        int size = DIV_ROUND_UP(fields->num_bits, 8);
	    char *value = hexdump(fields->out_value, size);
        LOG_INFO("IR write  (size=%d, buf=[0x%s]) -> %u", size, value, fields->num_bits);
	    free(value);
    } else {
        LOG_INFO("No IR write");
    }
    */
    
    /* the code below could had been replaced by c_aji_access_ir_a(), i.e.
       the BYTE* version of aji_access_ir(). However, for quartus 20.3
       there is a bug where (1) it expects write_data and read_data to be
       DWORDs and  it sends the wrong instruction to the TAP, i.e., it 
       did not send write_bit. */  
    DWORD instruction = 0;
    for (int i = 0 ; i <  (fields->num_bits+7)/8 ; i++) {
        instruction |= fields->out_value[i] << (i * 8);
    }

    DWORD capture = 0;
    status = c_aji_access_ir(open_id, instruction, fields->in_value? &capture : NULL, 0);

    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failure to access IR register. Return Status is %d (%s)\n", 
            status, c_aji_error_decode(status)
        );
	    c_aji_unlock(open_id);
assert(0);   //deliberately assert() to be able to see where the error is, if it occurs
        return ERROR_FAIL;
    }
    /*
    if(fields->in_value) {
        int size = DIV_ROUND_UP(fields->num_bits, 8);
    	char *value = hexdump((uint8_t*) &capture, size);
        LOG_INFO("IR read  (size=%d, buf=[0x%s]) -> %u", size, value, fields->num_bits);
    	free(value);
    } else {
        LOG_INFO("No IR read");
    }
    */
    if(fields->in_value) {
        for (int i = 0 ; i < (fields->num_bits+7)/8 ; i++) {
           fields->in_value[i] = (BYTE) (capture >> (i * 8));
        }
    }
    /*
    if(fields->in_value) {
        int size = DIV_ROUND_UP(fields->num_bits, 8);
    	char *value = hexdump(fields->in_value, size);
        LOG_DEBUG("fields.in_value  (size=%d, buf=[0x%s]) -> %u", size, value, fields->num_bits);
    	free(value);
    }
    */
    tap_set_state(TAP_IRUPDATE);
        
    if(TAP_IDLE != state) {
        LOG_WARNING("IR SCAN not yet handle transition to state other than TAP_IDLE(%d). Requested state is %s(%d)", TAP_IDLE, tap_state_name(state), state);
assert(0); //deliberately assert() to be able to see where the error is, if it occurs
    }
    
    status = c_aji_run_test_idle(open_id, 2);
    if(AJI_NO_ERROR != status) {
        LOG_WARNING("Failed to go to TAP_IDLE after IR register write");
    }

    c_aji_unlock(open_id);
	return ERROR_OK;
}

int interface_jtag_add_plain_ir_scan(int num_bits, const uint8_t *out_bits,
		uint8_t *in_bits, tap_state_t state)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

    {
    int size = DIV_ROUND_UP(num_bits, 8);
	char *outbits = hexdump(out_bits, size);
    char *inbits  = hexdump(in_bits,  size);
    
	LOG_INFO("  out_bits (size=%d, buf=[0x%s]) -> %u", size, outbits, num_bits);
	LOG_INFO("  in_bits (size=%d, buf=[0x%s]) -> %u", size, inbits, num_bits);
	free(outbits);
	free(inbits);
    }

	/* synchronously do the operation here */

    /* Only needed if we are going to permit SVF or XSVF use */
    LOG_ERROR("No plan to implement interface_jtag_add_plain_ir_scan");
 assert(0);
    
	return ERROR_FAIL;
}

int interface_jtag_add_dr_scan(struct jtag_tap *active, int num_fields,
		const struct scan_field *fields, tap_state_t state)
{   //LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

/*    {
        LOG_INFO("tap=0x%X, num_fields=%d state=(0x%d) %s:", active->idcode, num_fields, state, tap_state_name(state));
    	for(int i=0; i<num_fields; ++i) {
        	int size = DIV_ROUND_UP(fields[i].num_bits, 8);
        	char *value = NULL;
        	if(fields[i].out_value) {
        	    hexdump(fields[i].out_value, size);
                LOG_INFO("  fields[%d].out_value  (size=%d, buf=[0x%s]) -> %u", i, size, value, fields[i].num_bits);
                free(value);
            } else {
                LOG_INFO("  fields[%d].out_value  : <NONE>", i);
            }
            if(fields[i].in_value) {
                value = hexdump(fields[i].in_value, size);
                LOG_INFO("  fields[%d].in_value   (size=%d, buf=[0x%s]) -> %u", i, size, value, fields[i].num_bits);
                free(value);
            } else {
                LOG_INFO("  fields[%d].in_value  : <NONE>", i);
            }
            if(fields[i].check_value) {
                value = hexdump(fields[i].check_value, size);
                LOG_INFO("  fields[%d].check_value (size=%d, buf=[0x%s]) -> %u", i, size, value, fields[i].num_bits);
                free(value);

                value = hexdump(fields[i].check_mask, size);
                LOG_INFO("  fields[%d].check_mask  (size=%d, buf=[0x%s]) -> %u", i, size, value, fields[i].num_bits);
                free(value);
            } else {
                LOG_INFO("  fields[%d].check_value : <NONE>", i);
                LOG_INFO("  fields[%d].check_mask  : <NONE>", i);
            }
        } //end for(i)
        printf("\n");
    }
*/    
	/* synchronously do the operation here */

    DWORD activeIndex = 0;
    for (struct jtag_tap* t = jtag_tap_next_enabled(NULL);
        t != NULL;
        t = jtag_tap_next_enabled(t),
        ++activeIndex
        ) {
        if (t == active) {
            break;
        }
    }
    if (activeIndex == jtagservice.device_count) {
        LOG_ERROR("DR - Cannot find requested tap 0x%08lX for DR instruction", 
		  (unsigned long) active->idcode
        );
        return ERROR_FAIL;
    }
assert(activeIndex = jtagservice.in_use_device_tap_position);  //At present, it should be the same because we cannot access other taps

    /* Right now, we can only do ARMVHPS or FE310-G002 */
    uint32_t idcode = active->idcode;
/*    if (jtag_tap_on_all_vtaps_list(active)) {
        idcode = ((struct vjtag_tap*)active)->parent->idcode;
    }
*/    
    static bool NotYetWarned = false;
    if (NotYetWarned) {
        if (idcode == 0 || jtagservice.in_use_device_id != idcode) {
            LOG_WARNING("DR - Expecting TAP with IDCODE 0x%08lX to be the active tap, but got 0x%08lX, but never mind, code currently will use as 0x%08lX active tap.",
                (unsigned long)jtagservice.in_use_device_id,
                (unsigned long)idcode,
                (unsigned long)jtagservice.in_use_device_id
            );
        }
    }


    if (jtagservice.in_use_device_tap_position != activeIndex
        ) {
        AJI_ERROR status = jtagservice_activate_jtag_tap(&jtagservice, 0, activeIndex);

        if (AJI_NO_ERROR != status) {
            LOG_ERROR("DR - Cannot reactivate physical tap %s (0x%08l" PRIX32 "). Return status is %d (%s)",
                active->dotted_name, (unsigned long)active->expected_ids[0],
                status, c_aji_error_decode(status)
            );
            assert(0); //deliberately assert() to be able to see where the error is, if it occurs
            return ERROR_FAIL;
        }
    } //end if(jtagservice.is_sld == true)        

    /* prepare the input/output fields for AJI */
    DWORD length_dr = 0;
    for(int i=0; i<num_fields; ++i) {
        length_dr += fields[i].num_bits;
    }

    _Bool write_to_dr = false, read_from_dr = false;
    BYTE *read_bits = calloc((length_dr+7)/8, sizeof(BYTE));
    BYTE *write_bits = calloc((length_dr+7)/8, sizeof(BYTE));    
    DWORD bit_count = 0;
	for (int i = 0; i < num_fields; i++) {
		if (fields[i].out_value) {	
    		buf_set_buf(fields[i].out_value, 0, 
	    	            write_bits,	bit_count, 
	    	            fields[i].num_bits
	    	);
	    	write_to_dr = true;
	    }
	    if(fields[i].in_value) {
	        read_from_dr = true;
	    }
	    bit_count += fields[i].num_bits;
	} 
/*printf("BEFORE: length_dr=%d write_bits=0x%X%X%X%X read_bits=0x%X%X%X%X\n", length_dr,
          write_bits[3],write_bits[2],write_bits[1],write_bits[0], 
          read_bits[3],read_bits[2],read_bits[1],read_bits[0]
    );    
*/
    /*
    if (write_to_dr) { 
        int size = DIV_ROUND_UP(length_dr, 8);
	    char *value = hexdump(fields->out_value, size);
        LOG_INFO("DR write  (size=%d, buf=[0x%s]) -> %lu", size, value, (unsigned long) length_dr);
	    free(value);
    } else {
        LOG_INFO("No DR write");
    }
    */

	AJI_ERROR  status = AJI_NO_ERROR;
	AJI_OPEN_ID open_id = jtagservice.device_open_id_list[jtagservice.in_use_device_tap_position];

	status = c_aji_lock(open_id, JTAGSERVICE_TIMEOUT_MS, AJI_PACK_NEVER);
	if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failure to lock before accessing DR register. Return Status is %d (%s). Reacquiring lock ...\n", 
            status, c_aji_error_decode(status)
        );
    	status = reacquire_open_id();
    }
	if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failure to lock before accessing DR register. Return Status is %d (%s)\n", 
            status, c_aji_error_decode(status)
        );
        free(read_bits);
        free(write_bits);
assert(0); //deliberately assert() to be able to see where the error is, if it occurs
        return ERROR_FAIL;
    }

    keep_alive();
    
    status = c_aji_access_dr(
        open_id, length_dr, AJI_DR_UNUSED_0,
        0, write_to_dr?  length_dr : 0, write_bits,
        0, read_from_dr? length_dr : 0, read_bits
    );
/*    
printf("AFTER:  length_dr=%d write_bits=0x%X%X%X%X read_bits=0x%X%X%X%X\n", length_dr,
          write_bits[3],write_bits[2],write_bits[1],write_bits[0], 
          read_bits[3],read_bits[2],read_bits[1],read_bits[0]
    ); 
*/   
    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Failure to access DR register. Return Status is %d (%s)\n",
            status, c_aji_error_decode(status)
        );
	    c_aji_unlock(open_id);
        free(read_bits);
        free(write_bits);
        return ERROR_FAIL;
    }
    /*
    if (read_from_dr) { 
        int size = DIV_ROUND_UP(length_dr, 8);
    	char *value = hexdump(read_bits, size);
        LOG_INFO("DR read   (size=%d, buf=[0x%s]) -> %lu", size, value, (unsigned long) length_dr);
    	free(value);
    } else {
        LOG_INFO("No DR read");
    }
    */ 
    if(read_from_dr) {
        bit_count=0;
	    for (int i = 0; i < num_fields; i++) {
	    	if (fields[i].in_value) {
	    		buf_set_buf(read_bits, bit_count,
	    				    fields[i].in_value, 0, 
	    				    fields[i].num_bits
	            );
	            int size =  (fields[i].num_bits+7)/8;
                char *value = hexdump(fields[i].in_value, size);
                LOG_DEBUG("DR read:  fields[%d].in_value   (size=%d, buf=[0x%s]) -> %u", i, size, value, fields[i].num_bits);
                free(value);
            } else {
                    //LOG_ERROR("  fields[%d].in_value  : <NONE>", i);
	    	}
	    	bit_count += fields[i].num_bits;
	    } 
	} else {
	    LOG_INFO("FINAL: Not reading data from TAP");
	} //end if(read_from_dr) 
    tap_set_state(TAP_IRUPDATE);    
    
    if(TAP_IDLE != state) {
        LOG_WARNING("DR SCAN not yet handle transition to state other than TAP_IDLE(%d). Requested state is %s(%d)", 
            TAP_IDLE, tap_state_name(state), state
        );
assert(0); //deliberately assert() to be able to see where the error is, if it occurs
    }
    
    status = c_aji_run_test_idle(open_id, 2);
    if(AJI_NO_ERROR != status) {
        LOG_WARNING("Failed to go to TAP_IDLE after DR register write");
    }
    c_aji_unlock(open_id);
    free(read_bits);
    free(write_bits);
	return ERROR_OK;
}

int interface_jtag_add_plain_dr_scan(int num_bits, const uint8_t *out_bits,
		uint8_t *in_bits, tap_state_t state)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

    {
    int size = DIV_ROUND_UP(num_bits, 8);
	char *outbits = hexdump(out_bits, size);
    char *inbits  = hexdump(in_bits,  size);
    
	LOG_INFO("  out_bits (size=%d, buf=[0x%s]) -> %u", size, outbits, num_bits);
	LOG_INFO("  in_bits (size=%d, buf=[0x%s]) -> %u", size, inbits, num_bits);
	free(outbits);
	free(inbits);
    }

	/* synchronously do the operation here */

    /* Only needed if we are going to permit SVF or XSVF use */
    LOG_ERROR("No plan to implement interface_jtag_add_plain_dr_scan");
assert(0); //"Need to implement interface_jtag_add_plain_dr_scan");
	return ERROR_FAIL;
}

int interface_jtag_add_tlr()
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

	/* synchronously do the operation here */
	AJI_ERROR status = AJI_NO_ERROR;
	AJI_OPEN_ID open_id = 
        jtagservice.device_open_id_list[jtagservice.in_use_device_tap_position];
        
    status = c_aji_lock(open_id, JTAGSERVICE_TIMEOUT_MS, AJI_PACK_NEVER);
    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot lock device chain. Return status is %d (%s)\n", 
            status, c_aji_error_decode(status)
        );
        return ERROR_FAIL;
    }

    status = c_aji_test_logic_reset(open_id);
    if(AJI_NO_ERROR == status) {
        tap_set_state(TAP_RESET);
    } else {
        LOG_ERROR("Unexpected error setting TAPs to TLR state. Return status is %d (%s)\n", 
            status, c_aji_error_decode(status)
        );
    }

    AJI_ERROR status2 = c_aji_unlock(open_id);
    if(AJI_NO_ERROR != status2) {
        LOG_WARNING("Unexpected error unlocking device chain. Return status is %d (%s)\n", 
            status2, c_aji_error_decode(status2)
        );
    }

	return (status || status2) ? ERROR_FAIL : ERROR_OK;
}

int interface_jtag_add_reset(int req_trst, int req_srst)
{   //LOG_DEBUG("***> IN %s(%d): %s req_trst=%d, req_srst=%d\n", __FILE__, __LINE__, __FUNCTION__, req_trst, req_srst);
	/* synchronously do the operation here */

    LOG_INFO("interface_jtag_add_reset not implemented because we do not need it");
	return ERROR_OK;
}

int interface_jtag_add_runtest(int num_cycles, tap_state_t state)
{   //LOG_DEBUG("***> IN %s(%d): %s num_cycles=%d, end state = (%d) %s\n", __FILE__, __LINE__, __FUNCTION__, num_cycles, state, tap_state_name(state));

	/* synchronously do the operation here */

	AJI_ERROR status = AJI_NO_ERROR;
	AJI_OPEN_ID open_id = 
        jtagservice.device_open_id_list[jtagservice.in_use_device_tap_position];
        
    status = c_aji_lock(open_id, JTAGSERVICE_TIMEOUT_MS, AJI_PACK_NEVER);
    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot lock device chain. Return status is %d (%s)\n",
            status, c_aji_error_decode(status)
        );
        return ERROR_FAIL;
    }
    
    status = c_aji_run_test_idle(open_id, num_cycles);
    if(AJI_NO_ERROR == status) {
        tap_set_state(TAP_IDLE);
    } else {
        LOG_ERROR("Unexpected error setting TAPs to RUN/IDLE state. Return status is %d (%s)\n", 
            status, c_aji_error_decode(status)
        );
    }

    AJI_ERROR status2 = c_aji_unlock(open_id);
    if(AJI_NO_ERROR != status) {
        LOG_WARNING("Unexpected error unlocking device chain. Return status is %d (%s)\n",
            status, c_aji_error_decode(status)
        );
    }

    if(state != TAP_IDLE) {
        LOG_WARNING("Not yet support interface_jtag_add_runtest() to finish in non TAP_IDLE state. "
                    "State %s (%d) requested", tap_state_name(state), state);
    }
	return (status || status2) ? ERROR_FAIL : ERROR_OK;

	return ERROR_OK;
}

int interface_jtag_add_clocks(int num_cycles)
{   LOG_DEBUG("***> IN %s(%d): %s num_cycyles=%d\n", __FILE__, __LINE__, __FUNCTION__, num_cycles);
	/* synchronously do the operation here */

    LOG_ERROR("interface_jtag_add_clocks to be implemented if needed");
assert(0); // "Need to implement interface_jtag_add_clocks"
	return ERROR_OK;
}

int interface_jtag_add_sleep(uint32_t us)
{   LOG_DEBUG("***> IN %s(%d): %s us=%d\n", __FILE__, __LINE__, __FUNCTION__, us);

	/* synchronously do the operation here */
	AJI_ERROR status = AJI_NO_ERROR;
	AJI_OPEN_ID open_id = 
        jtagservice.device_open_id_list[jtagservice.in_use_device_tap_position];
        
    status = c_aji_lock(open_id, JTAGSERVICE_TIMEOUT_MS, AJI_PACK_NEVER);
    if(AJI_NO_ERROR != status) {
        LOG_ERROR("Cannot lock device chain. Return status is %d (%s)\n",
            status, c_aji_error_decode(status)
        );
        return ERROR_FAIL;
    }

    status = c_aji_delay(open_id, us);
    if(AJI_NO_ERROR == status) {
        tap_set_state(TAP_RESET);
    } else {
        LOG_ERROR("Unexpected error trying to sleep for %d. Return status is %d (%s)\n", 
            us, status, c_aji_error_decode(status)
        );
    }

    AJI_ERROR status2 = c_aji_unlock(open_id);
    if(AJI_NO_ERROR != status2) {
        LOG_WARNING("Unexpected error unlocking device chain. Return status is %d (%s)\n",
            status2, c_aji_error_decode(status2)
        );
    }

	return (status || status2) ? ERROR_FAIL : ERROR_OK;
}
int interface_jtag_add_pathmove(int num_states, const tap_state_t *path)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

    { 
    LOG_INFO("Transition from state %s to %s in %d steps. Path:", 
             tap_state_name(cmd_queue_cur_state),
             tap_state_name(path[num_states] -1),
             num_states
    );
    printf("  %s", tap_state_name(cmd_queue_cur_state));
    for (int i=0; i<num_states; ++i) {
        printf(" ->%s", tap_state_name(path[i]));
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
        tms=!!tms; //keep "unused variable tms" error at bay
        
		cur_state = path[state_count];
		state_count++;
		num_states--;
	}


	/* synchronously do the operation here */

    LOG_ERROR("Will implement interface_jtag_add_path_move if we need it");
assert(0); //"Need to implement interface_jtag_add_path_move");

	return ERROR_OK;
}

int interface_add_tms_seq(unsigned num_bits, const uint8_t *seq, enum tap_state state)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
	/* synchronously do the operation here */

    LOG_ERROR("Will implement interface_jtag_add_tms_seq if we need it");
assert(0); //"Need to implement interface_jtag_add_tms_seq");
	return ERROR_OK;
}

COMMAND_HANDLER(jclient_handle_jclient_hardware_command)
{
    if (CMD_ARGC == 1) {
        int count = strlen(CMD_ARGV[0]);

        if (count < 1) {
            command_print(CMD, "Invalid argument: %s.", CMD_ARGV[0]);
            return ERROR_COMMAND_SYNTAX_ERROR;
        }

        jclient_config.hardware_id = calloc(count + 1, sizeof(char));
        strncpy(jclient_config.hardware_id, CMD_ARGV[0], count);
    } else {
        command_print(CMD, "Need exactly one argument for 'jclient hardware'.");
        return ERROR_COMMAND_SYNTAX_ERROR;
    }
    return ERROR_OK;
}

int jim_newtap_hardware(Jim_Nvp* n, Jim_GetOptInfo* goi, struct jtag_tap* pTap) 
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

    const char* hardware_name = NULL;
    int len = 0;
    Jim_GetOpt_String(goi, &hardware_name, &len);
    if (len == 0) {
        Jim_SetResultFormatted(goi->interp, "%s: Missing argument.", n->name);
        return JIM_ERR;
    }

    LOG_INFO("Finding hardware. Expecting %s, candidate hardware %s",
        hardware_name, 
        jclient_config.hardware_name
    );
    /* Currently only expecting and supporting one hardware */
    int namelen = strlen(jclient_config.hardware_name);
    if (strncmp(hardware_name,
                jclient_config.hardware_name, 
                len < namelen ? len :  namelen
    )) {
        Jim_SetResultFormatted(
            goi->interp, 
            "%s: Unknown hardware %s. Expecting %s", 
            n->name, hardware_name, jclient_config.hardware_name);

        //For reason unknwon, not seeing the above error message
        LOG_ERROR("%s: Unknown hardware %s. Expecting %s",
            n->name, hardware_name, jclient_config.hardware_name);
        free((char*)hardware_name);
        return JIM_ERR;
    }

    pTap->hardware = (char*)hardware_name;

    return JIM_OK;
}

static int jim_hardware_cmd(Jim_GetOptInfo* goi)
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);

    /*
     * we expect NAME + ID
     * */
    if (goi->argc < 2) {
        Jim_SetResultFormatted(goi->interp, "Missing NAME ID");
        return JIM_ERR;
    }

    Jim_GetOpt_String(goi, (const char**) (&jclient_config.hardware_name), NULL);
    Jim_GetOpt_String(goi, (const char**) (&jclient_config.hardware_id),   NULL);
    LOG_DEBUG("Creating New hardware, Name: %s, ID %s",
        jclient_config.hardware_name,
        jclient_config.hardware_id
    );

    return JIM_OK;
}

int jclient_jim_jclient_hardware_command(Jim_Interp* interp, int argc, Jim_Obj* const* argv)
{
    LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    Jim_GetOptInfo goi;
    Jim_GetOpt_Setup(&goi, interp, argc - 1, argv + 1);
    return jim_hardware_cmd(&goi);
}
static const struct command_registration jclient_subcommand_handlers[] = {
    {
        .name = "hardware",
        .jim_handler = &jclient_jim_jclient_hardware_command,
//        .handler = &jclient_handle_jclient_hardware_command,
        .mode = COMMAND_CONFIG,
        .help = "select the hardware",
        .usage = "<name> <type>[<port>]",
    },
    COMMAND_REGISTRATION_DONE
};

static const struct command_registration jclient_command_handlers[] = {
    {
        .name = "jclient",
        .mode = COMMAND_CONFIG,
        .help = "Perform jclient management",
        .usage = "",
        .chain = jclient_subcommand_handlers,
    },
    COMMAND_REGISTRATION_DONE
};


static struct jtag_interface jclient_interface = {
	.execute_queue = NULL,
};

struct adapter_driver jclient_adapter_driver = {
	.name = "jclient",
	.transports = jtag_only,
	.commands = jclient_command_handlers,

	.init = jclient_init,
	.quit = jclient_quit,
	.speed = NULL,
	.khz = NULL,
	.speed_div = NULL,
	.power_dropout = NULL,
	.srst_asserted = NULL,

	.jtag_ops = &jclient_interface,
};

