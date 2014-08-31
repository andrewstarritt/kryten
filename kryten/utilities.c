/* $File: //depot/sw/epics/kryten/utilities.c $
 * $Revision: #10 $
 * $DateTime: 2012/02/26 16:10:02 $
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
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

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
char * extract (char *dest, const size_t n, const char *src, const char *upto)
{
   size_t m;

   m = (long) upto - (long) src;

   /* Truncate if necessary - leave room for the trailing '\0'
    */
   if (m > n - 1) {
      m = n - 1;
   }

   strncpy (dest, src, m);
   dest [m] = '\0';

   return dest;
}                               /* extract */


/*------------------------------------------------------------------------------
 */
char *substitute (char *dest, const size_t n, const char *src,
                  const char *find, const char *replace)
{
   const size_t find_len = strlen (find);
   const size_t replace_len = strlen (replace);

   char *src_find;
   char *from;
   size_t max_room;
   size_t upto_from_len;
   size_t copy_len;


   if (find_len <= 0) {
      strncpy (dest, src, n - 1);
      dest[n - 1] = '\0';
      return dest;
   }

   *dest = '\0';

   from = (char *) src;
   src_find = strstr (from, (char *) find);
   while (src_find != NULL) {

      max_room = n - 1 - strlen (dest);
      if (max_room <= 0)
         break;

      upto_from_len = (size_t) src_find - (size_t) from;
      copy_len = MIN (max_room, upto_from_len);
      strncat (dest, from, copy_len);

      max_room = n - 1 - strlen (dest);
      if (max_room <= 0)
         break;

      copy_len = MIN (max_room, replace_len);
      strncat (dest, replace, copy_len);

      /* Move past fins string 
       */
      from = src_find + find_len;
      src_find = strstr (from, (char *) find);
   }

   max_room = n - 1 - strlen (dest);
   copy_len = MIN (max_room, strlen (from));

   strncat (dest, from, copy_len);
   dest[n - 1] = '\0';
   return dest;
}                               /* substitute */


/*------------------------------------------------------------------------------
 */
long long_value (const char *image, bool * status)
{
   long result = 0;
   size_t len;
   char *end_ptr = NULL;
   char *read_ptr = NULL;

   /* Ensure not erroneous - hypothesize not successful.
    */
   *status = false;

   if (image) {

      len = strlen (image);
      end_ptr = (char *) image + len;
      while ((end_ptr > image) && (isspace (end_ptr[-1]))) {
         end_ptr--;
      }

      if (end_ptr > image) {

         /* Try decimal number first
          */
         errno = 0;
         result = strtol (image, &read_ptr, 10);
         /* No error and strtol 'consumed' all the data
          */
         if ((errno == 0) && (read_ptr == end_ptr)) {
            *status = true;
         } else {
            /* Try hexdecimal number next
             */
            errno = 0;
            result = strtol (image, &read_ptr, 16);
            /* No error and strtol 'consumed' all the data
             */
            if ((errno == 0) && (read_ptr == end_ptr)) {
               *status = true;
            }
         }
      }
   }

   if (!*status) {
      /* Set default if function failed.
       */
      result = 0;
   }

   return result;
}                               /* long_value */

/*------------------------------------------------------------------------------
 */
double double_value (const char *image, bool * status)
{
   double result = 0.0;
   bool int_status;
   size_t len;
   char *end_ptr = NULL;
   char *read_ptr = NULL;

   /* Ensure not erroneous - hypothesize not successful.
    */
   *status = false;

   /* Check if a valid integer - strtod is too slack.
    */
   long_value (image, &int_status);

   if (!int_status) {

      /* Check for double
       */
      if (image) {

         len = strlen (image);
         end_ptr = (char *) image + len;
         while ((end_ptr > image) && (isspace (end_ptr[-1]))) {
            end_ptr--;
         }

         if (end_ptr > image) {

            errno = 0;
            result = strtod (image, &read_ptr);

            /* No error and strtod 'consumed' all the data
             */
            if ((errno == 0) && (read_ptr == end_ptr)) {
               *status = true;
            }
         }
      }
   }

   if (!*status) {
      /* Set default if function failed.
       */
      result = 0.0;
   }

   return result;
}                               /* double_value */

/*------------------------------------------------------------------------------
 */
long get_long_env (const char *name, bool * status)
{
   long result = 0;
   char *image;

   /* Ensure not erroneous - hypothesize not successful.
    */
   result = 0;
   *status = false;

   if (name) {
      image = getenv (name);
      if (image) {
         result = long_value (image, status);
      }
   }

   if (!*status) {
      /* Set default if function failed.
       */
      result = 0;
   }

   return result;
}                               /* get_int_env */


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
