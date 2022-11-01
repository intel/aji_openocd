/***************************************************************************
 *   Copyright (C) 2021 by Intel Corporation                               *
 *   author: Ooi, Cinly                                                    *
 *   author-email: cinly.ooi@intel.com                                     *
 *   SPDX-License-Identifier: GPL-2.0-or-later                             *
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

#include <string.h>

#include <jtag/jtag.h>
#include <helper/jep106.h>

#include "log.h"
#include "jtagservice.h"
#include "aji/c_jtag_client_gnuaji.h"

//========================================
//CLAIMS Helper
//========================================

#define IR_ARM_ABORT  0b1000 // dr_len=35
#define IR_ARM_DPACC  0b1010 // dr_len=35
#define IR_ARM_APACC  0b1011 // dr_len=35
#define IR_ARM_IDCODE 0b1110 // dr_len=32
#define IR_ARM_BYPASS 0b1111 // dr_len=1

#define IR_RISCV_BYPASS0   0x00  // dr_len=1
#define IR_RISCV_IDCODE    0x01  // dr_len=32       
#define IR_RISCV_DTMCS     0x10  // dr_len=32
#define IR_RISCV_DMI       0x11  // dr_len=<address_length>+34
#define IR_RISCV_BYPASS    0x1f  // dr_len=1

#define IR_VJTAG_USER0     0xC; // dr_len=<not important>
#define IR_VJTAG_USER1     0xE; // dr_len=<not important>

typedef struct CLAIM_RECORD CLAIM_RECORD;
struct CLAIM_RECORD {
	DWORD claims_n;    ///! number of claims
    AJI_CLAIM2* claims; ///! IR claims
};

#define DEVICE_TYPE_COUNT 4
enum DEVICE_TYPE {
	UNKNOWN = 0, ///! UNKNWON DEVICE
	ARM = 1, ///! ARM device, with IR length = 4 bit
	RISCV = 2, ///! RISCV device, with IR length = 5 bit 
	VJTAG = 3, ///! vJTAG/SLD 
};

// Windows does not like typedef enum
#if PORT!=WINDOWS 
typedef enum DEVICE_TYPE DEVICE_TYPE;
#endif 

typedef struct SLD_RECORD SLD_RECORD;
struct SLD_RECORD {
	DWORD idcode;
	DWORD node_position;
};
/**
 * Create the claim records. One for each device_type
 * @param records On input, an array of size records_n. 
 *                On output, array will be filled with claims information. 
 *                index = @see JTAGSERVE_DEVICE_TYPE
 * @param records_n On input, size of records. On output, if return AJI_TOO_MANY_CLAIMS,
 *                  will contain the required size to host all claims
 * @return AJI_NO_ERROR if there is no error. 
 *         AJI_TOO_MANY_CLAIMS if the records array is too small
 *         AJI_NO_MEMORY if run out of memory
 * @note Memory allocated here freed as part of @jtagservice_free()
 */
AJI_ERROR jtagservice_create_claim_records(CLAIM_RECORD *records, DWORD * records_n) {
	if (UNKNOWN < *records_n) {
		DWORD csize = 0;
        AJI_CLAIM2 *claims = (AJI_CLAIM2*) calloc(csize, sizeof(AJI_CLAIM2)); //It's empty, NULL perhaps?
		if (claims == NULL) {
			return AJI_NO_MEMORY;
		}
	}
	if (ARM < *records_n) {
		DWORD csize = 4;
        AJI_CLAIM2* claims = (AJI_CLAIM2*) calloc(csize, sizeof(AJI_CLAIM2));

		if (claims == NULL) {
			return AJI_NO_MEMORY;
		}

		claims[0].type = AJI_CLAIM_IR_SHARED;
		claims[0].value = IR_ARM_IDCODE;
		claims[1].type = AJI_CLAIM_IR_SHARED;
		claims[1].value = IR_ARM_DPACC;
		claims[2].type = AJI_CLAIM_IR_SHARED;
		claims[2].value = IR_ARM_APACC;
		claims[3].type = AJI_CLAIM_IR_SHARED;
		claims[3].value = IR_ARM_ABORT;

		records[ARM].claims_n = csize;
		records[ARM].claims = claims;
	}

	if (RISCV < *records_n) {
		DWORD csize = 3;
		AJI_CLAIM2 *claims = (AJI_CLAIM2*) calloc(csize, sizeof(AJI_CLAIM2));
		if (claims == NULL) {
			return AJI_NO_MEMORY;
		}

		claims[0].type = AJI_CLAIM_IR_SHARED;
		claims[0].value = IR_RISCV_IDCODE;
		claims[1].type = AJI_CLAIM_IR_SHARED;
		claims[1].value = IR_RISCV_DTMCS;
		claims[2].type = AJI_CLAIM_IR_SHARED;
		claims[2].value = IR_RISCV_DMI;

		records[RISCV].claims_n = csize;
		records[RISCV].claims = claims;
	}

	if (VJTAG < *records_n) {
		DWORD csize = 4;
        AJI_CLAIM2 *claims = (AJI_CLAIM2*) calloc(csize, sizeof(AJI_CLAIM2));
		if (claims == NULL) {
			return AJI_NO_MEMORY;
		}
		//NOTE: HARDCODED TO
		claims[0].type = AJI_CLAIM_IR_SHARED_OVERLAY;
		claims[0].value = IR_VJTAG_USER1;
		claims[1].type = AJI_CLAIM_IR_SHARED_OVERLAID;
		claims[1].value = IR_VJTAG_USER0;
		claims[2].type = AJI_CLAIM_OVERLAY_SHARED;
		claims[2].value = IR_RISCV_DTMCS; 
		claims[3].type = AJI_CLAIM_OVERLAY_SHARED;
		claims[3].value = IR_RISCV_DMI;

		records[VJTAG].claims_n = csize;
		records[VJTAG].claims = claims;
	}

	AJI_ERROR status = AJI_NO_ERROR;
	if (*records_n < DEVICE_TYPE_COUNT) {
		status = AJI_TOO_MANY_CLAIMS;
		*records_n = DEVICE_TYPE_COUNT;
	}
	return status;
}



//==============================================
// Supported TAPs
//==============================================
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


//==============================================
// Global variable
//==============================================

struct jtagservice_record {
	char *appIdentifier;

	DWORD claims_count;
	CLAIM_RECORD *claims; ///! List of AJI_CLAIM2, by DEVICE_TYPE

	//data members
	//() Cable
	/* Not sure AJI_HARDWARE can survive function boundarie
	 * so store its persistent ID as backup.
	 * If proven that AJI_HARDWARE can work, then keep it.
	 */
	DWORD          hardware_count; 
	AJI_HARDWARE  *hardware_list;
	char         **server_version_info_list;
	
	//DWORD         in_use_hardware_index; 
	//< Current detection system does not allow this to be determined
	AJI_HARDWARE *in_use_hardware;
	DWORD         in_use_hardware_chain_pid;
	AJI_CHAIN_ID  in_use_hardware_chain_id;

	//() Tap device
	DWORD        device_count; //< Number of devices, Range is [0, \c UINT32_MAX)
	AJI_DEVICE  *device_list;
	AJI_OPEN_ID *device_open_id_list;
	DEVICE_TYPE *device_type_list;

	//SLD / Virtual JTAG
	DWORD* hier_id_n; //< How many SLD node per TAP device.
				       //< size = device_count since one number
				       //< per TAP (device), in the same order 
				       //< as device_list
	AJI_HIER_ID** hier_ids; //< hier_ids[TAP][SLD] where TAP =
				            //< [0, device_count), in the same  
				            //< order as device_list,
				            //< and SLD = [0, hier_id_n[TAP])
	AJI_HUB_INFO** hub_infos; //< hub_infos[TAP][HUB] where 
				      //< TAP = [0, device_count), in the same  
				      //< order as device_list, and hub =
				      //< [0, AJI_MAX_HIERARCHICAL_HUB_DEPTH)
	AJI_OPEN_ID**  hier_id_open_id_list; //< 
				            //<hier_ids[TAP][SLD] where TAP =
				            //< [0, device_count), in the same  
				            //< order as device_list,
				            //< and SLD = [0, hier_id_n[TAP])
	DEVICE_TYPE** hier_id_type_list; //< 
				        //< hier_ids_device_type[TAP][SLD] 
				        //< where TAP = [0, device_count),   
				        //< in the same order as device_list,
				        //< and SLD = [0, hier_id_n[TAP])

	//Active Tap - Implied locked
	DWORD  in_use_device_tap_position; //< index of the active tap.
				                       //< value can be [0, device_count) or
				                       //< UINT32_MAX if position is invalid
	AJI_DEVICE*  in_use_device;
	DWORD        in_use_device_id;
	BYTE         in_use_device_irlen;

	AJI_OPEN_ID* in_use_open_id; //< The #AJI_OPEN_ID in use. 
								 //< Can be for TAP (device) or SLD Node

	//SLD Node
	//Additional information needed if
	//  SLD node was selected
	bool         is_sld;
	DWORD        in_use_hier_id_node_position;
	AJI_HIER_ID* in_use_hier_id;
	DWORD        in_use_hier_id_idcode;
};
static struct jtagservice_record jtagservice;


//=====================================
// JTAG TAP management service
//=====================================
AJI_OPEN_ID jtagservice_get_in_use_open_id() 
{
	return jtagservice.in_use_open_id;
}

static AJI_ERROR jtagservice_device_index_by_idcode(
	const DWORD idcode,
	DWORD* tap_index) 
{
	assert(jtagservice.device_count != UINT32_MAX);

	DWORD index = 0;
	const DWORD tap_count = jtagservice.device_count;
	for (; index < tap_count; ++index) {
		if (jtagservice.device_list[index].device_id == idcode) {
			break;
		}
	}
	
	if (index == tap_count) {
		return AJI_FAILURE;
	}
	*tap_index = index;
	return AJI_NO_ERROR;
}

static AJI_ERROR jtagservice_hier_id_index_by_idcode(
	const DWORD idcode,
	const DWORD tap_index,
	DWORD* hier_index)
{

	AJI_HIER_ID *hier_id_list = jtagservice.hier_ids[tap_index];
	DWORD hier_id_count = jtagservice.hier_id_n[tap_index];

	DWORD index = 0;
	for (; index < hier_id_count; ++index) {
		if (hier_id_list[index].idcode == idcode) {
			break;
		}
	}

	if (index == hier_id_count) {
		return AJI_FAILURE;
	}
	*hier_index = index;
	return AJI_NO_ERROR;
}

/**
 * Check that \c tap_index is in range
 *
 * \param hardware_index Not yet used, set to zero
 * \param tap_index The index number to check against
 *
 * \return #AJI_NO_ERROR \c tap_index is in range
 * \return #AJI_FAILURE  \c tap_index is not in range. An entry will be logged via #LOG_ERROR
 *
 * \sa #jtagservice_validate_virtual_tap_index
 *
 * \note If you can, consider only testing for <tt>tap_index != UINT32_MAX</tt>
 */
static AJI_ERROR jtagservice_validate_tap_index(const DWORD hardware_index, const DWORD tap_index){
	if(tap_index == UINT32_MAX) {
		return AJI_NO_ERROR;
	}

	if(tap_index < jtagservice.device_count) {
		return AJI_NO_ERROR;
	}

	LOG_ERROR("Bad TAP index, i.e. %lu not in [0, %lu)", (unsigned long) tap_index, (unsigned long) jtagservice.device_count);
	return AJI_FAILURE;
}


/**
 * Check that \c node_index is in range.
 *
 * \param hardware_index Not yet used, set to zero
 * \param tap_index The index number to check against
 * \param node_index The SLD node index to check against
 *
 * \return #AJI_NO_ERROR \c tap_index is in range
 * \return #AJI_FAILURE  \c tap_index is not in range. An entry will be logged via #LOG_ERROR
 *
 * \sa #jtagservice_validate_tap_index
 *
 * \note If you can, consider testing for  <tt>tap_index != UINT32_MAX &7 node_index != UINT32_MAX</tt>
 *       or just <tt>node_index != UINT32_MAX</tt> alone
 */
AJI_ERROR jtagservice_validate_virtual_tap_index(
	const DWORD hardware_index,
	const DWORD tap_index,
	const DWORD node_index) {
	if (AJI_NO_ERROR != jtagservice_validate_tap_index(hardware_index, tap_index)) {
		return AJI_FAILURE;
	}

	if (node_index == UINT32_MAX) {
		LOG_ERROR("SLD node was not yet assigned");
		return AJI_FAILURE;
	}

	if (tap_index < jtagservice.hier_id_n[tap_index]) {
		return AJI_NO_ERROR;
	}

	LOG_ERROR("Bad Node index, i.e. %lu not in [0, %lu)", (unsigned long) tap_index, (unsigned long) jtagservice.hier_id_n[tap_index]);
	return AJI_FAILURE;
}


/**
 * Update the active tap information
 *
 * \param hardware Not yet used. Set to 0
 * \param tap_index The new active tap, or UINT32_MAX if no longer have an active TAP
 * \param is_sld Are we activating a SLD node?
 * \param node_index The node_index to activate, if \c is_sld is set to true
 *
 * \return AJI_NO_ERROR if successful
 * \return AJI_FAILURE Failed
 */
static AJI_ERROR jtagservice_update_active_tap_record(
						const DWORD hardware_index,
						const DWORD tap_index,
						const bool is_sld, 
						const DWORD node_index) 
{
//LOG_INFO("***> %s:%d:%s: BEGIN, tap_index=%lu is_sld=%s node_index=%lu", 
//			__FILE__, __LINE__, __FUNCTION__, 
//			(unsigned long) tap_index, 
//			is_sld? "yes" : "no", 
//			(unsigned long) node_index
//);
	//TODO: consider not validating and assuming all parameter passed to 
    // this function is validated to prevent redundant validation
	AJI_ERROR status = jtagservice_validate_tap_index(0, tap_index);
	if (AJI_NO_ERROR != status) {
		LOG_DEBUG("Supplied an in valid tap_index (%lu), expecting [0, %lu)", 
			(unsigned long) tap_index, 
			(unsigned long) jtagservice.device_count
		);
		return AJI_FAILURE;
	}
	if (is_sld) {
		status = jtagservice_validate_virtual_tap_index(0, tap_index, node_index);
		if (AJI_NO_ERROR != status) {
			LOG_DEBUG("Supplied an in valid node_index (%lu), expecting [0, %lu)", 
				(unsigned long) node_index, 
				(unsigned long) jtagservice.hier_id_n[tap_index]
			);
			return AJI_FAILURE;
		}
	}

	if(UINT32_MAX == tap_index) {
		jtagservice.in_use_device = NULL;
		jtagservice.in_use_device_id = 0;
		jtagservice.in_use_device_tap_position = UINT32_MAX;
		jtagservice.in_use_device_irlen = 0;

		jtagservice.in_use_open_id = 0;
	} else {
		AJI_DEVICE device = jtagservice.device_list[tap_index];
		jtagservice.in_use_device = &(jtagservice.device_list[tap_index]);
		jtagservice.in_use_device_id = device.device_id;
		jtagservice.in_use_device_tap_position = tap_index;
		jtagservice.in_use_device_irlen = device.instruction_length;

		jtagservice.in_use_open_id = jtagservice.device_open_id_list[tap_index];
	}

	jtagservice.is_sld = is_sld;
	if (jtagservice.is_sld) {
		jtagservice.in_use_hier_id_node_position = node_index;
		jtagservice.in_use_hier_id = &(jtagservice.hier_ids[tap_index][node_index]);
		jtagservice.in_use_hier_id_idcode = jtagservice.hier_ids[tap_index][node_index].idcode;

		jtagservice.in_use_open_id = jtagservice.hier_id_open_id_list[tap_index][node_index];
	} else {
		jtagservice.in_use_hier_id_node_position = UINT32_MAX;
		jtagservice.in_use_hier_id = NULL;
		jtagservice.in_use_hier_id_idcode = 0;
	}

	return AJI_NO_ERROR;
}


/**
 * Unlock
 */
AJI_ERROR jtagservice_unlock()
{
	if(UINT32_MAX == jtagservice.in_use_device_tap_position) {
		return AJI_NO_ERROR; //nothing to unlock
	}

	assert(jtagservice.in_use_device_tap_position < jtagservice.device_count);

	AJI_ERROR status = AJI_NO_ERROR;
	status = c_aji_unlock(jtagservice.in_use_open_id);
	if(AJI_NO_ERROR != status) {
		LOG_WARNING("Cannot unlock tap %lu idcode=%lX. Returned %d (%s)",
					(unsigned long) jtagservice.in_use_device_tap_position, 
					(unsigned long) jtagservice.in_use_device_id,
					status, 
					c_aji_error_decode(status)
		);
	}
	status = jtagservice_update_active_tap_record(0, (unsigned long) UINT32_MAX, false, UINT32_MAX);
	return AJI_NO_ERROR;
}


/**
 * Lock Tap
 * \param hardware_index Not yet in use, set to zero.
 * \note Use #jtagservice_unlock() to unlock
 */
static AJI_ERROR jtagservice_lock_jtag_tap (
	const DWORD hardware_index,
	const DWORD tap_index ) 
{
	assert(hardware_index == 0);

	//assert(tap_index != UINT32_MAX);
    if(tap_index == UINT32_MAX) {
        LOG_ERROR("JTAG TAP node does not exists");
        return AJI_BAD_TAP_POSITION;
    }

	//assert(tap_index < jtagservice.device_count);
	if(jtagservice.device_count <= tap_index) {
        LOG_ERROR("Bad JTAG TAP position (requested tap %lu not in [0,%lu)",
                  (unsigned long) tap_index, (unsigned long) jtagservice.device_count
		);
        return AJI_BAD_TAP_POSITION;
    }

	if(    !jtagservice.is_sld 
		&& tap_index == jtagservice.in_use_device_tap_position
	) {
		return AJI_NO_ERROR; //tap_index is the currenly locked device.
	}

	if(UINT32_MAX != jtagservice.in_use_device_tap_position) {
		jtagservice_unlock();
	}

	AJI_ERROR status = AJI_NO_ERROR;
	
	if (!jtagservice.device_type_list[tap_index]) {
		LOG_ERROR("Cannot activate tap #%lu idcode=0x%lX because we don't know what type it is",
			(unsigned long) tap_index, 
			(unsigned long) jtagservice.device_list[tap_index].device_id
		);
		//@TODO: Do a type lookup instead.
		return AJI_FAILURE;
	}

	if (!jtagservice.device_open_id_list[tap_index]) {
		LOG_DEBUG("Acquiring OPEN ID for tap %lu idcode=0x%08lX", 
			(unsigned long) tap_index, 
			(unsigned long) jtagservice.device_list[tap_index].device_id
		);

		status = c_aji_lock_chain(jtagservice.in_use_hardware_chain_id, 
				                  JTAGSERVICE_TIMEOUT_MS
		);
		if( !(AJI_NO_ERROR == status || AJI_LOCKED == status) ) {
			 LOG_ERROR("Cannot lock chain. Returned %d (%s)", 
				  status, c_aji_error_decode(status)
			 );
			 return status;
		}

        status = c_aji_open_device_a(
            jtagservice.in_use_hardware_chain_id,
            tap_index,
            &(jtagservice.device_open_id_list[tap_index]),
            (const AJI_CLAIM2*) (jtagservice.claims[jtagservice.device_type_list[tap_index]].claims), 
            jtagservice.claims[jtagservice.device_type_list[tap_index]].claims_n, 
            jtagservice.appIdentifier
        );

		if(AJI_NO_ERROR !=  status ) { 
			 LOG_ERROR("Problem openning tap %lu (0x%08lX). Returned %d (%s)", 
				  (unsigned long) tap_index, 
				  (unsigned long) jtagservice.device_list[tap_index].device_id, 
				  status, 
				  c_aji_error_decode(status)
			 );
			 return status;
		}
		status = c_aji_unlock_chain(jtagservice.in_use_hardware_chain_id);
		if(AJI_NO_ERROR !=  status ) { 
			 LOG_ERROR("Cannot unlock chain. Returned %d (%s)", 
						status, 
						c_aji_error_decode(status)
			 );
			 return status;
		}
	} //end if (!jtagservice.device_open_id_list[tap_index]) 

	status = c_aji_lock(jtagservice.device_open_id_list[tap_index], JTAGSERVICE_TIMEOUT_MS, AJI_PACK_AUTO);
	if(!(AJI_NO_ERROR ==  status || AJI_LOCKED == status)) { 
		 LOG_ERROR("Cannot lock tap %lu idcode=0x%08lX. Returned %d (%s)", 
					(unsigned long) tap_index, 
					(unsigned long) jtagservice.device_list[tap_index].device_id,
					status, 
					c_aji_error_decode(status)
		 );
		 return status;
	}
	
	status = jtagservice_update_active_tap_record(hardware_index, (unsigned long) tap_index, false, UINT32_MAX);
	return status;
}

/**
 * Activate Virtual Tap
 * \param hardware_index Not yet in use, set to zero.
 */
AJI_ERROR jtagservice_lock_virtual_tap(
	const DWORD hardware_index,
	const DWORD tap_index,
	const DWORD node_index     ) {

	assert(hardware_index == 0);

    if(tap_index == UINT32_MAX) {
        LOG_ERROR("JTAG TAP node for vJTAG node does not exists");
        return AJI_BAD_TAP_POSITION;
    }

	if(jtagservice.device_count <= tap_index) {
        LOG_ERROR("Bad JTAG TAP position of vJTAG node (requested tap %lu not in [0,%lu)",
                  (unsigned long) tap_index, (unsigned long) jtagservice.device_count
		);
        return AJI_BAD_TAP_POSITION;
    }

    if(node_index == UINT32_MAX) {
        LOG_ERROR("vJTAG node does not exists");
        return AJI_NO_MATCHING_NODES;
    }

    if(jtagservice.hier_id_n[tap_index] <= node_index) {
        LOG_ERROR("Bad  vJTAG node position (requested node %lu not in [0,%lu)",
                  (unsigned long) node_index, (unsigned long) jtagservice.hier_id_n[tap_index]
		);
        return AJI_NO_MATCHING_NODES;
    }

	if(		jtagservice.is_sld 
		&&	tap_index == jtagservice.in_use_device_tap_position
		&&  node_index == jtagservice.in_use_hier_id_node_position
	) {
		return AJI_NO_ERROR; //node_index is the currenly locked device.
	}

	if(UINT32_MAX != jtagservice.in_use_device_tap_position) {
		jtagservice_unlock();
	}

	AJI_ERROR status = AJI_NO_ERROR;
	if (!jtagservice.hier_id_type_list[tap_index][node_index]) {
		LOG_WARNING(
			"Unknown device type for  SLD #%lu idcode=0x%08lX, Tap #%lu idcode=0x%08lX",
			(unsigned long)node_index, (unsigned long)jtagservice.hier_ids[tap_index][node_index].idcode,
			(unsigned long)tap_index, (unsigned long)jtagservice.device_list[tap_index].device_id
		);
		
		//@TODO: Do a type lookup instead.
		return AJI_FAILURE;
			
	}

	if (!jtagservice.hier_id_open_id_list[tap_index][node_index]) {
		LOG_DEBUG("Getting OPEN ID for SLD #%lu idcode=0x%08lX, Tap #%lu idcode=0x%08lX",
			(unsigned long)node_index, (unsigned long)jtagservice.hier_ids[tap_index][node_index].idcode,
			(unsigned long)tap_index, (unsigned long)jtagservice.device_list[tap_index].device_id
		);

		status = c_aji_lock_chain( 
			jtagservice.in_use_hardware_chain_id,
			JTAGSERVICE_TIMEOUT_MS
		);
		
		if (AJI_NO_ERROR != status && AJI_LOCKED != status) {
		   LOG_ERROR("Cannot lock chain. Returned %d (%s)",
				     status, c_aji_error_decode(status)
		   );
		   return status;
		}

        status = c_aji_open_node_b(
            jtagservice.in_use_hardware_chain_id,
            tap_index,
            &(jtagservice.hier_ids[tap_index][node_index]),
            &(jtagservice.hier_id_open_id_list[tap_index][node_index]),
            (const AJI_CLAIM2*)(jtagservice.claims[jtagservice.hier_id_type_list[tap_index][node_index]].claims),
            jtagservice.claims[jtagservice.hier_id_type_list[tap_index][node_index]].claims_n,
            jtagservice.appIdentifier
        );

		if (AJI_NO_ERROR != status) {
			LOG_ERROR("Problem openning node %lu (0x%08lX) for tap position %lu (0x%08lX). Returned %d (%s)",
						(unsigned long) node_index,
						(unsigned long) jtagservice.hier_ids[tap_index][node_index].idcode,
						(unsigned long) tap_index,
						(unsigned long) jtagservice.device_list[tap_index].device_id,
						status,
						c_aji_error_decode(status)
			);
			return status;
		}

		status = c_aji_unlock_chain(jtagservice.in_use_hardware_chain_id);
		if (AJI_NO_ERROR != status) {
			LOG_ERROR("Cannot unlock chain. Returned %d (%s)",
				      status, c_aji_error_decode(status)
			);
			return status;
		}
	} //end if(!jtagservice.hier_id_open_id_list[tap_index][node_index]) 

	status = c_aji_lock(jtagservice.hier_id_open_id_list[tap_index][node_index], 
						JTAGSERVICE_TIMEOUT_MS, 
						AJI_PACK_AUTO
	);
	if(!(AJI_NO_ERROR ==  status || AJI_LOCKED == status)) { 
		 LOG_ERROR("Cannot lock virtual tap node %lu (0x%08lX) for "
						"tap position %lu (0x%08lX). Returned %d (%s)", 
						(unsigned long) node_index,
						(unsigned long) jtagservice.hier_ids[tap_index][node_index].idcode,
						(unsigned long) tap_index, 
						(unsigned long) jtagservice.device_list[tap_index].device_id,
						status, 
						c_aji_error_decode(status)
		 );
		 return status;
	}


	status = jtagservice_update_active_tap_record(
				hardware_index, 
				tap_index, 
				true, 
				node_index
	);
	return status;
}

/**
 * Lock any TAP
 * 
 * Designed for situation where a JTAG operation is needed and no TAP has been specifed yet.
 * Examples are JTAG state transition. These function calls do not specify a TAP as it 
 * is not needed: The call affect the whole JTAG Scan Chain and in OpenOCD, there is only one 
 * scan chain.
 * 
 * However, AJI operations required a TAP to be specified because AJI supports multiple scan
 * chain and the TAP is used to identify the scan chain to operate on.
 *
 * In situation like this, one cannot just simply select any TAP. The reason is AJI requires
 * us to setup #AJI_CLAIM  for the TAP to activate it, but not all TAPs' <tt>AJI_CLAIM</tt>s 
 * are known. This function sidestep this by activating any TAP that can be activated, 
 *
 * \return #AJI_NO_ERROR Success.
 * \return #AJI_FAILURE Cannot find any TAP that can be activated
 * 
 * \post If return \c AJI_NO_ERROR \c me is updated with information about the activated TAP.
 */
static AJI_ERROR jtagservice_lock_any_tap(void)
{
	if(jtagservice.in_use_device_tap_position != UINT32_MAX) {
		return AJI_NO_ERROR; //already locked something, so just return
	} 

	DWORD hardware_index = 0; //Not in use yet, set to zero

	for (DWORD tap = 0; tap < jtagservice.device_count; ++tap) {
		AJI_ERROR status = jtagservice_lock_jtag_tap(hardware_index, tap);
		if (AJI_NO_ERROR == status) {
			return AJI_NO_ERROR;
		}
	}
	LOG_ERROR("Cannot activate any tap on hardware %lu (device_count = %lu)",
		(unsigned long) hardware_index,
		(unsigned long)jtagservice.device_count
	);
	return AJI_FAILURE;
}


/**
 * Lock the required \c tap.
 *
 * \param tap The Tap to activate. If NULL, will activate any tap
 *            it can find.
 *
 * \return #AJI_NO_ERROR if \c tap or any tap if tap is NULL,
 *                is activated
 * \return ERROR_CODE An error had occured. No TAP activated.
 *                Look up the error code. Depending on the error
 *                code, the tap that was previously
 *                locked may had been unlocked.
 *
 * \post If necessary, the lock on the \c tap from previous call
 *            to this function will be unlocked.
 * \see #jtagservice_unlock()
 */
AJI_ERROR jtagservice_lock(const struct jtag_tap* const tap)
{
	if(tap && jtag_tap_on_all_vtaps_list(tap)) {
		DWORD tap_index = UINT32_MAX;
		jtagservice_device_index_by_idcode(
			((struct vjtag_tap*) tap)->parent->idcode,
			&tap_index
		); //Not checking return status because it should pass

		DWORD sld_index = UINT32_MAX;
		jtagservice_hier_id_index_by_idcode(
			tap->idcode,
			tap_index,
			&sld_index
		); //Not checking return status because it should pass

		return jtagservice_lock_virtual_tap(0, tap_index, sld_index);
	}

	if(tap) {
		DWORD tap_index = UINT32_MAX;
		jtagservice_device_index_by_idcode(
			tap->idcode,
			&tap_index
		); //Not checking return status because it should pass

		return jtagservice_lock_jtag_tap(0, tap_index);
	}
	return jtagservice_lock_any_tap(); //hardware index is not used and set to zero
}



//========================================
// JTAG Core overwrites
//========================================

//---
// Bits that have to be implemented/copied from src/jtag/core.c
//---


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
		tap_index,
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
int jtagservice_jtag_examine_chain(void)
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
int jtagservice_jtag_validate_ircapture(void)
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



//=====================================
// Init/free
//=====================================

static AJI_ERROR  jtagservice_free_cable(const DWORD timeout)
{   
	if(jtagservice.in_use_hardware_chain_id) {
		c_aji_unlock_chain(jtagservice.in_use_hardware_chain_id); //TODO: Make all lock/unlock self-contained then remove this
		jtagservice.in_use_hardware_chain_id = NULL;
	}

	if (jtagservice.hardware_count != 0) {
		free(jtagservice.hardware_list);
		free(jtagservice.server_version_info_list);

		jtagservice.hardware_list = NULL;
		jtagservice.server_version_info_list = NULL;

		//jtagservice.in_use_hardware_index = 0;
		jtagservice.in_use_hardware = NULL;
		jtagservice.in_use_hardware_chain_pid = 0;
		jtagservice.in_use_hardware_chain_id = NULL;

		jtagservice.hardware_count = 0;
	}
	return AJI_NO_ERROR;
}

static AJI_ERROR  jtagservice_free_common(const DWORD timeout)
{   
	if (jtagservice.claims_count) {
		for (DWORD i = 0; i < jtagservice.claims_count; ++i) {
			if (0 != jtagservice.claims[i].claims_n) {
				free(jtagservice.claims[i].claims);
				jtagservice.claims[i].claims = NULL;
				jtagservice.claims[i].claims_n = 0;
			}
		}

		jtagservice.claims_count = 0;
	}

	if (jtagservice.appIdentifier) {
		free(jtagservice.appIdentifier);
		jtagservice.appIdentifier = NULL;
	}

	return AJI_NO_ERROR;
}

static AJI_ERROR  jtagservice_free_tap(const DWORD timeout)
{
	if (jtagservice.hier_id_n) {
		for (DWORD i = 0; i < jtagservice.device_count; ++i) {
			free(jtagservice.hier_ids[i]);
			free(jtagservice.hub_infos[i]);
			
			free(jtagservice.hier_id_open_id_list[i]);
			free(jtagservice.hier_id_type_list[i]);
		}
		free(jtagservice.hier_ids);
		free(jtagservice.hub_infos);
		free(jtagservice.hier_id_open_id_list);
		free(jtagservice.hier_id_type_list);

		jtagservice.hier_ids = NULL;
		jtagservice.hub_infos = NULL;
		jtagservice.hier_id_open_id_list = NULL;
		jtagservice.hier_id_type_list = NULL;
	
		jtagservice.hier_id_n = 0;
	}

	if (jtagservice.device_count != 0) {
		free(jtagservice.device_list);
		free(jtagservice.device_open_id_list);
		free(jtagservice.device_type_list);

		jtagservice.device_list = NULL;
		jtagservice.device_open_id_list = NULL;
		jtagservice.device_type_list = NULL;

		jtagservice.device_count = 0;
	}

	jtagservice.in_use_device_tap_position = UINT32_MAX;
	jtagservice.in_use_device = NULL;
	jtagservice.in_use_device_id = 0;
	jtagservice.in_use_device_irlen = 0;

	jtagservice.in_use_open_id = 0;

	jtagservice.is_sld = false;
	jtagservice.in_use_hier_id_node_position = UINT32_MAX;
	jtagservice.in_use_hier_id = NULL;
	jtagservice.in_use_hier_id_idcode = 0;

	return AJI_NO_ERROR;
}

AJI_ERROR  jtagservice_free(DWORD timeout) 
{
	AJI_ERROR retval = AJI_NO_ERROR;
	AJI_ERROR status = AJI_NO_ERROR;
	status = jtagservice_free_tap(timeout);
	if (status) {
		retval = status;
	}

	status = jtagservice_free_cable(timeout);
	if (status) {
		retval = status;
	}
	status = jtagservice_free_common(timeout);
	if (status) {
		retval = status;
	}

	return status;
}


static AJI_ERROR jtagservice_init_tap(DWORD timeout) {
	jtagservice.device_count = 0;
	jtagservice.device_list = NULL;
	jtagservice.device_open_id_list = NULL;
	jtagservice.device_type_list = NULL;

	jtagservice.hier_id_n = 0;
	jtagservice.hier_ids = NULL;
	jtagservice.hub_infos = NULL;
	jtagservice.hier_id_open_id_list = NULL;
	jtagservice.hier_id_type_list = NULL;

	jtagservice.in_use_device_tap_position = UINT32_MAX;
	jtagservice.in_use_device = NULL;
	jtagservice.in_use_device_id = 0;
	jtagservice.in_use_device_irlen = 0;

	jtagservice.is_sld = false;
	jtagservice.in_use_hier_id_node_position = -1;
	jtagservice.in_use_hier_id = NULL;
	jtagservice.in_use_hier_id_idcode = 0;

	return AJI_NO_ERROR;
}

static AJI_ERROR jtagservice_init_cable(DWORD timeout) {
	jtagservice.hardware_count = 0;
	jtagservice.hardware_list = NULL;
	jtagservice.server_version_info_list = NULL;

	jtagservice.in_use_hardware = NULL;
	jtagservice.in_use_hardware_chain_pid = 0;
	jtagservice.in_use_hardware_chain_id = NULL;

	return AJI_NO_ERROR;
}

static AJI_ERROR jtagservice_init_common(DWORD timeout) {
	jtagservice.appIdentifier = (char*) calloc(28, sizeof(char));
	//jtagservice.in_use_hardware_index = 0;
	if (NULL == jtagservice.appIdentifier) {
		return AJI_NO_MEMORY;
	}

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(jtagservice.appIdentifier, "OpenOCD.%4d%02d%02d%02d%02d%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec
	);
	LOG_INFO("Application name is %s", jtagservice.appIdentifier);

	jtagservice.claims_count = DEVICE_TYPE_COUNT;
	jtagservice.claims = (CLAIM_RECORD*) calloc(jtagservice.claims_count, sizeof(CLAIM_RECORD));
	if (NULL == jtagservice.claims) {
		return AJI_NO_MEMORY;
	}
	AJI_ERROR status = jtagservice_create_claim_records(
		jtagservice.claims, &jtagservice.claims_count
	);
	return status;
}



AJI_ERROR jtagservice_init(const DWORD timeout) {
	AJI_ERROR status = AJI_NO_ERROR;

	status = jtagservice_init_common(timeout);
	if (status) {
		return status;
	}

	status = jtagservice_init_cable(timeout);
	if (status) {
		jtagservice_free_common(timeout);
		return status;
	}

	status = jtagservice_init_tap(timeout);
	if (status) {
		jtagservice_free_cable(timeout);
		jtagservice_free_common(timeout);
		return status;
	}


	status = c_jtag_client_gnuaji_init();
	if (AJI_NO_ERROR != status) {
		LOG_ERROR("Cannot initialize C_JTAG_CLIENT library. Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);

		jtagservice_free_cable(timeout);
		jtagservice_free_common(timeout);
		return status;
	}

	return AJI_NO_ERROR;
}


//=====================================
// JTAG scanning services
//=====================================

/**
 * Find the hardware cable from the jtag server
 * @return AJI_NO_ERROR if everything OK
 *         AJI_NO_MEMORY if run out of memory
 *         AJI_BAD_HARDWARE if cannot find hardware cable
 *         AJI_TIMEOUT if time out occured.
 * @pos Set jtagservice chain detail if return successful.
 */
AJI_ERROR jtagservice_scan_for_cables(void)
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

	if (hardware == NULL && jtagservice.hardware_count > 0) {
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

/**
 * Read the TAPs on the selected cable.
 * @pre The chain is already acquired, @see select_cable()
 * @pos jtagservice will be populated with the selected tap
 *
 * @sa unread_taps()
 */
AJI_ERROR jtagservice_scan_for_taps(void)
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
		jtagservice.hier_id_n[tap_position] = 10; //@TODO Find a good compromise for number of SLD so I don't have to call c_aji_get_nodes_b() twice.
		jtagservice.hier_ids[tap_position] = (AJI_HIER_ID*)calloc(jtagservice.hier_id_n[tap_position], sizeof(AJI_HIER_ID));
		if (NULL == jtagservice.hier_ids[tap_position]) {
			LOG_ERROR("Ran out of memory for jtagservice's tap  %lu's hier_ids", (unsigned long)tap_position);
			return AJI_NO_MEMORY;
		}
		jtagservice.hub_infos[tap_position] = (AJI_HUB_INFO*)calloc(jtagservice.hier_id_n[tap_position], sizeof(AJI_HUB_INFO));
		if (NULL == jtagservice.hub_infos[tap_position]) {
			LOG_ERROR("Ran out of memory for jtagservice's tap  %lu's hub_info", (unsigned long)tap_position);
			return AJI_NO_MEMORY;
		}

		status = c_aji_get_nodes_bi(
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

			free(jtagservice.hub_infos[tap_position]);
			jtagservice.hub_infos[tap_position] = (AJI_HUB_INFO*)calloc(jtagservice.hier_id_n[tap_position], sizeof(AJI_HUB_INFO));
			if (NULL == jtagservice.hub_infos[tap_position]) {
				LOG_ERROR("Ran out of memory for jtagservice's tap  %lu's hub_info", (unsigned long)tap_position);
				return AJI_NO_MEMORY;
			}

			status = c_aji_get_nodes_bi(
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


//=====================================
// misc
//=====================================

/**
 * Print SLD Node information
 * 
 * \param hier_id The SLD node information
 * \param hub_info Supplimentary hub information.
 *        if you do not want it displayed
 */
void jtagservice_sld_node_printf(const AJI_HIER_ID* hier_id, const AJI_HUB_INFO* hub_infos)
{
	printf(" idcode=%08lX position_n=%lu position: ( ",
		(unsigned long) hier_id->idcode,
		(unsigned long) hier_id->position_n
	);
	if (hub_infos) {
		for (int m = 0; m <= hier_id->position_n; ++m) {
			printf("%d ", hier_id->positions[m]);
		} //end for m
		printf(")  hub_infos: ");
		for (int m = 0; m <= hier_id->position_n; ++m) {
			printf(" (Hub %d) bridge_idcode=%08lX, hub_id_code=%08lX", 
				m, 
				(unsigned long) ( m == 0 ? 0 : hub_infos->bridge_idcode[m]), 
				(unsigned long) (hub_infos->hub_idcode[m])
			);
		} //end for m (bridge)
	}
}

