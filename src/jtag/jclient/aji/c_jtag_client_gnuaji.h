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

#ifndef INC_C_JTAG_CLIENT_GNUAJI_H
#define INC_C_JTAG_CLIENT_GNUAJI_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if PORT==WINDOWS
#include "c_jtag_client_gnuaji_win64.h"
#else
#include "c_jtag_client_gnuaji_lib64.h"
#endif

const char* c_aji_error_decode(AJI_ERROR code);
#endif
