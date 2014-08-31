/* $File: //depot/sw/opi/c/buffered_callbacks.c $
 * $Revision: #6 $
 * $DateTime: 2011/05/09 23:29:29 $
 * $Author: andrew $
 *
 * EPICS buffered callback module for use with Ada, Lazarus and other
 * runtime environments which don't like alien threads.
 *
 * Copyright (C) 2005-2011  Andrew C. Starritt
 *
 * This library is free software: you can redistribute it and/or modify
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
 *
 * Description:
 * The module buffers the channel access callbacks.
 * The registered callback handlers store a copy of the call back data.
 * The actual call backs are initiated via process_buffered_callbacks
 * from a native application thread.
 *
 * This ensures that the actual call back runs within an application
 * thread as opposed to a libca.so shared library thread.
 *
 * Rational: The main application runtime environment does not allow
 * code to do anything significant if running within an alien thread.
 *
 * Source code formatting:
 *    indent -kr -pcs -i3 -cli3 -nut
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cadef.h>
#include <caerr.h>
#include <ellLib.h>
#include <epicsMutex.h>

#include "buffered_callbacks.h"

/* These functions must be exported by the main application.
 *
 * Note: unlike the equivelent raw channel access callback functions,
 *       these functions always take POINTER arguments.
 */
extern void application_connection_handler (struct connection_handler_args
                                            *ptr);

extern void application_event_handler (struct event_handler_args *ptr);

extern void application_printf_handler (char *formatted_text);


/* -----------------------------------------------------------------------------
 * PRIVATE - implementation details
 * -----------------------------------------------------------------------------
 */

/* Buffered stuff kinds 
 */
typedef enum eCallback_Kinds {
   NULL_KIND,
   CONNECTION,
   EVENT,
   PRINTF,
} Callback_Kinds;


typedef struct sCallback_Items {
   ELLNODE ellnode;
   Callback_Kinds kind;

   /* perhaps we could use a union here
    */
   struct connection_handler_args cargs;
   struct event_handler_args eargs;
   char *formatted_text;
} Callback_Items;

static epicsMutexId linked_list_mutex = NULL;
static ELLLIST linked_list;


/* -----------------------------------------------------------------------------
 * free_element - frees all dynamic data associated with this element.
 */
static void free_element (Callback_Items * pci)
{
   switch (pci->kind) {

      case CONNECTION:
         /* No special action */
         break;

      case EVENT:
         if (pci->eargs.dbr) {
            free ((void *) pci->eargs.dbr);
            pci->eargs.dbr = NULL;
         }
         break;

      case PRINTF:
         /* No special action */
         if (pci->formatted_text) {
            free (pci->formatted_text);
         }
         break;

      default:
         /* What the .... ? */
         break;

   }

   free (pci);
}                               /* free_element */


/* -----------------------------------------------------------------------------
 * load  - when no room, item is lost.
 */
static void load_element (Callback_Items * pci)
{
   /* Gain exclusive access to card array
    */
   epicsMutexLock (linked_list_mutex);

   ellAdd (&linked_list, (ELLNODE *) pci);

   /* Release exclusive access to card array
    */
   epicsMutexUnlock (linked_list_mutex);
}                               /* load_element */


/* -----------------------------------------------------------------------------
 * unload - is NULL of nothing in the list.
 */
static Callback_Items *unload_element ()
{
   Callback_Items *result;

   /* Gain exclusive access to card array
    */
   epicsMutexLock (linked_list_mutex);

   result = (Callback_Items *) ellGet (&linked_list);

   /* Release exclusive access to card array
    */
   epicsMutexUnlock (linked_list_mutex);

   return result;
}                               /* unload_element */



/* -----------------------------------------------------------------------------
 * Connection handler
 */
void buffered_connection_handler (struct connection_handler_args args)
{
   Callback_Items *pci;

   pci = (Callback_Items *) malloc (sizeof (Callback_Items));
   pci->kind = CONNECTION;

   /* Copy all fields. */
   pci->cargs = args;

   load_element (pci);
}                               /* buffered_connection_handler */


/* -----------------------------------------------------------------------------
 * Event handler
 */
void buffered_event_handler (struct event_handler_args args)
{
   Callback_Items *pci;
   size_t size;

   pci = (Callback_Items *) malloc (sizeof (Callback_Items));
   pci->kind = EVENT;

   /* Copy all fields. */
   pci->eargs = args;

   /* Calculate size of dbr field, and alloc memory for copy iff required
    */
   if (args.dbr != NULL) {
      size = dbr_size_n (args.type, args.count);
      pci->eargs.dbr = malloc (size);
      memcpy ((void *) pci->eargs.dbr, args.dbr, size);
   }

   load_element (pci);
}                               /* buffered_event_handler */


/* -----------------------------------------------------------------------------
 * Replacement printf handler
 */
int buffered_printf_handler (const char *pformat, va_list args)
{
   Callback_Items *pci;
   /* Expanded strings never more than 80, so 400 is ample */
   char expanded[400];
   size_t size;

   pci = (Callback_Items *) malloc (sizeof (Callback_Items));
   pci->kind = PRINTF;

   /* Expand string here - it's just easier.
    * It should be done in the libca.so
    */
   vsprintf (expanded, pformat, args);
   va_end (args);

   /* add one for \0 at end */

   size = strlen (expanded) + 1;

   pci->formatted_text = (char *) malloc (size);

   /* Copy expanded string 
    */
   memcpy ((void *) pci->formatted_text, &expanded, size);

   load_element (pci);

   return ECA_NORMAL;
}                               /* buffered_printf_handler */


/* -----------------------------------------------------------------------------
 */
void initialise_buffered_callbacks ()
{
   linked_list_mutex = epicsMutexCreate ();
   ellInit (&linked_list);
}                               /* initialise_buffered_callbacks */

/* -----------------------------------------------------------------------------
 */
int number_of_buffered_callbacks ()
{
   int n;
   n = ellCount (&linked_list);
   return n;
}                               /* number_of_buffered_callbacks */

/* -----------------------------------------------------------------------------
 * Process callbacks - called from application thread.
 */
int process_buffered_callbacks (const int max)
{
   Callback_Items *pci;
   int n;

   n = 0;
   while (1) {

      pci = unload_element ();

      if (pci == NULL) {
         break;
      }

      switch (pci->kind) {

         case CONNECTION:
            application_connection_handler (&pci->cargs);
            break;

         case EVENT:
            application_event_handler (&pci->eargs);
            break;

         case PRINTF:
            application_printf_handler (pci->formatted_text);
            break;

         default:
            fprintf (stderr, "*** Unexpected callback kind: %d \n",
                     pci->kind);
            break;
      }

      /* Free element 
       */
      free_element (pci);

      n++;
      if (n >= max) {
         break;
      }

   }                            /* end loop */

   return n;
}                               /* process_buffered_callbacks */

/* end */
