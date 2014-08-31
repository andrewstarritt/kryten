/* $File: //depot/sw/epics/kryten/utilities.c $
 * $Revision: #8 $
 * $DateTime: 2012/02/24 23:15:17 $
 * $Author: andrew $
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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "utilities.h"

const char *red = "\033[31;1m";
const char *green = "\033[32;1m";
const char *yellow = "\033[33;1m";
const char *gray = "\033[37;1m";
const char *reset = "\033[00m";


/*------------------------------------------------------------------------------
 */
bool is_either (const char *s, const char *s1, const char *s2)
{
   return ((strcmp (s, s1) == 0) || (strcmp (s, s2) == 0)) ? true : false;
}                               /* is_either */


/*------------------------------------------------------------------------------
 */
void check_argument (const char *arg, const char *name,
                     bool * matches, bool * is_found, const char **value)
{
   char prefix[120];
   size_t n;

   (void) snprintf (prefix, sizeof (prefix), "--%s=", name);
   n = strlen (prefix);
   if (strncmp (arg, prefix, n) == 0) {
      *matches = true;
      if (*is_found == false) {
         *value = &arg[n];
         *is_found = true;
      } else {
         printf ("%sWarning:%s secondary %s option ignored\n",
                 yellow, gray, prefix);
      }
   }
}                               /* check_argument */


/*------------------------------------------------------------------------------
 */
void check_flag (const char *arg, const char *name1, const char *name2,
                 bool * matches, bool * is_found)
{
   if (is_either (arg, name1, name2)) {
      *matches = true;
      if (*is_found == false) {
         *is_found = true;
      } else {
         printf ("%sWarning:%s secondary %s/%s option ignored\n",
                 yellow, reset, name1, name2);
      }
   }

}                               /* check_flag */


/*------------------------------------------------------------------------------
 */
const char *vkImage (const Varient_Kind kind)
{
   switch (kind) {
      case vkVoid:
         return "void";

      case vkString:
         return "string";

      case vkInteger:
         return "integer";

      case vkFloating:
         return "floating";
   }

   return "unknown var kind";
}                               /* vkImage */


/*------------------------------------------------------------------------------
 */
bool Varient_Same (const Varient_Value * left, const Varient_Value * right)
{
   bool result = false;
   bool error = false;
   int t;

   if (left->kind == right->kind) {

      switch (right->kind) {
         case vkVoid:
            result = true;
            break;

         case vkString:
            t = strncmp (left->value.sval, right->value.sval,
                         sizeof (left->value.sval));
            result = (t == 0);
            break;

         case vkFloating:
            result = (left->value.dval == right->value.dval);
            break;

         case vkInteger:
            result = (left->value.ival == right->value.ival);
            break;

         default:
            result = false;
            error = true;
            break;
      }

   } else {
      result = false;
   }

   if (error) {
      printf ("Varient_Same error: left kind: %d, right kind: %d\n",
              (int) left->kind, (int) right->kind);
   }

   return result;
}                               /* Varient_Same */

/*------------------------------------------------------------------------------
 */
bool Varient_Le (const Varient_Value * left, const Varient_Value * right)
{
   bool result = false;
   bool error = false;
   int t;

   switch (left->kind) {
      case vkVoid:
         error = true;
         break;

      case vkString:
         switch (right->kind) {
            case vkVoid:
               error = true;
               break;

            case vkString:
               t = strncmp (left->value.sval, right->value.sval,
                            sizeof (left->value.sval));
               result = (t <= 0);
               break;

            case vkFloating:
               result = (atof (left->value.sval) <= right->value.dval);
               break;

            case vkInteger:
               result = (atol (left->value.sval) <= right->value.ival);
               break;

            default:
               error = true;
               break;
         }
         break;

      case vkFloating:
         switch (right->kind) {
            case vkVoid:
               error = true;
               break;

            case vkString:
               result = (left->value.dval <= atof (right->value.sval));
               break;

            case vkFloating:
               result = (left->value.dval <= right->value.dval);
               break;

            case vkInteger:
               result = (left->value.dval <= (double) right->value.ival);
               break;

            default:
               error = true;
               break;
         }
         break;

      case vkInteger:
         switch (right->kind) {
            case vkVoid:
               error = true;
               break;

            case vkString:
               result = (left->value.ival <= atol (right->value.sval));
               break;

            case vkFloating:
               result = ((double) left->value.ival <= right->value.dval);
               break;

            case vkInteger:
               result = (left->value.ival <= right->value.ival);
               break;

            default:
               error = true;
               break;
         }
         break;


      default:
         error = true;
         break;
   }

   if (error) {
      printf ("Varient_Le error: left kind: %d, right kind: %d\n",
              (int) left->kind, (int) right->kind);
   }

   return result;
}                               /* Varient_Le */

/*------------------------------------------------------------------------------
 */
int Varient_Image (char *str, size_t size, const Varient_Value * item)
{
   int result;
   double temp;

   switch (item->kind) {
      case vkVoid:
         result = -1;
         break;

      case vkString:
         /* We may need to check if sval contains any quotes
          */
         result = snprintf (str, size, "%s", item->value.sval);
         break;

      case vkFloating:
         temp = fabs (item->value.dval);
         if ((temp == 0.0) || ((temp >= 0.1) && (temp <= 1.0e6))) {
            result = snprintf (str, size, "%.3f", item->value.dval);
         } else {
            result = snprintf (str, size, "%.6e", item->value.dval);
         }
         break;

      case vkInteger:
         result = snprintf (str, size, "%ld", item->value.ival);
         break;

      default:
         result = -1;
         break;
   }
   return result;
}

/* end */
