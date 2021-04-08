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

#ifndef JTAGSERVICE_H_INCLUDED
#define JTAGSERVICE_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if IS_WIN32
#include <windows.h>
#endif

#include "aji/aji.h"
#include "aji/c_aji.h"

//Maximum time out time is 0x7FFFFF.
#define JTAGSERVICE_TIMEOUT_MS 60 * 1000 

#define IR_ARM_ABORT  0b1000 // dr_len=35
#define IR_ARM_DPACC  0b1010 // dr_len=35
#define IR_ARM_APACC  0b1011 // dr_len=35
#define IR_ARM_IDCODE 0b1110 // dr_len=32
#define IR_ARM_BYPASS 0b1111 // dr_len=1

#define IR_VJTAG_USER0     0xC; // dr_len=<not important>
#define IR_VJTAG_USER1     0xE; // dr_len=<not important>

typedef struct CLAIM_RECORD CLAIM_RECORD;
struct CLAIM_RECORD {
    DWORD claims_n;    ///! number of claims
//#if PORT == WINDOWS
    AJI_CLAIM* claims;
//#else
//    AJI_CLAIM2* claims; ///! IR claims
//#endif
};

#define DEVICE_TYPE_COUNT 4
enum DEVICE_TYPE {
    UNKNOWN = 0, ///! UNKNWON DEVICE
    ARM = 1, ///! ARM device, with IR length = 4 bit
    VJTAG = 2, ///! vJTAG/SLD 
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

typedef struct jtagservice_record jtagservice_record;
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

    //Active Tap
    DWORD  in_use_device_tap_position; //< index of the active tap.
                                       //< value can be [0, device_count) or
                                       //< UINT32_MAX if position is invalid
    AJI_DEVICE* in_use_device;
    DWORD       in_use_device_id;
    BYTE        in_use_device_irlen;

    //SLD Node
    //Additional information needed if
    //  SLD node was selected
    bool         is_sld;
    DWORD        in_use_hier_id_node_position;
    AJI_HIER_ID* in_use_hier_id;
    DWORD        in_use_hier_id_idcode;
};
 
AJI_ERROR jtagservice_init(jtagservice_record *me, const DWORD timeout);
AJI_ERROR jtagservice_init_common(jtagservice_record* me, const DWORD timeout);
AJI_ERROR jtagservice_init_cable(jtagservice_record* me, const DWORD timeout);
AJI_ERROR jtagservice_init_tap(jtagservice_record* me, const DWORD timeout);

AJI_ERROR jtagservice_free(jtagservice_record *me, const DWORD timeout);
AJI_ERROR jtagservice_free_common(jtagservice_record* me, const DWORD timeout);
AJI_ERROR jtagservice_free_cable(jtagservice_record* me, const DWORD timeout);
AJI_ERROR jtagservice_free_tap(jtagservice_record* me, const DWORD timeout);


AJI_ERROR jtagservice_activate_jtag_tap(jtagservice_record* me, const DWORD hardware_index, const DWORD tap_index);
AJI_ERROR jtagservice_activate_virtual_tap(jtagservice_record* me, const DWORD hardware_index, DWORD const tap_index, const DWORD node_index);

/**
 * Activate any TAP
 * 
 * Designed for situation where a JTAG operation is needed, but no TAP has been activated yet.
 * In situation like this, one cannot just simply select any TAP. The reason is AJI requires
 * us to setup #AJI_CLAIM  for the TAP to activate it, but not all TAPs' <tt>AJI_CLAIM</tt>s 
 * are known. This function sidestep this by activating any TAP that can be activated, 
 * in order to support the said JTAG operation
 * 
 * \param me The record to check against
 * \param hardware_index Not yet used, set to zero
 *
 * \return #AJI_NO_ERROR Success.
 * \return #AJI_FAILURE Cannot find any TAP that can be activated
 * 
 * \post If return \c AJI_NO_ERROR \c me is updated with information about the activated TAP.
 */
AJI_ERROR jtagservice_activate_any_tap(jtagservice_record* me, const DWORD hardware_index);

/**
 * Check that \c tap_index is in range
 *
 * \param me The record to check against
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
AJI_ERROR jtagservice_validate_tap_index(
    const jtagservice_record *me,
    const DWORD hardware_index, const DWORD tap_index);

/**
 * Check that \c node_index is in range.
 *
 * \param me The record to check against
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
    const jtagservice_record* me,
    const DWORD hardware_index,
    const DWORD tap_index,
    const DWORD node_index);

int jtagservice_query_main(void);
void jtagservice_display_sld_nodes(const jtagservice_record me);


void jtagservice_sld_node_printf(
    const AJI_HIER_ID* hier_id, 
    const AJI_HUB_INFO* hub_info);
AJI_ERROR jtagservice_device_index_by_idcode(
    const DWORD idcode,
    const AJI_DEVICE* tap_list, const DWORD tap_count,
    DWORD* tap_index);
AJI_ERROR jtagservice_hier_id_index_by_idcode(
    const DWORD idcode,
    const AJI_HIER_ID *vtap_list, const DWORD vtap_count,
    DWORD* vtap_index);

#endif //JTAGSERVICE_H_INCLUDED

