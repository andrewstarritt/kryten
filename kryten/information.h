/* $File: //depot/sw/epics/kryten/information.h $
 * $Revision: #10 $
 * $DateTime: 2012/05/16 21:51:29 $
 * Last checked in by: $Author: andrew $
 *
 * Copyright (C) 2011-2012  Andrew C. Starritt
 * See kryten.c for details.
 *
 */
#ifndef INFORMATION_H_
#define INFORMATION_H_

#define KRYTEN_VERSION    "2.1.4"
#define BUILD_DATETIME    __DATE__ " " __TIME__

/* All these functions print information only.
*/
void Version ();

void Usage ();

void Help ();

void Preamble ();

#endif                          /* INFORMATION_H_ */
