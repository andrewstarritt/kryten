/* $File: //depot/sw/epics/kryten/pv_client.c $
 * $Revision: #21 $
 * $DateTime: 2015/11/01 19:16:00 $
 * Last checked in by: $Author: andrew $
 *
 * Description:
 * Kryten is a EPICS PV monitoring program that calls a system command
 * when the value of the PV matches/cease to match specified criteria.
 *
 * Copyright (C) 2011-2015  Andrew C. Starritt
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
#include <time.h>

#include <caerr.h>
#include <cantProceed.h>
#include <db_access.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <epicsTypes.h>

#include "buffered_callbacks.h"
#include "filter.h"
#include "pv_client.h"
#include "read_configuration.h"


/* EPICS timestamp epoch: This is Mon Jan  1 00:00:00 1990 UTC.
 *
 * This itself is expressed as a system time which represents the number
 * of seconds elapsed since 00:00:00 on January 1, 1970, UTC.
 */
static const time_t epics_epoch = 631152000;

/* Quasi enumeration variables - Channel Access passes back a
 * pointer to one of these as user data. It is the distinct address
 * as opposed to the content that is important.
 */
static int Get;
static int Event;
static int Put;

/* Debug and diagnostic variables.
 */
static int debug = 0;
static long start_time = 0;
static unsigned long cycle = 0;


/*------------------------------------------------------------------------------
 * PRIVATE FUNCTIONS
 *------------------------------------------------------------------------------
 */
static void Report (const char *text)
{
   printf ("%s\n", text);
}                               /* Report */


/*------------------------------------------------------------------------------
 */
static void Create_Channel (CA_Client * pClient)
{
   int status;

   pClient->is_connected = false;
   status = ca_create_channel
       (pClient->pv_name, buffered_connection_handler,
        pClient, 10, &pClient->channel_id);
   if (status != ECA_NORMAL) {
      printf ("ca_create_channel (%s) failed (%s)\n", pClient->pv_name,
              ca_message (status));
   }
}                               /* Create_Channel */


/*------------------------------------------------------------------------------
 * Get initial data and subscribe for updates.
 */
static void Subscribe_Channel (CA_Client * pClient)
{
   unsigned long count;
   Variant_Kind kind;
   chtype initial_type;
   chtype update_type;
   size_t size;
   unsigned long truncated;
   int status;

   count = pClient->element_count;
   if (count == 0) {
      printf ("element count (%s) is zero\n", pClient->pv_name);
      return;
   }

   /* Determine initial buffer request type and subscription buffer
    * request type, based on the first match criteria field type.
    */
   kind = pClient->match_set_collection.item[0].lower.kind;
   switch (kind) {

      case vkString:
         initial_type = DBR_STS_STRING;
         update_type = DBR_TIME_STRING;
         size = sizeof (dbr_string_t);
         break;

      case vkInteger:
         initial_type = DBR_CTRL_LONG;
         update_type = DBR_TIME_LONG;
         size = sizeof (dbr_long_t);
         break;

      case vkFloating:
         initial_type = DBR_CTRL_DOUBLE;
         update_type = DBR_TIME_DOUBLE;
         size = sizeof (dbr_double_t);
         break;

      default:
         printf ("%s: match type is invalid (%s)\n", pClient->pv_name,
                 vkImage (kind));
         return;
   }

   /* If the PV does not support request element, then do not read or
    * subscrible for data.
    */
   if (pClient->element_index > count) {
      printf
          ("%s has %lu elements, element %d not available\n",
           pClient->pv_name, count, pClient->element_index);
      return;
   }

   if (pClient->element_index < count) {
      truncated = pClient->element_index;

      printf
          ("%s array get/subscription truncated from %lu (size %lu) to %lu elements\n",
           pClient->pv_name, count, size, truncated);

      count = truncated;
   }

   /* Initial request
    */
   status = ca_array_get_callback
       (initial_type, count, pClient->channel_id,
        buffered_event_handler, &Get);

   if (status != ECA_NORMAL) {
      printf ("ca_array_get_callback (%s) failed (%s)\n", pClient->pv_name,
              ca_message (status));
      return;
   }

   /* ... and now subscribe for time stamped data updates as well.
    */
   status = ca_create_subscription
       (update_type, count, pClient->channel_id,
        DBE_LOG | DBE_ALARM, buffered_event_handler,
        &Event, &pClient->event_id);

   if (status != ECA_NORMAL) {
      printf ("ca_create_subscription (%s) failed (%s)\n",
              pClient->pv_name, ca_message (status));
   }

   pClient->is_first_update = true;

}                               /* Subscribe_Channel */


/*------------------------------------------------------------------------------
 * Processes received data
 *
 * Arguments passed to event handlers and get/put call back handlers.   
 *
 * The status field below is the CA ECA_XXX status of the requested
 * operation which is saved from when the operation was attempted in the
 * server and copied back to the clients call back routine.
 * If the status is not ECA_NORMAL then the dbr pointer will be NULL
 * and the requested operation can not be assumed to be successful.
 *
 *   typedef struct event_handler_args {
 *       void            *usr;    -- user argument supplied with request
 *       chanId          chid;    -- channel id
 *       long            type;    -- the type of the item returned
 *       long            count;   -- the element count of the item returned
 *       READONLY void   *dbr;    -- a pointer to the item returned
 *       int             status;  -- ECA_XXX status from the server
 *   } evargs;
 *
 */
static void Get_Event_Handler (CA_Client * pClient,
                               const struct event_handler_args *args)
{
   const char *function = "Get_Event_Handler";

/* Local "functons" that make use of naming regularity 
 */
#define ASSIGN_STATUS(from) {                                               \
   pClient->status = from.status;                                           \
   pClient->severity = from.severity;                                       \
}


/* Convert EPICS time to system time. 
 * EPICS is number secons since 01-Jan-1990 where as
 * System time is number secons since 01-Jan-1970. 
 */
#define ASSIGN_STATUS_AND_TIME(from) {                                      \
   pClient->data_element_count = args->count;                               \
   pClient->status = from.status;                                           \
   pClient->severity = from.severity;                                       \
   pClient->update_time = epics_epoch + from.stamp.secPastEpoch;            \
   pClient->nano_sec = from.stamp.nsec;                                     \
}



#define ASSIGN_NUMERIC(from, prec) {                                        \
   pClient->precision = prec;                                               \
   strcpy (pClient->units, pDbr->cfltval.units);                            \
   pClient->num_states = 0;                                                 \
   pClient->upper_disp_limit    = (double) from.upper_disp_limit;           \
   pClient->lower_disp_limit    = (double) from.lower_disp_limit;           \
   pClient->upper_alarm_limit   = (double) from.upper_alarm_limit;          \
   pClient->upper_warning_limit = (double) from.upper_warning_limit;        \
   pClient->lower_warning_limit = (double) from.lower_warning_limit;        \
   pClient->lower_alarm_limit   = (double) from.lower_alarm_limit;          \
   pClient->upper_ctrl_limit    = (double) from.upper_ctrl_limit;           \
   pClient->lower_ctrl_limit    = (double) from.lower_ctrl_limit;           \
}


#define CLEAR_NUMERIC {                                                     \
   pClient->precision = 0;                                                  \
   pClient->units[0] = '\0';                                                \
   pClient->num_states = 0;                                                 \
   pClient->upper_disp_limit    = 0.0;                                      \
   pClient->lower_disp_limit    = 0.0;                                      \
   pClient->upper_alarm_limit   = 0.0;                                      \
   pClient->upper_warning_limit = 0.0;                                      \
   pClient->lower_warning_limit = 0.0;                                      \
   pClient->lower_alarm_limit   = 0.0;                                      \
   pClient->upper_ctrl_limit    = 0.0;                                      \
   pClient->lower_ctrl_limit    = 0.0;                                      \
}


   const union db_access_val *pDbr = (union db_access_val *) args->dbr;
   int number;
   int e;
   Variant_Kind kind;
   bool enums_as_string;
   dbr_short_t enum_value;

   /* Get number of elements.
    */
   number = MAX (0, args->count);

   if (number < pClient->element_index) {
      printf
          ("%s (%s): received elements (%d) less than expected (%d) for buffer type %ld\n",
           function, pClient->pv_name, number, pClient->element_index,
           args->type);
      return;
   }
   /* Form aray access index 
    */
   e = number - 1;

   kind = pClient->match_set_collection.item[0].lower.kind;
   enums_as_string = (kind == vkString);

   switch (args->type) {

   /** Control updates all meta data plus values (ingnored) **/

      case DBR_STS_STRING:
         ASSIGN_STATUS (pDbr->sstrval);
         CLEAR_NUMERIC;
         pClient->data.kind = vkString;
         strncpy (pClient->data.value.sval,
                  (&pDbr->sstrval.value)[e], MAX_STRING_SIZE);
         pClient->data.value.sval[MAX_STRING_SIZE] = '\0';
         break;

      case DBR_CTRL_SHORT:
         ASSIGN_STATUS (pDbr->cshrtval);
         ASSIGN_NUMERIC (pDbr->cshrtval, 0);
         pClient->data.kind = vkInteger;
         pClient->data.value.ival = (long) (&pDbr->cshrtval.value)[e];
         break;

      case DBR_CTRL_FLOAT:
         ASSIGN_STATUS (pDbr->cfltval);
         ASSIGN_NUMERIC (pDbr->cfltval, pDbr->cfltval.precision);
         pClient->data.kind = vkFloating;
         pClient->data.value.dval = (double) (&pDbr->cfltval.value)[e];
         break;

      case DBR_CTRL_ENUM:
         ASSIGN_STATUS (pDbr->cenmval);
         CLEAR_NUMERIC;
         pClient->num_states = pDbr->cenmval.no_str;
         memcpy (pClient->enum_strings, pDbr->cenmval.strs,
                 sizeof (pClient->enum_strings));

         enum_value = (dbr_short_t) (&pDbr->cenmval.value)[e];
         if (enums_as_string) {
            pClient->data.kind = vkString;
            if (enum_value < pClient->num_states) {
               strncpy (pClient->data.value.sval,
                        pClient->enum_strings[enum_value],
                        MAX_ENUM_STRING_SIZE);
               pClient->data.value.sval[MAX_ENUM_STRING_SIZE] = '\0';
            } else {
               pClient->data.value.sval[0] = '\0';
            }
         } else {
            pClient->data.kind = vkInteger;
            pClient->data.value.ival = (long) enum_value;
         }
         break;

      case DBR_CTRL_CHAR:
         ASSIGN_STATUS (pDbr->cchrval);
         ASSIGN_NUMERIC (pDbr->cchrval, 0);
         pClient->data.kind = vkInteger;
         pClient->data.value.ival = (long) (&pDbr->cchrval.value)[e];
         break;

      case DBR_CTRL_LONG:
         ASSIGN_STATUS (pDbr->clngval);
         ASSIGN_NUMERIC (pDbr->clngval, 0);
         pClient->data.kind = vkInteger;
         pClient->data.value.ival = (long) (&pDbr->clngval.value)[e];
         break;

      case DBR_CTRL_DOUBLE:
         ASSIGN_STATUS (pDbr->cdblval);
         ASSIGN_NUMERIC (pDbr->cdblval, pDbr->cdblval.precision);
         pClient->data.kind = vkFloating;
         pClient->data.value.dval = (double) (&pDbr->cdblval.value)[e];
         break;

   /** Time updates values [count], time, severity and status **/

      case DBR_TIME_STRING:
         ASSIGN_STATUS_AND_TIME (pDbr->tstrval);
         pClient->data.kind = vkString;
         strncpy (pClient->data.value.sval,
                  (&pDbr->tstrval.value)[e], MAX_STRING_SIZE);
         pClient->data.value.sval[MAX_STRING_SIZE] = '\0';
         break;

      case DBR_TIME_SHORT:
         ASSIGN_STATUS_AND_TIME (pDbr->tshrtval);
         pClient->data.kind = vkInteger;
         pClient->data.value.ival = (long) (&pDbr->tshrtval.value)[e];
         break;

      case DBR_TIME_FLOAT:
         ASSIGN_STATUS_AND_TIME (pDbr->tfltval);
         pClient->data.kind = vkFloating;
         pClient->data.value.dval = (double) (&pDbr->tfltval.value)[e];
         break;

      case DBR_TIME_ENUM:
         ASSIGN_STATUS_AND_TIME (pDbr->tenmval);
         enum_value = (dbr_short_t) (&pDbr->tenmval.value)[e];
         if (enums_as_string) {
            pClient->data.kind = vkString;
            if (enum_value < pClient->num_states) {
               strncpy (pClient->data.value.sval,
                        pClient->enum_strings[enum_value],
                        MAX_ENUM_STRING_SIZE);
               pClient->data.value.sval[MAX_ENUM_STRING_SIZE] = '\0';
            } else {
               pClient->data.value.sval[0] = '\0';
            }
         } else {
            pClient->data.kind = vkInteger;
            pClient->data.value.ival = (long) enum_value;
         }
         break;

      case DBR_TIME_CHAR:
         ASSIGN_STATUS_AND_TIME (pDbr->tchrval);
         pClient->data.kind = vkInteger;
         pClient->data.value.ival = (long) (&pDbr->tchrval.value)[e];
         break;

      case DBR_TIME_LONG:
         ASSIGN_STATUS_AND_TIME (pDbr->tlngval);
         pClient->data.kind = vkInteger;
         pClient->data.value.ival = (long) (&pDbr->tlngval.value)[e];
         break;

      case DBR_TIME_DOUBLE:
         ASSIGN_STATUS_AND_TIME (pDbr->tdblval);
         pClient->data.kind = vkFloating;
         pClient->data.value.dval = (double) (&pDbr->tdblval.value)[e];
         break;

      default:
         printf ("%s (%s): unexpected buffer type %ld\n",
                 function, pClient->pv_name, args->type);
         pClient->data.kind = vkVoid;
         return;
   }

   Process_PV_Update (pClient);
   pClient->is_first_update = false;

#undef ASSIGN_STATUS
#undef ASSIGN_STATUS_AND_TIME
#undef ASSIGN_NUMERIC
#undef CLEAR_NUMERIC
}                               /* Get_Event_Handler */


/*------------------------------------------------------------------------------
 * Unsubscribes channel
 */
static void Unsubscribe_Channel (CA_Client * pClient)
{
   int status;

   /* Unsubscribe iff needs be
    */
   if (pClient->event_id) {
      status = ca_clear_subscription (pClient->event_id);
      if (status != ECA_NORMAL) {
         printf ("ca_clear_subscription (%s) failed (%s)\n",
                 pClient->pv_name, ca_message (status));
      }
      pClient->event_id = NULL;

      /* Set connection closed in database
       */
      (void) time (&pClient->disconnect_time);
   }
}


/*------------------------------------------------------------------------------
 * closes channel
 */
static void Clear_Channel (CA_Client * pClient)
{
   int status;

   /* This function checks if we are subscribed.
    */
   Unsubscribe_Channel (pClient);

   /* Close channel iff needs be.
    */
   if (pClient->channel_id) {
      status = ca_clear_channel (pClient->channel_id);
      if (status != ECA_NORMAL) {
         printf ("ca_clear_channel (%s) failed (%s)\n",
                 pClient->pv_name, ca_message (status));
      }

      pClient->channel_id = NULL;
      pClient->is_connected = false;
   }
}                               /* Clear_Channel */


/* -----------------------------------------------------------------------------
 */
static CA_Client *Validate_Channel_Id (const chid channel_id)
{
   void *user_data;
   CA_Client *result = NULL;

   /* Hypothosize something wrong unless we pass all checks.
    */
   if (channel_id == NULL) {
      Report ("Unassigned channel id");
      return NULL;
   }

   user_data = ca_puser (channel_id);
   if (user_data == NULL) {
      Report ("Unassigned user data");
      return NULL;
   }

   result = (CA_Client *) user_data;
   if ((result->magic1 != CA_CLIENT_MAGIC)
       || (result->magic2 != CA_CLIENT_MAGIC)) {
      Report ("User Data not a CA_Client");
      return NULL;
   }

   if (result->channel_id == NULL) {
      Report ("CA Client has unassigned channel id");
      return NULL;
   }

   if (result->channel_id != channel_id) {
      Report ("Channel id mis-match");
      return NULL;
   }

   /* We passed all the checks.
    */
   return result;
}                               /* Validate_Channel_Id */

/*------------------------------------------------------------------------------
 * CALLBACK FUNCTIONS expected by the buffered_callbacks module
 *------------------------------------------------------------------------------
 *
 * Connection handler
 */
void application_connection_handler (struct connection_handler_args *args)
{
   CA_Client *pClient;

   pClient = Validate_Channel_Id (args->chid);

   if (pClient) {
      switch (args->op) {

         case CA_OP_CONN_UP:
            if (debug >= 4) {
               printf ("PV connected %s\n", pClient->pv_name);
            }
            pClient->is_connected = true;
            pClient->field_type = ca_field_type (pClient->channel_id);
            pClient->element_count =
                ca_element_count (pClient->channel_id);
            strncpy (pClient->host_name,
                     ca_host_name (pClient->channel_id),
                     sizeof (pClient->host_name));
            pClient->data_element_count = 0;    /* no data yet */
            Subscribe_Channel (pClient);
            break;

         case CA_OP_CONN_DOWN:
            if (debug >= 4) {
               printf ("PV disconnected %s\n", pClient->pv_name);
            }

            /* We unsubscribe here to avoid a duplicate subscriptions
             * if/when we reconnect. In principle we could keep the same
             * subscription active, but doing a new Subscribe on connect
             * will do a new Array_Get and Subscribe which is good in case
             * any PV meta data parameters (units, precision) have changed.
             */
            Unsubscribe_Channel (pClient);
            Process_PV_Disconnect (pClient);
            break;

         default:
            Report ("connection_handler: Unexpected args op");
      }
   }
}                               /* application_connection_handler */

/*------------------------------------------------------------------------------
 * Event handler
 */
void application_event_handler (struct event_handler_args *args)
{
   CA_Client *pClient;

   pClient = Validate_Channel_Id (args->chid);
   if (pClient) {

      /* Valid channel id - need some more event specific checks.
       */
      if (args->status == ECA_NORMAL) {

         if (debug >= 4) {
            printf ("PV event (%s) first %s\n", pClient->pv_name,
                    BOOL_IMAGE (pClient->is_first_update));
         }

         if ((args->usr == &Get) || (args->usr == &Event)) {

            if (args->dbr) {
               Get_Event_Handler (pClient, args);
            } else {
               printf ("event_handler (%s) args->dbr is null\n",
                       pClient->pv_name);
            }

         } else if (args->usr == &Put) {

            /* place holder */
            printf ("event_handler (%s) unexpected args->usr = Put\n",
                    pClient->pv_name);

         } else {
            printf ("event_handler (%s) unknown args->usr\n",
                    pClient->pv_name);
         }

      } else {
         printf ("event_handler (%s) error (%s)\n",
                 pClient->pv_name, ca_message (args->status));
      }
   }
}                               /* application_event_handler */


/*------------------------------------------------------------------------------
 * Replacement printf handler
 */
void application_printf_handler (char *formated_text)
{
   printf ("%s", formated_text);
}                               /* application_printf_handler */


/*------------------------------------------------------------------------------
 */
void Print_Match_Information (CA_Client * pClient)
{
   unsigned int j;
   Variant_Kind kind;
   char *request;
   Variant_Range *pVR;
   char lower[45];
   char upper[45];
   char *lq, *uq;

   kind = pClient->match_set_collection.item[0].lower.kind;
   switch (kind) {

      case vkString:
         request = "DBF_STRING";
         break;

      case vkInteger:
         request = "DBF_LONG";
         break;

      case vkFloating:
         request = "DBF_DOUBLE";
         break;

      default:
         request = "NONE";
         break;
   }

   printf ("PV Name: %s [%d]\n", pClient->pv_name, pClient->element_index);

   printf ("Request: %s\n", request);

   printf ("Command: %s\n", pClient->match_command);

   for (j = 0; j < pClient->match_set_collection.count; j++) {
      if (j == 0) {
         printf ("Matches: ");
      } else {
         printf ("     or: ");
      }

      pVR = &pClient->match_set_collection.item[j];

      Variant_Image (lower, sizeof (lower), &pVR->lower);
      lq = (pVR->lower.kind == vkString) ? "\"" : "";

      Variant_Image (upper, sizeof (upper), &pVR->upper);
      uq = (pVR->upper.kind == vkString) ? "\"" : "";

      printf ("%d  %s%s%s", pVR->comp, lq, lower, lq);

      if (pVR->comp == ckRange) {
         printf (" to %s%s%s", uq, upper, uq);
      }
      printf ("\n");

   }
   printf ("\n");
}                               /* Print_Match_Information */


/*------------------------------------------------------------------------------
 */
void Print_Connection_Timeout (CA_Client * pClient)
{
   if (pClient->is_connected != true) {
      printf ("Channel connect timed out: '%s' not found.\n",
              pClient->pv_name);
   }
}                               /* Print_Connection_Timeout */


/*------------------------------------------------------------------------------
 * CLIENT LIST functions
 *------------------------------------------------------------------------------
 */
static void Create_All_Channels (ELLLIST * CA_Client_List)
{
   CA_Client *pClient;

   pClient = (CA_Client *) ellFirst (CA_Client_List);
   while (pClient) {
      /* Open this Channel Access channel
       */
      Create_Channel (pClient);
      pClient = (CA_Client *) ellNext ((ELLNODE *) pClient);
   }
}                               /* Create_All_Channels */


/*------------------------------------------------------------------------------
 */
static void Clear_All_Channels (ELLLIST * CA_Client_List)
{
   CA_Client *pClient;

   pClient = (CA_Client *) ellFirst (CA_Client_List);
   while (pClient) {
      /* Close this Channel Access channel
       */
      Clear_Channel (pClient);
      pClient = (CA_Client *) ellNext ((ELLNODE *) pClient);
   }
}                               /* Clear_All_Channels */


/*------------------------------------------------------------------------------
 */
static void Print_All_Match_Information (ELLLIST * CA_Client_List)
{
   CA_Client *pClient;

   pClient = (CA_Client *) ellFirst (CA_Client_List);
   while (pClient) {
      Print_Match_Information (pClient);
      pClient = (CA_Client *) ellNext ((ELLNODE *) pClient);
   }
}                               /* Print_All_Client_Information */


/*------------------------------------------------------------------------------
 */
static void Print_All_Connection_Timeouts (ELLLIST * CA_Client_List)
{
   CA_Client *pClient;

   pClient = (CA_Client *) ellFirst (CA_Client_List);
   while (pClient) {
      Print_Connection_Timeout (pClient);
      pClient = (CA_Client *) ellNext ((ELLNODE *) pClient);
   }
}                               /* Verify_All_Clients_Are_Connected */


/*------------------------------------------------------------------------------
 * LOCAL DATA
 *------------------------------------------------------------------------------
 */
static ELLLIST CA_Client_List = ELLLIST_INIT;


/*------------------------------------------------------------------------------
 */
CA_Client *Allocate_Client ()
{
   CA_Client *result;

   result = (CA_Client *) callocMustSucceed
       (1, sizeof (CA_Client), "Allocate_Client");

   result->magic1 = CA_CLIENT_MAGIC;
   result->magic2 = CA_CLIENT_MAGIC;
   result->is_connected = false;
   result->channel_id = NULL;
   result->event_id = NULL;
   result->pv_name[0] = '\0';
   result->match_set_collection.count = 0;
   result->match_command[0] = '\0';

   /* Lastly add to client list.
    */
   ellAdd (&CA_Client_List, (ELLNODE *) result);

   return result;
}                               /* Allocate_Client */


/*------------------------------------------------------------------------------
 * PUBLIC FUNCTIONS
 *------------------------------------------------------------------------------
 */
bool Create_PV_Client_List_From_File (const char *pv_list_filename, int *number)
{
   bool result;
   int n;

   /* Initialialise the list of clients.
    */
   ellInit (&CA_Client_List);

   result = Scan_Configuration_File (pv_list_filename, &Allocate_Client);

   n = ellCount (&CA_Client_List);
   printf ("PV client list created - %d %s.\n", n,
           (n == 1 ? "entry" : " entries"));

   *number = n;
   return result;
}                               /* Create_PV_Client_List */

/*------------------------------------------------------------------------------
 */
bool Create_PV_Client_List_From_String (const char *buffer, const size_t size, int *number)
{
   bool result;
   int n;

   /* Initialialise the list of clients.
    */
   ellInit (&CA_Client_List);

   result = Scan_Configuration_String (buffer, size, &Allocate_Client);

   n = ellCount (&CA_Client_List);
   printf ("PV client list created - %d %s.\n", n,
           (n == 1 ? "entry" : " entries"));

   *number = n;
   return result;
}

/*------------------------------------------------------------------------------
 */
void Print_Clients_Info ()
{
   printf ("\n");
   Print_All_Match_Information (&CA_Client_List);
}                               /* Print_Clients_Info */


/*------------------------------------------------------------------------------
 */
bool Process_Clients (Bool_Function_Handle shut_down)
{
   const int maximum = 400;     /* maximum items processed at one time */
   const double delay = 0.05;   /* delay between processing burst */

   bool connection_timouts_are_done;
   int status;
/*
   static long last_time;
   static long this_time;
*/

   initialise_buffered_callbacks ();

   /* Create Channel Access context.
    */
   status = ca_context_create (ca_enable_preemptive_callback);
   if (status != ECA_NORMAL) {
      printf ("ca_context_create failed (%s)\n", ca_message (status));
      return false;
   }

   /* Replace the CA Client Library report handler.
    */
   status = ca_replace_printf_handler (buffered_printf_handler);
   if (status != ECA_NORMAL) {
      printf ("ca_replace_printf_handler failed (%s)\n",
              ca_message (status));
      /* This is not return-worthy. Carry on 
       */
   }

   if (is_verbose) {
      printf ("Creating all PV channels\n");
   }
   Create_All_Channels (&CA_Client_List);

   start_time = ((long) time (NULL));

   connection_timouts_are_done = false;
   cycle = 0;
   while ((*shut_down) () == false) {
      cycle++;

      status = ca_flush_io ();
      if (status != ECA_NORMAL) {
         printf ("ca_flush_io failed (%s)\n", ca_message (status));
      }

      process_buffered_callbacks (maximum);

      /* Allow channels 2 seconds to connect before we test for
       * connection timeouts.
       */
      if ((connection_timouts_are_done == false) &&
          ((cycle * delay) >= 2.0)) {
         Print_All_Connection_Timeouts (&CA_Client_List);
         connection_timouts_are_done = true;
      }

      /** TODO Maybe ??
      this_time = ((long) time (NULL));
      if (this_time >= last_time + 60) {
         printf ("re-reading /etc/kryton.conf\n");
         last_time += 60;
      }
      **/

      epicsThreadSleep (delay);
   }

   if (is_verbose) {
      printf ("Clearing all PV channels\n");
   }
   Clear_All_Channels (&CA_Client_List);

   /* Reset the CA Client Library report handler.
    */
   status = ca_replace_printf_handler (NULL);
   ca_context_destroy ();

   return true;
}                               /* Process_Clients */

/* end */
