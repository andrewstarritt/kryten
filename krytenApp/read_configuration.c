/* read_configuration.c
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
#define SKIP_WHITE_SPACE(input) {                                        \
   while (isspace (*input)) {                                            \
      input++;                                                           \
   }                                                                     \
}

/*------------------------------------------------------------------------------
 * Error return if at end of line
 */
#define QUIT_ON_EOL(input) {                                             \
   if (*input == '\0') {                                                 \
      printf ("%s:%d premature end of line.\n", data_source, line_num);  \
      return false;                                                      \
   }                                                                     \
}

/*------------------------------------------------------------------------------
 * Skip white space - return of end of line
 */
#define SKIP_WHITE_QUIT_ON_EOL(input) {                                  \
   SKIP_WHITE_SPACE (input);                                             \
   QUIT_ON_EOL (input);                                                  \
}

/*------------------------------------------------------------------------------
 * Input format is general, e.g.  123, 0x123, 456.67, 32.99e+8, Text, "text".
 */
static bool parse_value (char *input, Variant_Value * data, char **endptr,
                         const char *data_source, const int line_num)
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
      printf ("%s:%d  extracted item %s\n", data_source, line_num, item);
   }

   /* check for a quoted string.
    */
   if (item[0] == '"') {
      /* exclude leading/trailing quotes
       */
      n = n - 2;
      if (n > MAX_STRING_SIZE) {
         printf ("%s:%d quoted string too big: %s\n", data_source, line_num,
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
      printf ("%s:%d un-quoted string too big: %s\n", data_source, line_num,
              item);
      return false;
   }

   strncpy (data->value.sval, item, n);
   data->value.sval[n] = '\0';

   data->kind = vkString;
   return true;
}                               /* scan_value */

/*------------------------------------------------------------------------------
 * Valid format is
 *    value or
 *    value ~ value or
 *    op value where op is <=, >=, = , /= <, >
 */
static bool parse_match (char *line, Variant_Range* item, char **endptr,
                         const char *data_source, const int line_num)
{
   /* Must be consistant with Comparision_Kind
    */
   static const char* comparison_operators [7] = {
      "", "/=", "<=", ">=", "=", "<", ">"
   };

   /* Ensure not erroneous.
    */
   item->comp = ckVoid;
   item->lower.kind = vkVoid;
   item->upper.kind = vkVoid;

   bool status;
   char *source = line;

   /* Skip white space (if any) - error return if at end of line.
    */
   SKIP_WHITE_QUIT_ON_EOL (source);


   /* Check for equality operator
    */
   Comparision_Kind e;

   for (e = 1; e <= 6; e++) {
      size_t n = strlen (comparison_operators [e]);

      if (strncmp (source, comparison_operators [e], n) == 0) {
         /* found an operator
          */
         item->comp = e;

         source += n; /* skip the operator */

         SKIP_WHITE_QUIT_ON_EOL (source);

         status = parse_value
             (source, &item->lower, endptr, data_source, line_num);
         return status;
      }
   }

   /* must be value or value ~ value.
    */
   status = parse_value
       (source, &item->lower, endptr, data_source, line_num);
   if (status == false) {
      return false;
   }
   source = *endptr;

   SKIP_WHITE_QUIT_ON_EOL (source);

   /* Range operator ? */
   if (*source == '~') {
      source++;
      SKIP_WHITE_QUIT_ON_EOL (source);
      status = parse_value
          (source, &item->upper, endptr, data_source, line_num);
      if (status == false) {
         return false;
      }
      item->comp = ckRange;
   } else {
      item->comp = ckEqual;
   }

   return true;
}

/*------------------------------------------------------------------------------
 * pv_name and command must be large enough.
 * filename and line_num used for error reports
 */
static bool parse_line (char *line, char *pv_name, int *index,
                        Variant_Range_Collection * pVRC, char *command,
                        const char *data_source, const int line_num)
{
   static const char* type_mis_match =
       "%s:%d warning: %s value of sub-match %d is %s, expecting %s.\n";

   char *source;
   char *target;
   char *start;
   char *finish;
   char item[MAX_LINE_LENGTH];
   char *endptr;
   bool status;
   int j;
   Variant_Kind expected;
   Variant_Kind lower_kind;
   Variant_Kind upper_kind;
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
      printf ("%s:%d error invalid PV name: %s\n", data_source, line_num,
              pv_name);
      return false;
   }

   if (debug > 4) {
      printf ("%s:%d  pv = %s\n", data_source, line_num, pv_name);
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
         printf ("%s:%d error missing ']'\n", data_source, line_num);
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
                 data_source, line_num, item);
         return false;
      }

      if (debug > 4) {
         printf ("%s:%d [%s] index = %d\n", data_source, line_num, item,
                 *index);
      }

      if (*index < 1) {
         printf ("%s:%d error invalid PV index: %d\n", data_source, line_num,
                 *index);
         return false;
      }

      if (*index > 1000) {
         printf ("%s:%d query valid PV index: %d ???\n", data_source,
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

      status = parse_match (source, &pVRC->item[j], &endptr, data_source, line_num);
      if (status == false) {
         return false;
      }
      source = endptr;

      /* Update number of valid entries so far
       */
      pVRC->count = j + 1;

      /* Are value kinds consistent?
       */
      expected = pVRC->item[0].lower.kind;
      lower_kind = pVRC->item[j].lower.kind;
      upper_kind = pVRC->item[j].upper.kind;

      /* Never fails for zeroth lower entry
       */
      if (lower_kind != expected) {
         printf (type_mis_match, data_source, line_num,
                 (pVRC->item[j].comp == ckRange ? "1st" : "the"), pVRC->count,
                 vkImage (lower_kind), vkImage (expected));
      }

      if ((pVRC->item[j].comp == ckRange) && (upper_kind != expected)) {
         printf (type_mis_match, data_source, line_num,
                 "2nd", pVRC->count,
                 vkImage (upper_kind), vkImage (expected));
      }

      SKIP_WHITE_QUIT_ON_EOL (source);
      if (*source != '|') {
         break;
      }
      source++;                 /* skip the '|' */

      if (pVRC->count >= NUMBER_OF_VARIENT_RANGES) {
         printf ("%s:%d error attempting to specify more than %d sub-matches\n",
                 data_source, line_num, NUMBER_OF_VARIENT_RANGES);
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
static bool Scan_Configuration (FILE *input_file,
                                const char* data_source,
                                const Allocate_Client_Handle allocate)
{
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
   Variant_Range_Collection match_set_collection;
   bool status;
   char *source;

   /* Read lines from file
    */
   line_num = 0;
   while (fgets (line, MAX_LINE_LENGTH, input_file)) {

      line_num++;

      /* Remove trailing \n char if it exists.
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

      /* Split line into sublines using ';' character - strtok not smart enough.
       */
      while (*source != '\0') {
         char* sub_line = source; /* save where we parse from */

         /* TODO: check if ; within quotes */
         char* scan = source;
         while (*scan != '\0' && *scan != ';') scan++;

         char* next_source = scan;  /* end of string or ';' */
         if (*scan == ';') {
            /* terminate string and set up next source */
            *scan = '\0';
            next_source++;
         }
         source = next_source;

         status = parse_line (sub_line, pv_name, &index, &match_set_collection,
                              command, data_source, line_num);

         if (status == false) {
            /* Any errors already reported - just print whole line.
             */
            printf ("%s:%d %s\n", data_source, line_num, sub_line);
            continue;
         }

         /* Check sizes
          */
         if (strlen (pv_name) > sizeof (pClient->pv_name) - 1) {
            printf ("%s:%d pv name too long\n", data_source, line_num);
            printf ("%s:%d %s\n", data_source, line_num, sub_line);
            continue;
         }

         if (strlen (command) > sizeof (pClient->match_command) - 1) {
            printf ("%s:%d command too long\n", data_source, line_num);
            printf ("%s:%d %s\n", data_source, line_num, sub_line);
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
      }
   }

   if (debug > 0) {
      printf ("%s: exit: data='%s'.\n", __FUNCTION__, data_source);
   }

   return true;
}                               /* Scan_Configuration */


/*------------------------------------------------------------------------------
 */
bool Scan_Configuration_File (const char *filename,
                               const Allocate_Client_Handle allocate)
{
   bool result;
   FILE* f;

   if (debug > 0) {
      printf ("%s: entry: filename='%s'.\n", __FUNCTION__, filename);
   }

   /* Attempt to open the specified file.
    */
   f = fopen (filename, "r");
   if (!f) {
      printf ("%s: unable to open file %s.\n", __FUNCTION__, filename);
      return false;
   }

   result = Scan_Configuration (f, filename, allocate);

   /* Close file
    */
   (void) fclose (f);

   return result;
}

/*------------------------------------------------------------------------------
 */
bool Scan_Configuration_String (const char *buffer,
                                const size_t size,
                                const Allocate_Client_Handle allocate)
{
   bool result;
   FILE* f;

   /* Read only mode  - allow casting from const */
   f = fmemopen ((char *)buffer, size, "r");

   if (!f) {
      printf ("%s: unable to open memory stream.\n", __FUNCTION__);
      return false;
   }
   result = Scan_Configuration (f, "memory buffer", allocate);

   /* Close file
    */
   (void) fclose (f);

   return result;
}

/* end */
