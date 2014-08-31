/* $File: //depot/sw/epics/kryten/kryten.c $
 * $Revision: #17 $
 * $DateTime: 2013/02/03 17:35:29 $
 * Last checked in by: $Author: andrew $
 *
 * Description:
 * Kryten is a EPICS PV monitoring program that calls a system command
 * when the value of the PV matches/cease to match specified criteria.
 *
 * Copyright (C) 2011-2013  Andrew C. Starritt
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
 */
static bool shutdown_required = false;

/* Visible to all units
 */
bool is_verbose = false;

/*------------------------------------------------------------------------------
 * Signal catcher function. Only handles interrupt and terminate signals.
 */
static void Signal_Catcher (int sig)
{
   switch (sig) {
      case SIGINT:
         shutdown_required = true;
         printf ("\nSIGINT received - initiating orderly shutdown.\n");
         break;

      case SIGTERM:
         shutdown_required = true;
         printf ("\nSIGTERM received - initiating orderly shutdown.\n");
         break;
   }
}                               /* Signal_Catcher */


/*------------------------------------------------------------------------------
 * Checks if time to shut sown the kryten program.
 * This a test for SIGINT/SIGTERM.
 */
static bool Shut_Down_Is_Required ()
{
   return shutdown_required;
}                               /* Shut_Down */


/*------------------------------------------------------------------------------
 * Main functionality
 */
static bool Run (const char *config_filename,
                 const bool just_check_config_file, const bool is_daemon)
{
   bool status;
   int number;

   /* Read configuration file to get list of required PVs
    * and create a list of PV clients.
    */
   status = Create_PV_Client_List (config_filename, &number);
   if (!status) {
      printf ("%sError%s : PV client list creation failed\n", red, reset);
      return false;
   }

   if (number == 0) {
      printf
          ("PV client list is %sempty%s - initialing an early shutdown.\n",
           yellow, reset);
      return true;
   }

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
   const char *config_filename;
   bool found_config;
   bool is_daemon;
   bool is_suppress;
   bool is_just_check;
   bool is_okay;

   /* Check for special options proir to main processing.
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

      if (strcmp (argv[1], "--version") == 0) {
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
   found_config = false;

   while ((argc >= 2) && (argv[1][0] == '-')) {
      /* Hypothesize unknown option
       */
      is_okay = false;

      check_flag (argv[1], "--suppress", "-s", &is_okay, &is_suppress);
      check_flag (argv[1], "--verbose", "-v", &is_okay, &is_verbose);
      check_flag (argv[1], "--daemon", "-d", &is_okay, &is_daemon);
      check_flag (argv[1], "--check", "-c", &is_okay, &is_just_check);
      if (!is_okay) {
         printf ("%swarning%s unknown option '%s'  ignored.\n",
                 yellow, reset, argv[1]);
      }

      /* shift 1 */
      argc--;
      argv++;
   }

   /* Check for one and only parameter.
    */
   if ((argc < 2) || (strlen (argv[1]) == 0)) {
      printf ("missing/null configuration file parameter\n");
      Usage ();
      return 1;
   }
   config_filename = argv[1];


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

   /* Just about to start for real - set up sig term handler.
    */
   old_handler = signal (SIGTERM, Signal_Catcher);
   old_handler = signal (SIGINT, Signal_Catcher);

   status = Run (config_filename, is_just_check, is_daemon);
   if (status) {
      printf ("%skryten%s complete\n", green, reset);
   }

   return status ? 0 : 1;
}                               /* main */

/* end */
