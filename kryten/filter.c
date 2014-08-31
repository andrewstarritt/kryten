/* $File: //depot/sw/epics/kryten/filter.c $
 * $Revision: #11 $
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
#include <stdlib.h>

#include "filter.h"
#include "utilities.h"

#define VALUE_IMAGE_SIZE 44
#define STATE_IMAGE_SIZE 12
#define INDEX_IMAGE_SIZE  8

/*------------------------------------------------------------------------------
 */
static bool check_in_range (const Varient_Value * value,
                            const Varient_Range * range)
{
   return Varient_Le (&range->lower, value)
       && Varient_Le (value, &range->upper);
}                               /* check_in_range */

/*------------------------------------------------------------------------------
 */
static void call_command (CA_Client * pClient, const char *state_image,
                          const char *value_image)
{
   char command[sizeof (pClient->match_command) +
                sizeof (pClient->pv_name) + STATE_IMAGE_SIZE +
                VALUE_IMAGE_SIZE + INDEX_IMAGE_SIZE + 12];
   int status;

   snprintf (command, sizeof (command), "%s %s '%s' '%s' %d &",
             pClient->match_command, pClient->pv_name, state_image,
             value_image, pClient->element_index);

   if (is_verbose) {
      printf ("calling system (\"%s\")\n", command);
   }
   status = system (command);
   if (status != 0) {
      printf ("system (\"%s\") returned %d\n", command, status);
   }
}                               /* call_command */


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
      if (check_in_range
          (&pClient->data, &pClient->match_set_collection.item[j])) {
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
      state_image = (matches == TRUE) ? "entry" : "exit ";

      Varient_Image (value_image, sizeof (value_image), &pClient->data);

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
