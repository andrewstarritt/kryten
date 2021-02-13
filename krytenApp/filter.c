/* $File: filter.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "filter.h"
#include "utilities.h"

#define VALUE_IMAGE_SIZE 44
#define STATE_IMAGE_SIZE 12
#define INDEX_IMAGE_SIZE 10

#define COMMAND_BUFFER_SIZE (MATCH_COMMAND_LENGTH + MAXIMUM_PVNAME_SIZE + \
                             STATE_IMAGE_SIZE + VALUE_IMAGE_SIZE +  \
                             INDEX_IMAGE_SIZE + 12)

/*------------------------------------------------------------------------------
 */
static void call_command (CA_Client * pClient, const char *state_image,
                          const char *value_image)
{
   /* We flip flop substituting from command to dnammoc to command
    */
   char dnammoc[COMMAND_BUFFER_SIZE];
   char command[COMMAND_BUFFER_SIZE];
   char q_val_image[VALUE_IMAGE_SIZE];
   char index_image[INDEX_IMAGE_SIZE];
   int status;

   /* Create quotted value and index images.
    */
   snprintf (q_val_image, sizeof (q_val_image), "'%s'", value_image);
   snprintf (index_image, sizeof (index_image), "%d",
             pClient->element_index);

   substitute (dnammoc, sizeof (dnammoc), pClient->match_command, "%p",
               pClient->pv_name);
   substitute (command, sizeof (command), dnammoc, "%e", index_image);
   substitute (dnammoc, sizeof (dnammoc), command, "%m", state_image);
   /** We do value last as the value itself may contain %p, %m and or %e.
    **/
   substitute (command, sizeof (command), dnammoc, "%v", q_val_image);

   /* Check for built in commands.
    */
   if (strncmp (command, "quit ", 5) == 0) {

      if (strncmp (state_image, "match", 5) == 0) {
         if (is_verbose) {
            printf ("builtin: %s\n", command);
         }
         quit_invoked = true;
         exit_code = atoi (&command [5]);
      }

   } else {

      if (is_verbose) {
         printf ("calling system (\"%s\")\n", command);
      }

      status = system (command);
      if (status != 0) {
         printf ("system (\"%s\") returned %d\n", command, status);
      }
   }
}                               /* call_command */


/*------------------------------------------------------------------------------
 */
static bool is_value_a_match (const Variant_Value * value,
                              const Variant_Range * range)
{
   bool result;

   switch (range->comp) {
      case ckRange:
         result = Variant_Le (&range->lower, value) &&
                  Variant_Le (value, &range->upper);
         break;

      case ckEqual:
         result = Variant_Eq (value, &range->lower);
         break;

      case ckNotEqual:
         result = Variant_Ne (value, &range->lower);
         break;

      case ckLessThan:
         result = Variant_Lt (value, &range->lower);
         break;

      case ckLessThanEqual:
         result = Variant_Le (value, &range->lower);
         break;

      case ckGreaterThan:
         result = Variant_Gt (value, &range->lower);
         break;

      case ckGreaterThanEqual:
         result = Variant_Ge (value, &range->lower);
         break;

      default:
         result = false;
   }
   return result;

}                               /* is_value_a_match */

/*------------------------------------------------------------------------------
 */
void Process_PV_Update (CA_Client * pClient)
{
   bool matches;
   unsigned int j;
   char value_image[VALUE_IMAGE_SIZE] = "";
   char *state_image;

   /* Hypothesize no match
    */
   matches = false;

   /* Check each range in-turn
    */
   for (j = 0; j < pClient->match_set_collection.count; j++) {
      if (is_value_a_match (&pClient->data,
                            &pClient->match_set_collection.item[j])) {
         /* Found a match
          */
         matches = true;
         break;
      }
   }

   /* Has match state changed?
    */
   if (pClient->last_update_matched != matches) {

      /** PV has entered or exited the matched state
       */
      state_image = (matches == TRUE) ? "match " : "reject";

      Variant_Image (value_image, sizeof (value_image), &pClient->data);

      call_command (pClient, state_image, value_image);
   }

   pClient->last_update_matched = matches;
}                               /* Process_PV_Update */


/*------------------------------------------------------------------------------
 */
void Process_PV_Disconnect (CA_Client * pClient)
{
   call_command (pClient, "disconnect", "");
}                               /* Process_PV_Disconnect */

/* end */
