/* read_configuration.h 
 * 
 * Kryten is a EPICS PV monitoring program that calls a system command
 * when the value of the PV matches/cease to match specified criteria.
 *
 * Copyright (C) 2011-2021  Andrew C. Starritt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact details:
 * andrew.starritt@gmail.com
 * PO Box 3118, Prahran East, Victoria 3181, Australia.
 *
 * Source code formatting:
 * indent options:  -kr -pcs -i3 -cli3 -nbbo -nut
 *
 */

#ifndef READ_PV_LIST_H_
#define READ_PV_LIST_H_

#include <ellLib.h>

#include "kryten.h"
#include "pv_client.h"

bool Scan_Configuration_File (const char *filename,
                              const Allocate_Client_Handle allocate);

bool Scan_Configuration_String (const char *buffer,
                                const size_t size,
                                const Allocate_Client_Handle allocate);

#endif                          /* READ_PV_LIST_H_ */
