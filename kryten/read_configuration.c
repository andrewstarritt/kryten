/* $File: //depot/sw/epics/kryten/read_configuration.c $
 * $Revision: #7 $
 * $DateTime: 2011/05/22 15:50:28 $
 * Last checked in by: $Author: andrew $
 *
 * Description:
 * Kryten is a EPICS PV monitoring program that calls a system command
 * when the value of the PV matches/cease to match specified criteria.
 *
 * Copyright (C) 2011  Andrew C. Starritt
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
 * indent options:  -kr -pcs -i3 -cli3 -nut
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

static int debug = 0;

/* ------------------------------------------------------------------------------
 * Pre-process line - removes leading white space and converts subsequenct white
 * space groups to a single space except those between double quotes (").
 */
static void pre_process (char *input)
{
   size_t n;
   char *target;
   bool in_string;
   int count;
   size_t j;
   int k;
   char c;

   n = strlen (input);
   target = input;
   in_string = false;
   count = 1;                   /* force removal all leading white space */
   k = 0;                       /* k always <= j */
   for (j = 0; j <= n; j++) {   /* note: j <= n,  include trailing nul */
      c = input[j];

      if (c == '"') {
         in_string = !in_string;
      }

      if ((!in_string) && isspace (c)) {
         count++;
         if (count == 1) {
            target[k] = c;
            k++;
         }
      } else {
         count = 0;
         target[k] = c;
         k++;
      }
   }

   /* Trim trailing spaces as well, if any
    */
   n = strlen (input);
   while ((n > 0) && (isspace (input[n - 1]))) {
      n--;
   }
   input[n] = '\0';
}                               /* pre_process */


/*------------------------------------------------------------------------------
 * Parses input string into upto max_items sub-strings using given delimiter.
 * The sub strings are placed into the supplied buffer.
 * Note: length of buffer MUST be >= length of input.
 */
static bool parse (const char *input, const char delimiter,
                   const unsigned int max_items, char *buffer,
                   char *sub_items[], unsigned int *count)
{
   const char *function = "parse";

   bool in_string;
   size_t len;
   size_t j;
   char c;
   unsigned int i;

   /* Ensure not erroneous.
    */
   *count = 0;

   /* Create copy
    */
   strncpy (buffer, input, MAX_LINE_LENGTH);

   if (*count >= max_items) {
      if (debug > 2) {
         printf ("%s(%c): too many (%u) items: %s\n", function, delimiter,
                 max_items, input);
      }
      return false;
   }
   sub_items[0] = &buffer[0];
   (*count)++;

   in_string = false;
   len = strlen (buffer);
   for (j = 0; j < len; j++) {
      c = buffer[j];

      if ((!in_string) && (c == delimiter)) {
         if (*count >= max_items) {
            if (debug > 2) {
               printf ("%s(%c): too many (%u >) items: %s\n", function,
                       delimiter, max_items, input);
            }
            return false;
         }
         buffer[j] = '\0';
         sub_items[*count] = &buffer[j + 1];
         (*count)++;

      } else if (c == '"') {
         in_string = !in_string;
      }
   }

   if (debug > 4) {
      for (i = 0; i < *count; i++) {
         printf ("%s(%c): %2u  %s\n", function, delimiter, i,
                 sub_items[i]);
      }
   }

   return true;
}                               /* parse */


/*------------------------------------------------------------------------------
 * Input format is general, e.g.  123, 0x123, 456.67, 32.99e+8, Text, "text".
 */
static bool scan_value (char *input, Varient_Value * data)
{
   char input_copy[MAX_LINE_LENGTH + 1];
   char *item;
   size_t len;
   char *endptr;
   bool okay;

   /* Ensure not erroneous.
    */
   data->kind = vkVoid;

   /* Create working copy of source input
    */
   strncpy (input_copy, input, sizeof (input_copy) - 1);

   item = input_copy;

   /* Trim input so that we can check that strtol/strtod consumes all
    * the input, e.g. verify '12Q' is not an integer value 12
    */
   while (isspace (*item)) {
      item++;
   }

   len = strlen (item);
   while ((len > 0) && (isspace (item[len - 1]))) {
      len--;
   }
   item[len] = '\0';

   /* Need to check that len is non-zero as strtol("", ...) and
    * strtod("", ...) does consume all input, but does not set errno.
    *
    * Ada's  double'value ("")  would be really good here.
    */
   if (len > 0) {

      okay = false;

      if (!okay) {
         errno = 0;
         data->value.ival = strtol (item, &endptr, 10);
         if ((errno == 0) && (endptr == item + len)) {
            data->kind = vkInteger;
            okay = true;
         }
      }

      if (!okay) {
         errno = 0;
         data->value.ival = strtol (item, &endptr, 16);
         if ((errno == 0) && (endptr == item + len)) {
            data->kind = vkInteger;
            okay = true;
         }
      }

      if (!okay) {
         errno = 0;
         data->value.dval = strtod (item, &endptr);
         if ((errno == 0) && (endptr == item + len)) {
            data->kind = vkFloating;
            okay = true;
         }
      }

      if (!okay) {
         /* De quote string if necessary
          */
         if (*item == '"') {
            item++;
            len--;

            if ((len > 0) && (item[len - 1] == '"')) {
               len--;
               item[len] = '\0';
            }
         }

         strncpy (data->value.sval, item, sizeof (data->value.sval));
         data->kind = vkString;
      }

   } else {
      printf ("'%s' is a null string\n", item);
      data->value.sval[0] = '\0';
      data->kind = vkString;
   }

   if (debug > 0) {
      char buffer[120] = "";
      (void) Varient_Image (buffer, sizeof (buffer) - 1, data);
      printf ("%s\n", buffer);
   }
   return true;
}                               /* scan_value */


/*------------------------------------------------------------------------------
 * Input format is VALUE or VALUE:VALUE
 */
static bool scan_value_range (const char *input, Varient_Range * match_set)
{
   char parse_buffer[MAX_LINE_LENGTH + 1] = "";
   char *items[2] = { "", "" };
   unsigned int count = 0;
   bool status;

   status = parse (input, '~', 2, parse_buffer, items, &count);
   if (status) {
      status = scan_value (items[0], &(match_set->lower));
   }

   if (status) {
      if (count == 2) {
         status = scan_value (items[1], &(match_set->upper));
      } else {
         /* Just set upper value the same as the lower value.
          */
         match_set->upper = match_set->lower;
      }
   }

   return status;
}                               /* scan_value_range */


/*------------------------------------------------------------------------------
 * Input format is   VALUE_ITEM[,VALUE_ITEM[,VALUE_ITEM,[...]]]  upto 16 items
 * Value Item format is VALUE or VALUE:VALUE
 */
static bool scan_matches (const char *input,
                          Varient_Range_Collection * pVRC,
                          const char *filename, const int line_num)
{
   /* static const char *function = "scan_matches"; */

   char parse_buffer[MAX_LINE_LENGTH + 1] = "";
   char *items[NUMBER_OF_VARIENT_RANGES] =
       { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "" };
   unsigned int count = 0;
   bool status;
   unsigned int j;
   Varient_Kind expected;
   Varient_Kind lower_kind;
   Varient_Kind upper_kind;

   /* Ensure not erroneous.
    */
   pVRC->count = 0;

   status = parse (input, '|', NUMBER_OF_VARIENT_RANGES,
                   parse_buffer, items, &count);

   status = (count >= 1);

   if (status) {
      for (j = 0; j < count; j++) {
         status = scan_value_range (items[j], &pVRC->item[j]);
         if (!status) {
            break;
         }

         expected = pVRC->item[0].lower.kind;
         lower_kind = pVRC->item[j].lower.kind;
         upper_kind = pVRC->item[j].upper.kind;

         if (lower_kind == upper_kind) {
            /* Okay or generate warning together */
            if (lower_kind != expected) {
               printf
                   ("%s:%d warning type mis-match: both values of '%s' are %s, expecting %s.\n",
                    filename, line_num, items[j], vkImage (lower_kind),
                    vkImage (expected));
            }
         } else {
            /* Never fails for zeroth entry
             */
            if (lower_kind != expected) {
               printf
                   ("%s:%d warning type mis-match: lower value of '%s' is %s, expecting %s.\n",
                    filename, line_num, items[j], vkImage (lower_kind),
                    vkImage (expected));
            }

            if (upper_kind != expected) {
               printf
                   ("%s:%d warning type mis-match: upper value of '%s' is %s, expecting %s.\n",
                    filename, line_num, items[j], vkImage (upper_kind),
                    vkImage (expected));
            }
         }
         /* Update number of valid entries so far
          */
         pVRC->count = j + 1;
      }
   }

   return status;
}                               /* scan_matches */


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
   size_t n;
   char parse_buffer[MAX_LINE_LENGTH + 1] = "";
   char *items[6] = { "", "", "", "", "", "" };
   unsigned int count = 0;
   char *pv_name;
   char *matches;
   char *command;

   bool status;
   bool pv_name_is_okay;
   bool matches_is_okay;
   Varient_Range_Collection match_set_collection;


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

      /* Remove trailing line feed if needs be.
       */
      n = strlen (line);
      if ((n >= 1) && (line[n - 1] == '\n')) {
         n--;
         line[n] = '\0';
      }

      /* Pre-process line, remove leading white space and convert subsequent
       * un quotes white space groups to a single space.
       */
      pre_process (line);

      /* Ignore empty lines and comment lines.
       */
      if ((line[0] == '\0') || (line[0] == '#')) {
         continue;
      }

      /* Attempt to read name, match critera and command.
       */
      status = parse (line, ' ', 6, parse_buffer, items, &count);
      if (!status) {
         continue;
      }

      if (count != 3) {
         printf
             ("%s:%d warning skipping: '%s', %u items found, but expecting 3\n",
              filename, line_num, line, count);
         continue;
      }

      /* Set up aray aliases
       */
      pv_name = items[0];
      matches = items[1];
      command = items[2];

      /* Check for sensible channel name, check it starts okay at least.
       */
      pv_name_is_okay = isalnum (pv_name[0]) || pv_name[0] == '$';

      /* Check for sensible match criteria.
       */
      match_set_collection.count = 0;
      matches_is_okay =
          scan_matches (matches, &match_set_collection, filename,
                        line_num);

      if (pv_name_is_okay && matches_is_okay) {

         if (debug >= 2) {
            printf ("processing PV: %s\n", pv_name);
         }

         pClient = Allocate_Client ();
         strncpy (pClient->pv_name, pv_name, sizeof (pClient->pv_name));
         strncpy (pClient->match_command, command, MATCH_COMMAND_LENGTH);

         pClient->match_set_collection = match_set_collection;

         /* Lastly add to client list.
          */
         if (pClient) {
            ellAdd (ca_client_list, (ELLNODE *) pClient);
         }
         continue;
      }

      /* Ignore anything left over
       */
      printf ("%s:%d warning skipping: '%s'\n", filename, line_num, line);
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
