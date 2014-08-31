/* $File: //depot/sw/epics/kryten/read_configuration.c $
 * $Revision: #13 $
 * $DateTime: 2012/03/03 23:48:38 $
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
#include <ctype.h>

#include "read_configuration.h"
#include "utilities.h"


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
   bool status;

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
   extract (item, sizeof (item), start, finish);
   n = strlen (item);

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
    * Must test integer first as double_value will match an integer.
    */
   data->value.ival = long_value (item, &status);
   if (status == true) {
      data->kind = vkInteger;
      return true;
   }

   data->value.dval = double_value (item, &status);
   if (status == true) {
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
   static const char *type_mis_match =
       "%s:%d warning: %s value of sub-match %d is %s, expecting %s.\n";

   char *source;
   char *target;
   char *start;
   char *finish;
   char item[MAX_LINE_LENGTH];
   char *endptr;
   bool status;
   int j;
   Varient_Kind expected;
   Varient_Kind lower_kind;
   Varient_Kind upper_kind;
   bool single_value;
   bool simple_command;

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
      source++;                 /* skip the '['  */

      start = source;

      /* Find the ']'
       */
      while ((*source != '\0') && (*source != ']')) {
         source++;
      }

      if (*source != ']') {
         printf ("%s:%d error missing ']'\n", filename, line_num);
         return false;
      }
      finish = source;
      source++;                 /* skip the ']'  */

      /* Extract lexical item into a local copy
       */
      extract (item, sizeof (item), start, finish);

      *index = (int) long_value (item, &status);

      if (status == false) {
         printf ("%s:%d  index item [%s] is not a valid integer\n",
                 filename, line_num, item);
         return false;
      }

      if (debug > 4) {
         printf ("%s:%d [%s] index = %d\n", filename, line_num, item,
                 *index);
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
         single_value = false;
      } else {
         /* Just set upper value the same as the lower value.
          */
         pVRC->item[j].upper = pVRC->item[j].lower;
         single_value = true;
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

      /* Never fails for zeroth entry
       */
      if (lower_kind != expected) {
         printf (type_mis_match, filename, line_num,
                 (single_value ? "the" : "1st"), pVRC->count,
                 vkImage (lower_kind), vkImage (expected));
      }

      if ((!single_value) && (upper_kind != expected)) {
         printf (type_mis_match, filename, line_num,
                 "2nd", pVRC->count,
                 vkImage (upper_kind), vkImage (expected));
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

   simple_command = true;

   /* Copy rest of line to command
    */
   target = command;
   while (*source != '\0') {
      if (isspace (*source) == 0) {
         /* Not white space - just copy and increment pointers.
          */
         *target = *source;
         source++;
         target++;
      } else {
         SKIP_WHITE_SPACE (source);
         /* If not end of line then add a sigle space and
          * flag as non simple command
          */
         if (*source != '\0') {
            *target = ' ';
            target++;
            simple_command = false;
         }
      }
   }
   *target = '\0';

   SKIP_WHITE_SPACE (source);

   /* Append default parameter spec if simple command
    */
   if (simple_command) {
      strcat (command, " %p %m %v %e");
   }

   return true;
}                               /* parse_line */


/*------------------------------------------------------------------------------
 */
bool Scan_Configuration_File (const char *filename,
                              const Allocate_Client_Handle allocate)
{
   const char *function = "Scan_Configuration_File";

   FILE *input_file = NULL;
   CA_Client *pClient = NULL;
   int line_num;
   char line[MAX_LINE_LENGTH + 1];
   /* The extracted pv name can't be longer than the input line.
    * And the command can have at most 12 characters added to it.
    */
   char pv_name[MAX_LINE_LENGTH + 1];
   char command[MAX_LINE_LENGTH + 13];
   int len;
   int index;
   Varient_Range_Collection match_set_collection;
   bool status;
   char *source;

   if (debug > 0) {
      printf ("%s: entry: filename='%s'.\n", function, filename);
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
         /* Any errors already reported - just print whole line.
          */
         printf ("%s:%d %s\n", filename, line_num, line);
         continue;
      }

      /* Check sizes
       */
      if (strlen (pv_name) > sizeof (pClient->pv_name) - 1) {
         printf ("%s:%d pv name too long\n", filename, line_num);
         printf ("%s:%d %s\n", filename, line_num, line);
         continue;
      }

      if (strlen (command) > sizeof (pClient->match_command) - 1) {
         printf ("%s:%d command too long\n", filename, line_num);
         printf ("%s:%d %s\n", filename, line_num, line);
         continue;
      }

      if (debug >= 2) {
         printf ("processing PV: %s [%d] {match}%d %s\n", pv_name, index,
                 match_set_collection.count, command);
      }

      pClient = allocate ();
      if (pClient) {
         /* Unlike strncpy, snprintf includes tailing '\0'
          */
         snprintf (pClient->pv_name, sizeof (pClient->pv_name), "%s",
                   pv_name);
         snprintf (pClient->match_command, sizeof (pClient->match_command),
                   "%s", command);
         pClient->element_index = index;
         pClient->match_set_collection = match_set_collection;

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
