/* kryten.c
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

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "kryten.h"
#include "information.h"
#include "gnu_public_licence.h"
#include "pv_client.h"
#include "utilities.h"


/* This does not seem to be defined in signal.h.
 */
typedef void (*sighandler_t) (int);


extern int daemon (int nochdir, int noclose);

/*------------------------------------------------------------------------------
 * Global Data
 * Visible to all units
 */
bool is_verbose = false;
bool quit_invoked = false;
int exit_code = 0;

/*------------------------------------------------------------------------------
 */
static volatile bool sig_int_received = false;
static volatile bool sig_term_received = false;


/*------------------------------------------------------------------------------
 * Signal catcher function. Only handles interrupt and terminate signals.
 */
static void Signal_Catcher (int sig)
{
   switch (sig) {

      case SIGINT:
         sig_int_received = true;
         exit_code = 128 + sig;
         printf ("\nSIGINT received - initiating orderly shutdown.\n");
         break;

      case SIGTERM:
         sig_term_received = true;
         exit_code = 128 + sig;
         printf ("\nSIGTERM received - initiating orderly shutdown.\n");
         break;
   }
}                               /* Signal_Catcher */


/*------------------------------------------------------------------------------
 * Checks if time to shut sown the kryten program.
 * This a test if SIGINT/SIGTERM have been received.
 */
static bool Shut_Down_Is_Required ()
{
   return (sig_int_received || sig_term_received || quit_invoked);
}                               /* Shut_Down_Is_Required */


/*------------------------------------------------------------------------------
 * Main functionality
 */
static bool Run (const bool just_check_config_file, const bool is_daemon)
{
   bool status;

   if (is_verbose) {
      printf ("Channels/match criteria...\n");
      Print_Clients_Info ();
   }

   if (just_check_config_file) {
      /* Nothing more to do - just return.
       */
      return true;
   }

   /* Completed a lot of the preliminary checks and about to start.
    * Run as daemon now if user requested it.
    */
   if (is_daemon) {
      printf ("Running kryten as system daemon ...\n");
      /* Don't change directory but do re-direct all output.
       */
      (void) daemon (1, 0);
   }

   /* Opens all channnels, process all data and
    * regularly calls Shut_Down to see if time to
    * shut down, and then closes all channels.
    */
   if (is_verbose) {
      printf ("Processing starting...\n");
   }
   status = Process_Clients (Shut_Down_Is_Required);

   return status;
}                               /* Run */

/*------------------------------------------------------------------------------
 * main program
 */
int main (int argc, char *argv[])
{
   sighandler_t old_handler;
   bool status;
   const char *config_filename = "";
   const char* string_config = NULL;
   bool is_daemon;
   bool is_suppress;
   bool is_just_check;
   bool is_command_line_config;

   /* Check for special options prior to main processing.
    */
   if (argc >= 2) {

      if (is_either (argv[1], "--help", "-h")) {
         Help ();
         return 0;
      }

      if (is_either (argv[1], "--licence", "-l")) {
         Licence ();
         return 0;
      }

      if (is_either (argv[1], "--warranty", "-w")) {
         No_Warranty ();
         return 0;
      }

      if (is_either (argv[1], "--redistribute", "-r")) {
         Redistribute ();
         return 0;
      }

      if (is_either (argv[1], "--version", "-V")) {
         Version ();
         return 0;
      }
   }

   /* Main parameter processing.
    */
   is_suppress = false;
   is_verbose = false;
   is_daemon = false;
   is_just_check = false;
   is_command_line_config = false;

   while ((argc >= 2) && (argv[1][0] == '-')) {
      if      (check_flag (argv[1], "--suppress", "-s", &is_suppress)) { }
      else if (check_flag (argv[1], "--verbose", "-v", &is_verbose)) { }
      else if (check_flag (argv[1], "--daemon", "-d", &is_daemon)) { }
      else if (check_flag (argv[1], "--check", "-c", &is_just_check)) { }
      else if (check_argument (argv[1], argv[2], "--monitor", "-m",
                               &is_command_line_config, &string_config))
      {
         /* skip option parameter */
         argc--;
         argv++;
      } else {
         printf ("%swarning%s unknown option '%s'  ignored.\n",
                 yellow, reset, argv[1]);
      }

      /* shift 1 */
      argc--;
      argv++;
   }

   /* If not inline, check for one and only parameter.
    */
   if (!is_command_line_config) {
      if ((argc < 2) || (strlen (argv[1]) == 0)) {
         printf ("missing/null configuration file parameter\n");
         usage ();
         return 1;
      }
      config_filename = argv[1];
   } else {
      config_filename = "";
   }


   /* Ready to go
    */
   if (!is_suppress) {
      Preamble ();
   }

   if (is_verbose) {
      Version ();
      printf ("configuration file: %s\n", config_filename);
   }

   if (argc > 2) {
      printf ("%swarning%s extra %d parameter(s) ignored.\n",
              yellow, reset, argc - 2);
   }

   /* Read configuration file / string to get list of required PVs
    * and create a list of PV clients.
    */
   int number = 0;
   if (is_command_line_config) {
      status = Create_PV_Client_List_From_String (string_config, strlen (string_config), &number);
    } else {
      status = Create_PV_Client_List_From_File (config_filename, &number);
   }

   if (!status) {
      printf ("%sError%s : PV client list creation failed\n", red, reset);
      return 1;
   }

   if (number == 0) {
      printf ("PV client list is %sempty%s - initialing an early shutdown.\n",
              yellow, reset);
      return 0;
   }


   /* Just about to start for real - set up sig term handler.
    */
   old_handler = signal (SIGTERM, Signal_Catcher);
   old_handler = signal (SIGINT, Signal_Catcher);

   status = Run (is_just_check, is_daemon);
   if (status) {
      printf ("%skryten%s complete\n", green, reset);
   }

   return status ? exit_code : 1;
}                               /* main */

/* end */
