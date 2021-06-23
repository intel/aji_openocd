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

#include <time.h>

#include <jtag/jtag.h>
#include <target/embeddedice.h>
#include <jtag/minidriver.h>
#include <jtag/interface.h>
#include <helper/jep106.h>

#include "jtag/commands.h"
#include "jtag/jtagcore_overwrite.h"
#include "log.h"
#include "server/server.h"

#include "aji/aji.h"
#include "aji/c_jtag_client_gnuaji.h"
#include "jtagservice.h"

extern void jtag_tap_add(struct jtag_tap* t);

// for "aji_client hardware" command
#define HARDWARE_OPT_EXPECTED_ID 1

#define IDCODE_SOCVHPS (0x4BA00477)
#define IDCODE_FE310_G002 (0x20000913)
#define IDCODE_CVA6 (0x000000001)


/**
 * Masking Manufacturer ID bit[12:1] together with
 *  bit[0]. The latter is always 1
 */
#define JTAG_IDCODE_MANUFID_W_ONE_MASK 0x00000FFF 
 /**
  * Altera Manufacturer ID in bit[12:1] with
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
#define JTAG_IDCODE_MANUFID_SIFIVE_W_ONE 0x913
/**
 * OpenHWGroup Manufacturer ID bit[12:1] with
 * bit[0]=1
 * \note We are assuming this is for Ariane Core (CVA6) 
 */
#define JTAG_IDCODE_MANUFID_OPENHWGROUP_W_ONE 0x001


//=================================
// Global variables
//=================================
static struct jtagservice_record jtagservice;

/**
 * Store user configuration/request.
 * @pre At present, function must not require any initialization. It is used before
 *      aji_client_init() is called and I have yet to find a good place to put
 *      the initilaization code if I need it. As it is a
 *      global/static instance, it is automatically initialized to all zero
 *      (or zero equivalent such as NULL). Initialization is guaranteed by C99.
 * @deprecated Use the first jtag_all_hardwares()
 */
struct aji_client_parameters {
	char* hardware_name;
	char* hardware_id; //<AJI compliant hardware descriptor. @see aji_find_hardware
};
typedef struct aji_client_parameters aji_client_parameters;

aji_client_parameters aji_client_config;

AJI_ERROR aji_client_parameters_free(aji_client_parameters* me) {
	if (me->hardware_id) {
		free(me->hardware_id);
		me->hardware_id = NULL;
	}
	if (me->hardware_name) {
		free(me->hardware_name);
		me->hardware_name = NULL;
	}
	return AJI_NO_ERROR;
}


//=================================
// Helpers
//=================================

/*
 * Access functions to lowlevel driver, agnostic of libftdi/libftdxx
 */
static char* hexdump(const uint8_t* buf, const unsigned int size)
{
	unsigned int i;
	char* str = calloc(size * 2 + 1, 1);

	for (i = 0; i < size; i++)
		sprintf(str + 2 * i, "%02x", buf[i]);
	return str;
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
static void jtag_examine_chain_display(enum log_levels level, const char* msg,
	const char* name, uint32_t idcode)
{
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
static bool jtag_examine_chain_match_tap(const struct jtag_tap* tap)
{
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
 */
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
			vtap->dotted_name, (unsigned long)vtap->expected_ids[0],
			status, c_aji_error_decode(status)
		);
		return false;
	}

	LOG_INFO("Virtual Tap/SLD node 0x%08lX found at tap position %lu vtap position %lu",
		(unsigned long)vtap->expected_ids[0], (unsigned long)tap_index, (unsigned long)node_index
	);
	return true;
}


/**
 * Overwrite JTAG Core's jtag_examine_chain function
 * 
 * @pre jtagservice is filled with device inforamtion. That should
 *      already been done during adapter initialization via
 *      @c aji_client_init() function.
 */
int aji_client_jtag_examine_chain(void)
{
	int retval = ERROR_OK;

	struct jtag_tap* tap = jtag_tap_next_enabled(NULL);
	unsigned num_taps = jtagservice.device_count;
	unsigned autocount = 0;
	unsigned t = 0;
	for (t = 0, tap = jtag_tap_next_enabled(NULL);
		t < num_taps;
		++t, tap = jtag_tap_next_enabled(tap)) {
		AJI_DEVICE device = jtagservice.device_list[t];

		if (tap == NULL) {
			tap = calloc(1, sizeof * tap);
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
			LOG_DEBUG("TAP %s does not have valid IDCODE (idcode=0x%08l" PRIx32 ")", tap->dotted_name, (unsigned long)device.device_id);
			tap->hasidcode = false;
			tap->idcode = 0;
		}
		else {
			/* Friendly devices support IDCODE */
			tap->idcode = device.device_id;
			tap->hasidcode = true;
			jtag_examine_chain_display(LOG_LVL_INFO, "tap/device found", tap->dotted_name, tap->idcode);
		}

		/* ensure the TAP ID matches what was expected */
		if (!jtag_examine_chain_match_tap(tap))
			retval = ERROR_JTAG_INIT_SOFT_FAIL;
	} //end for t

	if (AJI_NO_ERROR != retval && ERROR_JTAG_INIT_SOFT_FAIL != retval) {
		return retval;
	}

	for (struct vjtag_tap* vtap = vjtag_all_taps();
		vtap != NULL;
		vtap = (struct vjtag_tap*)vtap->next_tap) {
		vtap->idcode = vtap->expected_ids[0];
		vtap->hasidcode = true;

		if (!vjtag_examine_chain_match_tap(vtap)) {
			vtap->hasidcode = false;
			vtap->idcode = 0;
			retval = ERROR_JTAG_INIT_SOFT_FAIL;
			continue;
		}
	}

	return retval;
}


/*
 * Overwrite JTAG Core's jtag_validate_ircapture function
 * 
 * No need to validate irlen because we are trusting AJI to provide us with
 * the correct value. However, need to produce a message telling
 * the user to add new tap points if the TAP point were autodetected.
 * On exit the scan chain is reset.
 */
int aji_client_jtag_validate_ircapture(void)
{
	for (struct jtag_tap* tap = jtag_tap_next_enabled(NULL);
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
{
	AJI_ERROR status = AJI_NO_ERROR;
	struct jtag_hardware *hardware = jtag_all_hardwares();

	_Bool havent_shown_banner = true;
	DWORD MYJTAGTIMEOUT = 250; //ms
	DWORD TRIES = 4 * 60;
	for (DWORD i = 0; i < TRIES; i++) {
		//currently only one hardware, so it is ok just compare hardware_id
		// to see whether a hardware is specified.
		if (hardware != NULL) {
			if (havent_shown_banner) {
				LOG_INFO("Attempting to find '%s'", hardware->address);
				havent_shown_banner = false;
			}
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
				hardware->address,
				&(jtagservice.hardware_list[0]),
				MYJTAGTIMEOUT
			);
		}
		else {
			if (havent_shown_banner) {
				LOG_INFO("No cable specified, so will be searching for cables");
				havent_shown_banner = false;
			}
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
		} //end if (aji_client_config.hardware_id != NULL)

		if (status != AJI_TIMEOUT)
			break;
		if (i == 2)
			LOG_OUTPUT("Connecting to server(s) [.                   ]"
				"\b\b\b\b\b\b\b\b\b\b"
				"\b\b\b\b\b\b\b\b\b\b");
		else if ((i % 12) == 2)
			LOG_OUTPUT(".");
	} // end for i
	LOG_OUTPUT("\n");


	if (AJI_NO_ERROR != status) {
		LOG_ERROR("Failed to query server for hardware cable information. "
			" Return Status is %i (%s)",
			status, c_aji_error_decode(status)
		);
		return status;
	}
	if (0 == jtagservice.hardware_count) {
		LOG_ERROR("JTAG server reports that it has no hardware cable");
		return AJI_BAD_HARDWARE;
	}

	if (aji_client_config.hardware_id == NULL) {
		LOG_INFO("At present, The first hardware cable will be used"
			" [%lu cable(s) detected]",
			(unsigned long)jtagservice.hardware_count
		);
	}

	AJI_HARDWARE hw = jtagservice.hardware_list[0];
	LOG_INFO("Cable %u: device_name=%s, hw_name=%s, server=%s, port=%s,"
		" chain_id=%p, persistent_id=%lu, chain_type=%d, features=%lu,"
		" server_version_info=%s",
		1, hw.device_name, hw.hw_name, hw.server, hw.port,
		hw.chain_id, (unsigned long)hw.persistent_id,
		hw.chain_type, (unsigned long)hw.features,
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

static AJI_ERROR unread_taps(void);
static AJI_ERROR unselect_cable(void)
{
	AJI_ERROR retval = AJI_NO_ERROR;
	AJI_ERROR status = AJI_NO_ERROR;

	status = unread_taps();
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
 * Read the TAPs on the selected cable.
 * @pre The chain is already acquired, @see select_cable()
 * @pos jtagservice will be populated with the selected tap
 *
 * @sa unread_taps()
 */
static AJI_ERROR read_taps(void)
{
	AJI_ERROR status = AJI_NO_ERROR;

	AJI_HARDWARE hw;
	status = c_aji_find_hardware(jtagservice.in_use_hardware_chain_pid, &hw, JTAGSERVICE_TIMEOUT_MS);
	if (AJI_NO_ERROR != status) {
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
	if (AJI_TOO_MANY_DEVICES == status) {
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

	if (AJI_NO_ERROR != status) {
		LOG_ERROR("Failed to query server for TAP information. "
			" Return Status is %i (%s)",
			status, c_aji_error_decode(status)
		);
		c_aji_unlock_chain(hw.chain_id);
		return status;
	}

	if (0 == jtagservice.device_count) {
		LOG_ERROR("JTAG server reports that it has no TAP attached to the cable");
		c_aji_unlock_chain(hw.chain_id);
		return AJI_NO_DEVICES;
	}

	//SLD discovery
	bool sld_discovery_failed = false;
	jtagservice.hier_id_n = (DWORD*)calloc(jtagservice.device_count, sizeof(DWORD));
	jtagservice.hier_ids = (AJI_HIER_ID**)calloc(jtagservice.device_count, sizeof(AJI_HIER_ID*));
	jtagservice.hub_infos = (AJI_HUB_INFO**)calloc(jtagservice.device_count, sizeof(AJI_HUB_INFO*));
	jtagservice.hier_id_open_id_list = (AJI_OPEN_ID**)calloc(jtagservice.device_count, sizeof(AJI_OPEN_ID*));
	jtagservice.hier_id_type_list = (DEVICE_TYPE**)calloc(jtagservice.device_count, sizeof(DEVICE_TYPE*));
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
			LOG_ERROR("Ran out of memory for jtagservice's tap  %lu's hub_info", (unsigned long)tap_position);
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
				LOG_ERROR("Ran out of memory for jtagservice's tap  %lu's hier_ids", (unsigned long)tap_position);
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
				(unsigned long)tap_position, status, c_aji_error_decode(status)
			);
			sld_discovery_failed = true;
			continue;
		}

		jtagservice.hier_id_open_id_list[tap_position] = \
			(AJI_OPEN_ID*)calloc(jtagservice.hier_id_n[tap_position], sizeof(AJI_OPEN_ID));
		jtagservice.hier_id_type_list[tap_position] = \
			(DEVICE_TYPE*)calloc(jtagservice.hier_id_n[tap_position], sizeof(DEVICE_TYPE));
		if (NULL == jtagservice.hub_infos[tap_position]) {
			LOG_ERROR("Ran out of memory for jtagservice's tap  %lu's hier_ids_claims",
				(unsigned long)tap_position
			);
			return AJI_NO_MEMORY;
		}

		LOG_INFO("TAP position %lu (%lX) has %lu SLD nodes",
			(unsigned long)tap_position,
			(unsigned long)jtagservice.device_list[tap_position].device_id,
			(unsigned long)jtagservice.hier_id_n[tap_position]
		);
		if (jtagservice.hier_id_n[tap_position]) {
			for (DWORD n = 0; n < jtagservice.hier_id_n[tap_position]; ++n) {
				LOG_INFO("    node %2lu idcode=%08lX position_n=%lu",
				    (unsigned long)n,
				    (unsigned long)(jtagservice.hier_ids[tap_position][n].idcode),
				    (unsigned long)(jtagservice.hier_ids[tap_position][n].position_n)
				);
				jtagservice.hier_id_type_list[tap_position][n] = VJTAG; //@TODO Might have to ... 
				                                                        //... replace with  node specific claims
			} //end for(n in jtagservice.hier_id_n[tap_position])
		} //end if (jtagservice.hier_id_n[tap_position])
	} //end for tap_position (SLD discovery)

	if (sld_discovery_failed) {
		LOG_WARNING("Have failures in SLD discovery. See previous log entries. Continuing ...");
	}

	LOG_INFO("Discovered %lx TAP devices", (unsigned long)jtagservice.device_count);
	for (DWORD tap_position = 0; tap_position < jtagservice.device_count; ++tap_position) {
		AJI_DEVICE device = jtagservice.device_list[tap_position];
		LOG_INFO("Detected device (tap_position=%lu) device_id=%08lx,"
			" instruction_length=%d, features=%lu, device_name=%s",
			(unsigned long)tap_position,
			(unsigned long)device.device_id,
			device.instruction_length,
			(unsigned long)device.features,
			device.device_name
		);
		DWORD manufacturer_with_one = device.device_id & JTAG_IDCODE_MANUFID_W_ONE_MASK;

		if (JTAG_IDCODE_MANUFID_ARM_W_ONE == manufacturer_with_one) {
			jtagservice.device_type_list[tap_position] = ARM;
			LOG_INFO("Found a ARM device at tap_position %lu."
				" Currently assume it is JTAG-DP capable",
				(unsigned long) tap_position
			);
			continue;
		}

		if (JTAG_IDCODE_MANUFID_ALTERA_W_ONE == manufacturer_with_one) {
			jtagservice.device_type_list[tap_position] = VJTAG;
			LOG_INFO("Found an Intel device at tap_position %lu."
				"Currently assuming it is SLD Hub", 
				(unsigned long) tap_position
			);
			continue;
		}

		if (JTAG_IDCODE_MANUFID_SIFIVE_W_ONE == manufacturer_with_one) {
			jtagservice.device_type_list[tap_position] = RISCV;
			LOG_INFO("Found SiFive device at tap_position %lu."
				"Currently assuming it is RISCV capable",
				(unsigned long) tap_position
			);
			continue;
		}

		if (JTAG_IDCODE_MANUFID_OPENHWGROUP_W_ONE == manufacturer_with_one) {
			jtagservice.device_type_list[tap_position] = RISCV;
			LOG_INFO("Found OpenHWGroup device at tap_position %lu."
				"Currently assuming it is RISCV capable",
				(unsigned long) tap_position
			);
			continue;
		}
		
		LOG_WARNING("Cannot identify device at tap_position %lu."
			" You will not be able to use it",
			(unsigned long) tap_position
		);
	} //end for tap_position

	return AJI_NO_ERROR;
}


static AJI_ERROR unread_taps(void)
{
	AJI_ERROR status = AJI_NO_ERROR;
	status = jtagservice_free_tap(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
	return status;
}

AJI_ERROR reacquire_open_id(void)
{
	int max_try = 5;
	int sleep_duration = 5; //seconds
	AJI_ERROR status = AJI_NO_ERROR;

	for (int try = 0; try < max_try; ++try) {
		unselect_cable();
		if (try) {
			LOG_INFO("Sleep %d sec before reattempting cable acquisition", sleep_duration);
			sleep(sleep_duration);
		}
		LOG_INFO("Attempt to reacquire cable. Try %d of %d ... ", try + 1, max_try);
		status = select_cable();
		if (AJI_NO_ERROR == status) {
			LOG_INFO(" ... Succeeded");
			break;
		}
		LOG_INFO(" ... Not successful. Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);
	}

	if (AJI_NO_ERROR != status) {
		LOG_INFO("Give Up!");
		return status;
	}

	for (int try = 0; try < max_try; ++try) {
		unread_taps();
		if (try) {
			LOG_INFO("Sleep %d sec before reattempting tap selection", sleep_duration);
			sleep(sleep_duration);
		}
		LOG_INFO("Attempt to select tap. Try %d of %d ... ", try + 1, max_try);
		status = read_taps();
		if (AJI_NO_ERROR == status) {
			LOG_INFO(" ... Succeeded");
			break;
		}
		LOG_INFO(" ... Not successful. Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);
	}

	if (AJI_NO_ERROR != status) {
		LOG_INFO("Give Up!");
		return status;
	}

	assert(UINT32_MAX != jtagservice.in_use_device_tap_position); //quick valid TAP  test
	if (jtagservice.is_sld) {
		assert(UINT32_MAX != jtagservice.in_use_hier_id_node_position); //quick valid hier_id  test
		return jtagservice_activate_virtual_tap(
			&jtagservice,
			0,
			jtagservice.in_use_device_tap_position,
			jtagservice.in_use_hier_id_node_position
		);
	}
	return jtagservice_activate_jtag_tap(
		&jtagservice,
		0,
		jtagservice.in_use_device_tap_position
	);
}

/**
 * Initialize
 *
 * Returns ERROR_OK if JTAG Server found.
 * TODO: Write a TCL Command that allows me to specify the jtagserver config file
 */
static int aji_client_init(void)
{
	char* quartus_jtag_client_config = getenv("QUARTUS_JTAG_CLIENT_CONFIG");
	if (quartus_jtag_client_config != NULL) {
		LOG_INFO("Configuration file, set via QUARTUS_JTAG_CLIENT_CONFIG, is '%s'",
			quartus_jtag_client_config
		);
	}
	else {
		LOG_DEBUG("You can choose to pass a jtag client config file QUARTUS_JTAG_CLIENT_CONFIG");
	}

	jtagservice_init(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
	AJI_ERROR status = AJI_NO_ERROR;

	status = c_jtag_client_gnuaji_init();
	if (AJI_NO_ERROR != status) {
		LOG_ERROR("Cannot initialize C_JTAG_CLIENT library. Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);
		jtagservice_free(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
		aji_client_parameters_free(&aji_client_config);
		return ERROR_JTAG_INIT_FAILED;
	}

	status = select_cable();
	if (AJI_NO_ERROR != status) {
		LOG_ERROR("Cannot select JTAG Cable. Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);
		jtagservice_free(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
		aji_client_parameters_free(&aji_client_config);
		return ERROR_JTAG_INIT_FAILED;
	}

	status = read_taps();
	if (AJI_NO_ERROR != status) {
		LOG_ERROR("Cannot select TAP devices. Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);
		jtagservice_free(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
		aji_client_parameters_free(&aji_client_config);
		return ERROR_JTAG_INIT_FAILED;
	}

	//This driver automanage the tap_state. Hence, this is just
	// to tell the targets which state we think our TAP interface is at.
	tap_set_state(TAP_RESET);

	//overwrites JTAGCORE
	struct jtagcore_overwrite* record = jtagcore_get_overwrite_record();
	record->jtag_examine_chain = aji_client_jtag_examine_chain;
	record->jtag_validate_ircapture = aji_client_jtag_validate_ircapture;

	return ERROR_OK;

}

static int aji_client_quit(void)
{
	struct jtagcore_overwrite* record = jtagcore_get_overwrite_record();
	record->jtag_examine_chain = NULL;
	record->jtag_validate_ircapture = NULL;

	jtagservice_free(&jtagservice, JTAGSERVICE_TIMEOUT_MS);
	aji_client_parameters_free(&aji_client_config);

	c_jtag_client_gnuaji_free();

	return ERROR_OK;
}



//-----------------
// jtag operations
//-----------------

/**
 * Find the next tap used in a jtag_command sequence
 *
 * \param cmds The list of commands to search
 * \param next_tap On output,the next tap if found. 
 *                 if not, no modification to the input value, 
 */
static void find_next_active_tap(
				const struct jtag_command *cmds, 
					  struct jtag_tap **next_tap ) 
{
	for(const struct jtag_command *candidate = cmds; 
								   candidate != NULL; 
								   candidate = candidate->next) {
		if(candidate->type == JTAG_SCAN) {
			(*next_tap) = candidate->cmd.scan->tap;
			return;
		}
	}
}

int aji_client_execute_queue(void)
{
	struct jtag_command *cmd;
	int ret = ERROR_OK;

	struct jtag_tap *active = NULL;
	for (cmd = jtag_command_queue; ret == ERROR_OK && cmd != NULL;
	     cmd = cmd->next) {

		find_next_active_tap(cmd, &active);

		switch (cmd->type) {
		case JTAG_RESET:
			LOG_INFO("Ignoring command JTAG_RESET(trst=%d, srst=%d)\n", 
					 cmd->cmd.reset->trst, 
					 cmd->cmd.reset->srst
			);
			//aji_client_reset(cmd->cmd.reset->trst, cmd->cmd.reset->srst);
			break;
		case JTAG_RUNTEST:
			LOG_ERROR("Not yet coded JTAG_RUNTEST(num_cycles=%d, end_state=0x%x)\n",
					  cmd->cmd.runtest->num_cycles, 
					  cmd->cmd.runtest->end_state
			);
			//aji_client_runtest(
			//	cmd->cmd.runtest->num_cycles,
			//	cmd->cmd.runtest->end_state
			//);
			break;
		case JTAG_STABLECLOCKS:
			LOG_ERROR("Not yet coded JTAG_STABLECLOCKS(num_cycles=%d)\n", 
					  cmd->cmd.stableclocks->num_cycles
			);
			//aji_client_stableclocks(cmd->cmd.stableclocks->num_cycles);
			break;
		case JTAG_TLR_RESET:
			LOG_INFO("JTAG_TLR_RESET(end_state=0x%x,skip_initial_bits=%d)\n", 
					 cmd->cmd.statemove->end_state, 
					 0
			);
			//aji_client_state_move(cmd->cmd.statemove->end_state, 0);
			break;
		case JTAG_PATHMOVE:
			LOG_INFO(
				"Ignoring JTAG_PATHMOVE(numstate=%d, first_state=0x%x, end_state=0x%x)\n",
				cmd->cmd.pathmove->num_states, 
				cmd->cmd.pathmove->path[0], 
				cmd->cmd.pathmove->path[cmd->cmd.pathmove->num_states-1]
			);
			//aji_client_path_move(cmd->cmd.pathmove);
			break;
		case JTAG_TMS:
			LOG_ERROR("Not yet coded JTAG_TMS(num_bits=%d)\n",
					  cmd->cmd.tms->num_bits
			);
			//aji_client_tms(cmd->cmd.tms);
			break;
		case JTAG_SLEEP:
			LOG_ERROR("Not yet coded JTAG_SLEEP(time=%d us)\n", cmd->cmd.sleep->us);
			//aji_client_usleep(cmd->cmd.sleep->us);
			break;
		case JTAG_SCAN:
			LOG_ERROR(
				"Not yet coded JTAG_SCAN(register=%s, num_fields=%d, end_state=0x%x"
                " tap=%p (%s), "
				" num_tap_fields=%d, tap_fields=%p (>%p) )\n",
				cmd->cmd.scan->ir_scan? "IR" : "DR", 
				cmd->cmd.scan->num_fields, 
				cmd->cmd.scan->end_state,
				cmd->cmd.scan->tap,
				jtag_tap_name(cmd->cmd.scan->tap), //cmd->cmd.scan->tap->dotted_name,
				cmd->cmd.scan->tap_num_fields,
				cmd->cmd.scan->tap_fields,
				cmd->cmd.scan->fields
			);
			//ret = aji_client_scan(cmd->cmd.scan);
			break;
		default:
			LOG_ERROR("BUG: unknown JTAG command type 0x%X",
				  cmd->type);
			ret = ERROR_FAIL;
			break;
		}
	}

	return ret;
}


static const struct command_registration aji_client_subcommand_handlers[] = {
	{
		/*
		 * This is a duplication of "hardware" top level command
		 * Prefer the "hardware" top level command over this one,
		 * i.e., "aji_client hardware ..."
		 * The reason this exists is because there are implementation that
		 * relies on "aji_client hardware ..."
		 */
		.name = "hardware",
		.jim_handler = &jim_hardware_newhardware,
		.mode = COMMAND_CONFIG,
		.help = "select the hardware",
		.usage = "<name> <type>[<port>]",
	},
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration aji_client_command_handlers[] = {
	{
		.name = "aji_client",
		.mode = COMMAND_CONFIG,
		.help = "Perform aji_client management",
		.usage = "",
		.chain = aji_client_subcommand_handlers,
	},
	COMMAND_REGISTRATION_DONE
};


static struct jtag_interface aji_client_interface = {
	.execute_queue = aji_client_execute_queue,
};

struct adapter_driver aji_client_adapter_driver = {
	.name = "aji_client",
	.transports = jtag_only,
	.commands = aji_client_command_handlers,

	.init = aji_client_init,
	.quit = aji_client_quit,
	.speed = NULL,
	.khz = NULL,
	.speed_div = NULL,
	.power_dropout = NULL,
	.srst_asserted = NULL,

	.jtag_ops = &aji_client_interface,
};

