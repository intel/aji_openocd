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

#include <stdbool.h>
#include <stdio.h>

#include "aji/aji.h"
#include "aji/c_aji.h"

//========================================
// JTAG Core overwrites
//========================================
int jtagservice_jtag_examine_chain(void);
int jtagservice_jtag_validate_ircapture(void);


//========================================
// AJI-based JTAG Services
//========================================

//Maximum time out time is 0x7FFFFF.
#define JTAGSERVICE_TIMEOUT_MS 60 * 1000 

AJI_ERROR jtagservice_init(const DWORD timeout);
AJI_ERROR jtagservice_free(const DWORD timeout);


AJI_ERROR jtagservice_scan_for_cables(void);
AJI_ERROR jtagservice_scan_for_taps(void);

AJI_ERROR jtagservice_lock(const struct jtag_tap* const tap);
AJI_ERROR jtagservice_unlock(void);




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
AJI_ERROR jtagservice_activate_any_tap(const DWORD hardware_index);

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
AJI_ERROR jtagservice_validate_tap_index(
	const DWORD hardware_index, const DWORD tap_index);

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
	const DWORD node_index);

int jtagservice_query_main(void);
void jtagservice_display_sld_nodes(void);


void jtagservice_sld_node_printf(
	const AJI_HIER_ID* hier_id, 
	const AJI_HUB_INFO* hub_info);
AJI_ERROR jtagservice_hier_id_index_by_idcode(
	const DWORD idcode,
	const AJI_HIER_ID *vtap_list, const DWORD vtap_count,
	DWORD* vtap_index);


/**
 * Helper function to print \c jtagservice_record
 *
 * \param stream The stream to output the result to
 * \param record The record to output
 */
void jtagservice_fprintf_jtagservice_record(
	FILE *stream
);

#endif //JTAGSERVICE_H_INCLUDED

