/* $File: //depot/sw/epics/kryten/information.c $
 * $Revision: #7 $
 * $DateTime: 2012/02/04 09:44:25 $
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

#include "information.h"
#include "utilities.h"

/*------------------------------------------------------------------------------
 */
void Version ()
{
   printf ("%skryten%s version: %s (built %s)\n",
           green, reset, KRYTEN_VERSION, BUILD_DATETIME);
}

/*------------------------------------------------------------------------------
 */
void Usage ()
{
   printf ("usage: kryten  [OPTIONS]  configuration-file\n"     /* */
           "       kryten  --help | -h\n"       /* */
           "       kryten  --version\n" /* */
           "       kryten  --licence | -l\n"    /* */
           "       kryten  --warranty | -w\n"   /* */
           "       kryten  --redistribute | -r\n");
}                               /* Usage */


/*------------------------------------------------------------------------------
 */
static const char *help_text =
    "%skryten%s allows an arbitary set of EPICS Channel Access Process Variables (PVs)\n"
    "to be monitored and if the monitored value starts to match or ceases to match\n"
    "the given criteria then invokes a specified system command.\n"
    "\n"
    "Options\n"
    "\n"
    "--check, -c\n"
    "    Check configuration file and print errors/warnings and quit.\n"
    "\n"
    "--daemon, -d\n"
    "    Run program as system daemon.\n"
    "\n"
    "--suppress, -s\n"
    "    Suppress copyright preamble when program starts.\n"
    "\n"
    "--verbose, -v\n"
    "    Output is more verbose.\n"
    "\n"
    "--help, -h\n"
    "    Display this help information and quit.\n"
    "\n"
    "--version\n"
    "    Display verion information and quit.\n"
    "\n"
    "--licence, -l\n"
    "    Display licence information and quit.\n"
    "\n"
    "--warranty, -w\n"
    "    Display the without warranty information and quit.\n"
    "\n"
    "--redistribute | -r\n"
    "    Display the program redistribution conditions and quit.\n"
    "\n"
    "\n"
    "Parameters\n"
    "\n"
    "configuration-file\n"
    "    Specifies the file that defines the PVs to be monitored together with\n"
    "    match critera and the system command to be called.\n"
    "\n"
    "    The expected file format is as follows. Blank lines and lines starting\n"
    "    with a # character are ignored, the later being useful for comments.\n"
    "    Otherwise the line consists of fields. Fields are separated by white\n"
    "    space (spaces and/or tabs) with optional leading and tailing white space.\n"
    "\n"
    "    The first field is the PV name.\n"
    "\n"
    "    The second field is the match criteria. This consists of upto 16 sub\n"
    "    -match criteria seprated by a | character. Each sub field consists of\n"
    "    a single value or a pair of values, sepratated by a ~ character, that\n"
    "    specifies an inclusive range of match values.\n"
    "\n"
    "    The values may be a string, an integer or a floating value. The type of\n"
    "    the first match value determines the Channel Access request field type:\n"
    "\n"
    "        String    DBF_STRING\n"
    "        Integer   DBF_LONG\n"
    "        Floating  DBF_DOUBLE\n"
    "\n"
    "    It is therefore important that a range of values, say for a pump, be\n"
    "    specified as 2.0~6.25 as opposed to 2~6.25, as the latter will cause the\n"
    "    subscription of DBF_LONG values from the IOC, yielding, for example, a\n"
    "    returned value of 6 when the true value is 6.45, thus leading to an\n"
    "    erroneous match.\n"
    "\n"
    "    Match criteria values may be forced to be considered string by enclosing\n"
    "    the value in double quotes (\"). String values containing white space\n"
    "    must be encliosed in double quotes. For string matches, the case is\n"
    "    significant.\n"
    "\n"
    "    The third field is any program or script that is normally available to\n"
    "    the user. If a relative path name is specified, this is relative to the\n"
    "    directory in which kryten was started, and not to the configuration file.\n"
    "    If a path name is not specified, then the usual PATH environment search\n"
    "    rules apply.\n"
    "\n"
    "    The program or script should expect three parameters, namely the PV name,\n"
    "    the match status (i.e. 'entry' or 'exit') and the current PV value. The\n"
    "    program or script is only invoked when the natch status changes. If a PV\n"
    "    disconnects then the program or script is called with a 'disconnect' status\n"
    "    and the value parameter is an empty string.\n"
    "\n"
    "    The program or script is run in background mode, and therefore it will run\n"
    "    asynchronously.\n\n";

static const char *example_text =
    "Configuration file example:\n"
    "\n"
    "# This is a comment within and example kryten configuration file\n"
    "\n"
    "# Monitor beam current and invoke xmessage if current drops below 5mA or\n"
    "# exceeds 205 mA\n"
    "# Note: we assume current never ever < -1.0e9 or > +1.0e9\n"
    "#\n"
    "SR11BCM01:CURRENT_MONITOR   -1.0e9~5.0|205.0~+1.0e9   /usr/bin/xmessage\n"
    "\n"
    "# Monitor the rainbow status and invoke echo when status becomes Green or Orange.\n"
    "#\n"
    "RAINDOW:STATUS              \"Green\"|\"Orange\"          /bin/echo\n"
    "\n" "# end\n\n";

static const char *smiley_text =
    "%skryten%s is named after Kryten 2X4B 523P out of RE%sD D%sWARF, the\n"
    "classic British SciFi series (http://www.reddwarf.co.uk).\n\n";

void Help ()
{
   Version ();
   printf ("\n");
   Usage ();
   printf ("\n");
   printf (help_text, green, reset);
   printf (example_text);
   printf (smiley_text, green, reset, red, reset);
}                               /* Help */


/*------------------------------------------------------------------------------
 */
static const char *preamble_text =
    "%skryten%s  Copyright (C) 2011-2012 Andrew C. Starritt\n"
    "This program comes with ABSOLUTELY NO WARRANTY; for details run 'kryten --warranty'.\n"
    "This is free software, and you are welcome to redistribute it under certain\n"
    "conditions; run 'kryten --redistribute' for details.\n\n";

void Preamble ()
{
   printf (preamble_text, green, reset);
}

/* end */
