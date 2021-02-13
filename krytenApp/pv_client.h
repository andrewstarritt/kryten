/* pv_client.h 
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

#ifndef PV_CLIENT_H_
#define PV_CLIENT_H_

#include <time.h>

#include <alarm.h>
#include <cadef.h>
#include <dbDefs.h>
#include <ellLib.h>

#include "kryten.h"
#include "utilities.h"

/* For use with an ELLLIST objects
 */
#define CA_CLIENT_MAGIC   0xEB1C5314

#define MAXIMUM_PVNAME_SIZE        80
#define NUMBER_OF_VARIENT_RANGES   20
#define MATCH_COMMAND_LENGTH      120

/* Defines the types of value copmparisons that may be performed.
 * Order is significant, e.g. <= comes before <.
 */
typedef enum eComparision_Kind {
   ckVoid = 0,
   ckNotEqual,            /* /= */
   ckLessThanEqual,       /* <= */
   ckGreaterThanEqual,    /* >= */
   ckEqual,               /* =  */
   ckLessThan,            /* <  */
   ckGreaterThan,         /* >  */
   ckRange                /* ~  */
} Comparision_Kind;

typedef struct sVariant_Range {
   Comparision_Kind comp;
   Variant_Value lower;
   Variant_Value upper;
} Variant_Range;


typedef struct sVariant_Range_Collection {
   unsigned int count;
   Variant_Range item[NUMBER_OF_VARIENT_RANGES];
} Variant_Range_Collection;


struct sCA_Client {
   ELLNODE node;
   int magic1;                  /* used when void pointer cast to a sCA_Client */

   /* Channel Access connection info
    */
   char pv_name[MAXIMUM_PVNAME_SIZE];
   int element_index;
   chid channel_id;
   evid event_id;
   char host_name[80];
   short int field_type;
   unsigned long int element_count;

   /* Meta data (apart from time stamp, returned first update)
    * Essentially as out of dbr_ctrl_double and/or dbr_ctrl_enum.
    * Use double as this caters for all types (float, long, short etc.)
    */
   dbr_short_t precision;       /* number of decimal places */
   char units[MAX_UNITS_SIZE];  /* units of value */
   dbr_short_t num_states;      /* number of strings (was no_str) */
   char enum_strings[MAX_ENUM_STATES][MAX_ENUM_STRING_SIZE];    /* was strs */
   double upper_disp_limit;     /* upper limit of graph */
   double lower_disp_limit;     /* lower limit of graph */
   double upper_alarm_limit;
   double upper_warning_limit;
   double lower_warning_limit;
   double lower_alarm_limit;
   double upper_ctrl_limit;     /* upper control limit */
   double lower_ctrl_limit;     /* lower control limit */

   /* Per update channel information.
    */
   bool is_connected;
   bool is_first_update;
   long int data_element_count; /* number of elements received */

   epicsAlarmCondition status;  /* status of value */
   epicsAlarmSeverity severity; /* severity of alarm */
   time_t update_time;          /* secPastEpoch converted to system time */
   epicsUInt32 nano_sec;        /* nsec - direct copy */

   time_t disconnect_time;      /* system time */

   Variant_Value data;          /* current data value */

   char match_command[MATCH_COMMAND_LENGTH + 1];        /* system command to be called */
   Variant_Range_Collection match_set_collection;
   bool last_update_matched;

   int magic2;
};

typedef struct sCA_Client CA_Client;


/* pointer to a function that returns a boolean
 */
typedef bool (*Bool_Function_Handle) ();

typedef CA_Client *(*Allocate_Client_Handle) ();

bool Create_PV_Client_List_From_File (const char *pv_list_filename, int *number);
bool Create_PV_Client_List_From_String (const char *buffer, const size_t size, int *number);

void Print_Clients_Info ();

bool Process_Clients (Bool_Function_Handle shut_down);

#endif                          /* PV_CLIENT_H_ */
