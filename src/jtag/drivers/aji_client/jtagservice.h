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
 * Get the OPEN ID of the currently in use (locked) TAP/SLD node
 *
 * \return The #AJI_OPEN_ID of the TAP/SLD node in use.
 *         If return NULL, no #AJI_OPEN_ID in use
 */
AJI_OPEN_ID jtagservice_get_in_use_open_id(void);

/**
 * Helper function to print \c jtagservice_record
 *
 * \param stream The stream to output the result to
 * \param record The record to output
 */
void jtagservice_fprintf_jtagservice_record(
	FILE *stream
);



int jtagservice_query_main(void);

#endif //JTAGSERVICE_H_INCLUDED

