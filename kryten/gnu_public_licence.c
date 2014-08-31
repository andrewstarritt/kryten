/* $File: //depot/sw/epics/kryten/gnu_public_licence.c $
 * $Revision: #4 $
 * $DateTime: 2012/03/18 11:25:45 $
 * Last checked in by: $Author: andrew $
 *
 * Description:
 * Kryten is a EPICS PV monitoring program that calls a system command
 * when the value of the PV matches/cease to match specified criteria.
 *
 * Copyright (C) 2011-2012  Andrew C. Starritt
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
 * starritt@netspace.net.au
 * PO Box 3118, Prahran East, Victoria 3181, Australia.
 *
 * Source code formatting:
 * indent options:  -kr -pcs -i3 -cli3 -nbbo -nut
 *
 */
#include <stdio.h>

#include "utilities.h"
#include "gnu_public_licence.h"

/*------------------------------------------------------------------------------
 */
static const char *licence_text =
    "%skryten%s is distributed under the the GNU General Public License version 3.\n"
    "\n"
    "Copyright (C) 2011-2012  Andrew C. Starritt\n"
    "\n"
    "This program is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation, either version 3 of the License, or\n"
    "(at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
    "\n";

void Licence ()
{
   printf (licence_text, green, reset);
}                               /* Licence */

/*------------------------------------------------------------------------------
 */
static const char *no_warranty_text =
    "%skryten%s is distributed under the the GNU General Public License version 3.\n"
    "\n"
    "Disclaimer of Warranty.\n"
    "\n"
    "  THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY\n"
    "APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT\n"
    "HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY\n"
    "OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,\n"
    "THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n"
    "PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM\n"
    "IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF\n"
    "ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n\n";

void No_Warranty ()
{
   printf (no_warranty_text, green, reset);
}                               /* No_Warranty */

/*------------------------------------------------------------------------------
 */
static const char *redistribute_text =
    "%skryten%s is distributed under the the GNU General Public License version 3.\n"
    "\n"
    "This program is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation, either version 3 of the License, or\n"
    "(at your option) any later version.\n\n";

void Redistribute ()
{
   printf (redistribute_text, green, reset);
}                               /* Redistribute */

/* end */
