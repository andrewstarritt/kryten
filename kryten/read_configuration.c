/* $File: //depot/sw/epics/kryten/read_configuration.c $
 * $Revision: #10 $
 * $DateTime: 2012/02/25 15:42:01 $
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>

#include "pv_client.h"
#include "read_configuration.h"

#define MAX_LINE_LENGTH    256

#define ALTERNATIVE    '|'
#define RANGE          '~'


static int debug = 0;

/*------------------------------------------------------------------------------
 * Skip white space
 */
#define SKIP_WHITE_SPACE(input) {                                     \
   while (isspace (*input)) {                                         \
      input++;                                                        \
   }                                                                  \
}

/*------------------------------------------------------------------------------
 * Error return if at end of line
 */
#define QUIT_ON_EOL(input) {                                          \
   if (*input == '\0') {                                              \
      printf ("%s:%d premature end of line.\n", filename, line_num);  \
      return false;                                                   \
   }                                                                  \
}

/*------------------------------------------------------------------------------
 * Skip white space - return of end of line
 */
#define SKIP_WHITE_QUIT_ON_EOL(input) {                               \
   SKIP_WHITE_SPACE (input);                                          \
   QUIT_ON_EOL (input);                                               \
}

/*------------------------------------------------------------------------------
 * Extract sub string from start upto but excluding finish.
 * Returns length of extracted string.
 */
static int extract (char *into, const int into_size, const char *start,
                    const char *finish)
{
   int n;

   n = (long) finish - (long) start;

   /* Truncate if necessary - leave room for the trailing '\0'
    */
   if (n > into_size - 1) {
      n = into_size - 1;
   }

   strncpy (into, start, (size_t) n);
   into[n] = '\0';

   return n;
}                               /* extract */


/*-----------------------------------------------------------------------------
 * The item parameter is assumed tp be trimmed of trailing white space.
 * This function fail returns false if strtod does not consure all of item.
 *
 * Ada's  double'Value ("...") would be really good here.
 */
static bool scan_double (const char *item, double *number)
{
   char *endptr = NULL;

   errno = 0;
   *number = strtod (item, &endptr);
   if ((errno == 0) && (endptr == item + strlen (item))) {
      return true;
   }

   return false;
}                               /* scan_double */

/*-----------------------------------------------------------------------------
 * Ditto for long.
 */
static bool scan_long (const char *item, long *number)
{
   char *endptr = NULL;

   /* Try decimal number first
    */
   errno = 0;
   *number = strtol (item, &endptr, 10);
   if ((errno == 0) && (endptr == item + strlen (item))) {
      return true;
   }

   /* Try hexadecimal number next
    */
   errno = 0;
   *number = strtol (item, &endptr, 16);
   if ((errno == 0) && (endptr == item + strlen (item))) {
      return true;
   }

   return false;
}                               /* scan_long */

/*------------------------------------------------------------------------------
 * Input format is general, e.g.  123, 0x123, 456.67, 32.99e+8, Text, "text".
 */
static bool scan_value (char *input, Varient_Value * data, char **endptr,
                        const char *filename, const int line_num)
{
   char *source;
   int len;
   char *start;
   char *finish;
   char item[MAX_LINE_LENGTH];
   int n;

   /* Ensure not erroneous.
    */
   data->kind = vkVoid;

   source = input;
   SKIP_WHITE_SPACE (source);

   /* Remaining length
    */
   len = strlen (source);

   /* Need to check that len is non-zero as strtol("", ...) and
    * strtod("", ...) does consume all input, but does not set errno.
    */
   if (len == 0) {
      return false;
   }

   /* Save start to lexical item - scan for end.
    */
   start = source;
   if (*source == '"') {
      source++;
      while ((*source != '\0') && (*source != '"')) {
         source++;
      }
      if (*source == '"') {
         source++;
      }
   } else {
      /* Numeric or unquoted string
       */
      while ((*source != '\0') && (*source != '~') && (*source != '|') &&
             (isspace (*source) == 0)) {
         source++;
      }
   }
   finish = source;
   *endptr = source;

   /* Extract lexical item into a local copy
    */
   n = extract (item, sizeof (item), start, finish);

   if (debug) {
      printf ("%s:%d  extracted item %s\n", filename, line_num, item);
   }

   /* check for a quoted string.
    */
   if (item[0] == '"') {
      /* exclude leading/trailing quotes
       */
      n = n - 2;
      if (n > MAX_STRING_SIZE) {
         printf ("%s:%d quoted string too big: %s\n", filename, line_num,
                 item);
         return false;
      }

      strncpy (data->value.sval, &item[1], n);
      data->value.sval[n] = '\0';

      data->kind = vkString;
      return true;
   }

   /* Test for numerical values.
    * Must test integer first as scan_double will match an integer.
    */
   if (scan_long (item, &data->value.ival)) {
      data->kind = vkInteger;
      return true;
   }

   if (scan_double (item, &data->value.dval)) {
      data->kind = vkFloating;
      return true;
   }

   /* Treat as an unquoted string
    */
   if (n > MAX_STRING_SIZE) {
      printf ("%s:%d un-quoted string too big: %s\n", filename, line_num,
              item);
      return false;
   }

   strncpy (data->value.sval, item, n);
   data->value.sval[n] = '\0';

   data->kind = vkString;
   return true;
}                               /* scan_value */

/*------------------------------------------------------------------------------
 * pv_name and command must be large enough.
 * filename and line_num used for error reports
 */
bool parse_line (char *line, char *pv_name, int *index,
                 Varient_Range_Collection * pVRC, char *command,
                 const char *filename, const int line_num)
{
   char *source;
   char *target;
   char *endptr;
   bool status;
   int j;
   Varient_Kind expected;
   Varient_Kind lower_kind;
   Varient_Kind upper_kind;

   /* Ensure not erroneous/set defaults.
    */
   *pv_name = '\0';
   *index = 1;
   pVRC->count = 0;
   *command = '\0';

   source = line;

   /* Skip white space (if any) - error return if at end of line.
    */
   SKIP_WHITE_QUIT_ON_EOL (source);

   target = pv_name;
   while ((*source != '\0') && (isspace (*source) == false)) {
      *target = *source;
      source++;
      target++;
   }
   *target = '\0';

   /* Check for sensible PV name, check it starts okay at least.
    */
   if ((isalnum (*pv_name) == false) && (*pv_name != '$')) {
      printf ("%s:%d error invalid PV name: %s\n", filename, line_num,
              pv_name);
      return false;
   }

   if (debug > 4) {
      printf ("%s:%d  pv = %s\n", filename, line_num, pv_name);
   }

   SKIP_WHITE_QUIT_ON_EOL (source);

   if (*source == '[') {
      source++;
      SKIP_WHITE_QUIT_ON_EOL (source);

      *index = strtol (source, &endptr, 10);
      if ((errno != 0) || (endptr == NULL)) {
         return false;
      }
      source = endptr;

      if (debug > 4) {
         printf ("%s:%d  index = %d\n", filename, line_num, *index);
      }

      if (*index < 1) {
         printf ("%s:%d error invalid PV index: %d\n", filename, line_num,
                 *index);
         return false;
      }

      if (*index > 1000) {
         printf ("%s:%d query valid PV index: %d ???\n", filename,
                 line_num, *index);
      }

      SKIP_WHITE_QUIT_ON_EOL (source);

      if (*source == ']') {
         source++;
      } else {
         printf ("%s:%d error missing ']'\n", filename, line_num);
         return false;
      }

   } else {
      *index = 1;
   }

   /* Parse match criteria
    */
   for (j = 0; j < NUMBER_OF_VARIENT_RANGES; j++) {

      SKIP_WHITE_QUIT_ON_EOL (source);
      status = scan_value
          (source, &pVRC->item[j].lower, &endptr, filename, line_num);
      if (status == false) {
         return false;
      }
      source = endptr;

      SKIP_WHITE_QUIT_ON_EOL (source);
      if (*source == '~') {

         source++;
         SKIP_WHITE_QUIT_ON_EOL (source);
         status = scan_value
             (source, &pVRC->item[j].upper, &endptr, filename, line_num);
         if (status == false) {
            return false;
         }
         source = endptr;

      } else {
         /* Just set upper value the same as the lower value.
          */
         pVRC->item[j].upper = pVRC->item[j].lower;
      }

      /* Update number of valid entries so far
       */
      pVRC->count = j + 1;

      SKIP_WHITE_QUIT_ON_EOL (source);
      if (*source == '~') {
         printf ("%s:%d error unexpected 3rd value in sub-match %d\n",
                 filename, line_num, pVRC->count);
         return false;
      }

      /* Are value kinds consistent?
       */
      expected = pVRC->item[0].lower.kind;
      lower_kind = pVRC->item[j].lower.kind;
      upper_kind = pVRC->item[j].upper.kind;

      if (lower_kind == upper_kind) {
         /* Okay or generate warning together
          */
         if (lower_kind != expected) {
            printf
                ("%s:%d warning type mis-match: both values of sub-match %d are %s, expecting %s.\n",
                 filename, line_num, pVRC->count, vkImage (lower_kind),
                 vkImage (expected));
         }
      } else {
         /* Never fails for zeroth entry
          */
         if (lower_kind != expected) {
            printf
                ("%s:%d warning type mis-match: lower value of sub-match %d is %s, expecting %s.\n",
                 filename, line_num, pVRC->count, vkImage (lower_kind),
                 vkImage (expected));
         }

         if (upper_kind != expected) {
            printf
                ("%s:%d warning type mis-match: upper value of sub-match %d is %s, expecting %s.\n",
                 filename, line_num, pVRC->count, vkImage (upper_kind),
                 vkImage (expected));
         }
      }

      if (*source != '|') {
         break;
      }
      source++;                 /* skip the '|' */

      if (pVRC->count >= NUMBER_OF_VARIENT_RANGES) {
         printf
             ("%s:%d error attempting to specify more than %d sub-matches\n",
              filename, line_num, NUMBER_OF_VARIENT_RANGES);
         return false;
      }
   }

   SKIP_WHITE_QUIT_ON_EOL (source);

   target = command;
   while ((*source != '\0') && (isspace (*source) == false)) {
      *target = *source;
      source++;
      target++;
   }
   *target = '\0';

   SKIP_WHITE_SPACE (source);

   if (*source != '\0') {
      printf
          ("%s:%d warning trailing input after command %s ignored: %s\n",
           filename, line_num, command, source);
   }

   return true;
}                               /* parse_line */


/*------------------------------------------------------------------------------
 */
bool Scan_Configuration_File (const char *filename,
                              ELLLIST * ca_client_list)
{
   const char *function = "Scan_Configuration_File";

   FILE *input_file = NULL;
   CA_Client *pClient = NULL;
   int line_num;
   char line[MAX_LINE_LENGTH + 1];
   int len;
   char pv_name[MAX_LINE_LENGTH + 1];
   int index;
   char command[MAX_LINE_LENGTH + 1];
   Varient_Range_Collection match_set_collection;
   bool status;
   char *source;


   if (debug > 0) {
      printf ("%s: entry: filename='%s'.\n", function, filename);
   }

   if (ca_client_list == NULL) {
      printf ("%s: null client list\n", function);
      return false;
   }

   /* Attempt to open the specified file.
    */
   input_file = fopen (filename, "r");
   if (!input_file) {
      printf ("%s: unable to open file %s.\n", function, filename);
      return false;
   }

   /* Read lines from file
    */
   line_num = 0;
   while (fgets (line, MAX_LINE_LENGTH, input_file)) {

      line_num++;

      /* Remove trailing \n char
       */
      len = strlen (line);
      if ((len > 0) && (line[len - 1] == '\n')) {
         line[len - 1] = '\0';
      }

      source = line;
      SKIP_WHITE_SPACE (source);

      /* Ignore empty lines and comment lines.
       */
      if ((*source == '\0') || (*source == '#')) {
         continue;
      }

      status = parse_line (source, pv_name, &index, &match_set_collection,
                           command, filename, line_num);

      if (status == false) {
         /* Any errors already reported - jist print whole line.
          */
         printf ("%s:%d %s\n", filename, line_num, line);
         continue;
      }

      if (debug >= 2) {
         printf ("processing PV: %s [%d] {match}%d %s\n", pv_name, index,
                 match_set_collection.count, command);
      }

      pClient = Allocate_Client ();
      if (pClient) {
         strncpy (pClient->pv_name, pv_name, sizeof (pClient->pv_name));
         pClient->element_index = index;
         strncpy (pClient->match_command, command, MATCH_COMMAND_LENGTH);
         pClient->match_set_collection = match_set_collection;

         /* Lastly add to client list.
          */
         ellAdd (ca_client_list, (ELLNODE *) pClient);
         continue;
      }

      /* Ignore anything left over
       */
      printf ("%s:%d warning skipping: '%s' - failed to allocate client\n",
              filename, line_num, line);
   }

   /* Close file
    */
   (void) fclose (input_file);

   if (debug > 0) {
      printf ("%s: exit:  filename='%s'.\n", function, filename);
   }

   return true;
}                               /* Scan_Configuration_File */

/* end */
