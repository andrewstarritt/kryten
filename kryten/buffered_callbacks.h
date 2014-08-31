/* $File: //depot/sw/c/buffered_callbacks.h $
 * $Revision: #1 $
 * $DateTime: 2012/02/28 04:53:21 $
 * $Author: andrew $
 *
 * EPICS buffered callback module for use with Ada, Lazarus and other
 * runtime environments which don't like alien threads.
 *
 * Copyright (C) 2010-2011  Andrew C. Starritt
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
 */

#ifndef BUFFERED_CALLBACKS_H_
#define BUFFERED_CALLBACKS_H_

#include <cadef.h>

/* These functions are exported by this unit.
 *
 * NOTE: We never call the handers directly, but do pass the address of these
 * functions as parameters to the relevent functions within the ca library.
 */
void buffered_connection_handler (struct connection_handler_args args);
void buffered_event_handler (struct event_handler_args args);
int buffered_printf_handler (const char *pformat, va_list args);

/* This function should be called once, prior to calling process_buffered_callbacks
 * or the possibility of any callbacks.
 */
void initialise_buffered_callbacks ();

/* Returns number of currently buffer callbacks
 */
int number_of_buffered_callbacks ();

/* This function should be called regularly - say every 50 mSeconds.
 * It process a maximum of max buffer items. It returns thr actual
 * number of callbacks processed (<= max).
 */
int process_buffered_callbacks (const int max);

#endif                          /* BUFFERED_CALLBACKS_H_ */
