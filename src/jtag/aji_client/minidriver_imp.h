/* WARNING 
 * This file is copied from src/jtag/aji_client/ to src/jtag by the make
 * process. Make sure you do not edit the copy in src/jtag as it can be 
 * replaced by the original, and SCM is setup to ignore the copy in src/jtag
 */
 
/***************************************************************************
 *   Copyright (C) 2005 by Dominic Rath <Dominic.Rath@gmx.de>              *
 *   Copyright (C) 2007,2008 Øyvind Harboe <oyvind.harboe@zylin.com>       *
 *   Copyright (C) 2009 Zachary T Welch <zw@superlucidity.net>             *
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

#ifndef OPENOCD_JTAG_JCLIENT_MINIDRIVER_IMP_H
#define OPENOCD_JTAG_JCLIENT_MINIDRIVER_IMP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <jtag/jtag_minidriver.h>

void interface_jtag_add_callback(jtag_callback1_t f, jtag_callback_data_t data0);

void interface_jtag_add_callback4(jtag_callback_t f, jtag_callback_data_t data0,
				  jtag_callback_data_t data1, jtag_callback_data_t data2,
				  jtag_callback_data_t data3);

void jtag_add_callback4(jtag_callback_t f, jtag_callback_data_t data0,
			jtag_callback_data_t data1, jtag_callback_data_t data2,
			jtag_callback_data_t data3);
#endif /* OPENOCD_JTAG_JCLIENT_MINIDRIVER_IMP_H */
