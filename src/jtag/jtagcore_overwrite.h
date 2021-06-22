/***************************************************************************
 *   Copyright (C) 2021 Cinly Ooi                                          *
 *   cinly.ooi@intel.com                                                   *
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

#ifndef OPENOCD_JTAGCORE_OVERWRITE_H
#define OPENOCD_JTAGCORE_OVERWRITE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


/*
 * JTAG CORE FUNCTION OVERWRITE
 * 
 * Sometimes, the core function provided by the JTAG Core has
 * to be replaced with one provided by the driver.
 * The set of functions and structure here provide
 * the facility to do so
 */


/**
 * Structure to hold information about overwriting
 * jtag core function.
 */
struct jtagcore_overwrite {
	int (*jtag_examine_chain)(void);
	int (*jtag_validate_ircapture)(void);

};

/**
 * Get the JTAG Core function overwrite data
 * \return The jtagcore_overwrite record. Will not return NULL
 */
struct jtagcore_overwrite* jtagcore_get_overwrite_record(void);

#endif /* OPENOCD_JTAGCORE_OVERWRITE_H */
