/* $File: //depot/sw/epics/kryten/information.c $
 * $Revision: #11 $
 * $DateTime: 2012/03/04 15:10:39 $
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
static const char *intro_text =
    "%skryten%s allows an arbitary set of EPICS Channel Access Process Variables (PVs)\n"
    "to be monitored and if the monitored value starts to match or ceases to match\n"
    "the given criteria then invokes a specified system command.\n";

static const char *help_text =
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
    "configuration-file\n"
    "\n"
    "The configuration-file parameter is the name of the file that defines the\n"
    "PVs to be monitored together with match critera and the system command to\n"
    "be called.\n"
    "\n"
    "The expected file format is described below using a Backus Naur like \n"
    "syntax. Blank lines and lines starting with a # character are ignored,\n"
    "the later being useful for comments. Items in {} are primitives and are\n"
    "defined after the syntax. Items in single quotes (') are to be interpreted\n"
    "literally.\n"
    "\n"
    "<line> ::=\n"
    "    '#' {any text} | <channel-spec> | <null>\n"
    "\n"
    "<channel-spec> ::=\n"
    "    <pv-name> <element-index> <match-list> <command>\n"
    "\n"
    "<pv-name> ::=\n"
    "    {PV name}\n"
    "\n"
    "<element-index> ::=\n"
    "    '[' {element index} ']' | <null>\n"
    "\n"
    "<match-list> ::=\n"
    "    <match-item> | <match-item> '|' <match-list>\n"
    "\n"
    "<match-item> ::=\n"
    "    <value> | <value> '~' <value>\n"
    "\n"
    "<value> ::=\n"
    "    {integer} | {real number} | <string-value>\n"
    "\n"
    "<string-value> ::=\n"
    "    {unquoted string} | '\"'{any text}'\"'\n"
    "\n"
    "<command> ::=\n"
    "    <simple-command> | <elaborate-command>\n"
    "\n"
    "<simple-command> ::=\n"
    "    {basic command, no parameters}\n"
    "\n"
    "<elaborate-command> ::=\n"
    "    <simple-command> <parameters>\n"
    "\n"
    "<parameters> ::=\n"
    "    <parameter> | <parameter> <parameters>\n"
    "\n"
    "<parameter> ::=\n"
    "    {any text} | '%%p' | '%%e' | '%%m' | '%%v'\n"
    "\n"
    "<null> ::=\n"
    "    {blank, empty}\n"
    "\n"
    "PV name\n"
    "The usual EPICS interpretation of a PV name. The PV name may include an\n"
    "optional record field name (e.g. .SEVR).\n"
    "\n"
    "As of yet, kryten does not understand long strings applicable when the \n"
    "PV name end with a '$'.\n"
    "\n"
    "Integer\n"
    "Any integer number. Hexadecimal numbers (e.g. 0xCAFE) also accepted.\n"
    "\n"
    "Element Index\n"
    "For waveform records and other array PVs an element index may be specified.\n"
    "When specified, the element index must be a positive integer. When not\n"
    "specified the default is 1.\n"
    "Note: kryten array indexing starts from 1.\n"
    "\n"
    "Real Number\n"
    "Any real number, i.e. a fixed point numbers or a floating point number.\n"
    "A real number specifically excludes items that are also integer, \n"
    "e.g. 4.0 is a real number, 4 is an integer.\n"
    "\n"
    "Unquoted String\n"
    "Any text that does not contain white space and is neither an integer nor a\n"
    "real number is interpreted as an unquoted string. If a string value requires\n"
    "one or more spaces it must be quoted.\n"
    "Note: quoted and unquoted (e.g. \"Red\" and Red) semantically identical.\n"
    "\n"
    "Match List\n"
    "Upto 16 match items may be specified.\n"
    "\n"
    "Format Converson Parameters\n"
    "%%p, %%e, %%m, and %%v are format conversion parameters that are expanded\n"
    "prior to the system call as follows:\n"
    "\n"
    "    %%p is replaced by the PV name,\n"
    "    %%m is replaced by the match status (i.e. 'match', 'reject' or 'disconnect'),\n"
    "    %%v is replaced by the current PV value; and\n"
    "    %%e is replaced by the element number.\n"
    "\n"
    "Configuration file example\n"
    "%s\n"
    "# This is a comment within an example kryten configuration file. \n"
    "# Monitor beam current and invoke xmessage if current drops below 5mA or \n"
    "# exceeds 205 mA or when the current enters the range 5mA to 205 mA \n"
    "# Note: we assume the beam current never ever < -1.0e9 or > +1.0e9 \n"
    "#\n"
    "SR11BCM01:CURRENT_MONITOR -1.0e9 ~ 5.0 | 205.0~+1.0e9 /usr/bin/xmessage \n"
    "\n"
    "# Monitor the rainbow status and invoke echo when status becomes Green or Orange \n"
    "# or when the status ceases to be neither Green nor Orange. \n"
    "#\n"
    "RAINBOW:STATUS \"Green\" | \"Orange\" /bin/echo \n"
    "\n"
    "# Monitor 3rd element of waveform record for value being 199 \n"
    "#\n"
    "WAVEFORM:ARRAY [3] 199 /bin/echo\n"
    "\n"
    "# Monitor for prime numbers - just echo value \n"
    "#\n"
    "NATURAL:NUMBER 2 ~ 3 | 5 | 7 | 11 | 13 | 17 | 19 | 23 | 27 /bin/echo %%v\n"
    "\n"
    "# end%s\n"
    "\n"
    "Operations\n"
    "\n"
    "The match item values may be a string, an integer or a real number value.\n"
    "The type of the first match value determines the Channel Access request\n"
    "field type:\n"
    "\n"
    "    String    DBF_STRING\n"
    "    Integer   DBF_LONG\n"
    "    Floating  DBF_DOUBLE\n"
    "\n"
    "It is therefore important that a range of values, say for a pump, be\n"
    "specified as 2.0~6.25 as opposed to 2~6.25, as the latter will cause the\n"
    "subscription of DBF_LONG values from the IOC, yielding, for example, a\n"
    "returned value of 6 when the true value is 6.45, thus leading to an \n"
    "erroneous match.\n"
    "\n"
    "Match criteria values may be forced to be considered string by enclosing\n"
    "the value in double quotes (\"). String values containing white space must\n"
    "be enclosed in double quotes. For string matches, the case is significant.\n"
    "\n"
    "The specified program or script must be one that is normally available to\n"
    "the user. If a relative path name is specified, this is relative to the\n"
    "directory in which kryten was started, and not relative to the configuration\n"
    "file. If a path name is not specified, then the usual PATH environment\n"
    "search rules apply.\n"
    "\n"
    "The program or script is only invoked when the match status changes. If a\n"
    "PV disconnects then the program or script is called with a 'disconnect'\n"
    "status and the value parameter is an empty string.\n"
    "\n"
    "The program or script is run in background mode, and therefore it will run\n"
    "asynchronously. It is the user's responsibility to manage the interactions\n"
    "between any asynchronous processes.\n"
    "\n"
    "When a basic command, i.e. no parameters, is specified, then the program\n"
    "or script should expect four parameters, namely:\n"
    "\n"
    "    the PV name,\n"
    "    the match status (i.e. 'match' or 'reject'),\n"
    "    the current PV value; and\n"
    "    the element number.\n"
    "\n";

static const char *smiley_text =
    "%skryten%s is named after Kryten 2X4B 523P out of RE%sD D%sWARF, the\n"
    "classic British SciFi series (http://www.reddwarf.co.uk).\n\n";

void Help ()
{
   Version ();
   printf ("\n");
   printf (intro_text, green, reset);
   printf ("\n");
   Usage ();
   printf ("\n");
   printf (help_text, yellow, reset);
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
