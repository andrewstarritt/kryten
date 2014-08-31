/* $File: //depot/sw/epics/kryten/information.h $
 * $Revision: #7 $
 * $DateTime: 2012/02/19 11:24:28 $
 * Last checked in by: $Author: andrew $
 *
 * Copyright (C) 2011-2012  Andrew C. Starritt
 * See kryten.c for details.
 *
 */
#ifndef INFORMATION_H_
#define INFORMATION_H_

#define KRYTEN_VERSION    "2.1.1"
#define BUILD_DATETIME    __DATE__ " " __TIME__

/* All these functions print information only.
*/
void Version ();

void Usage ();

void Help ();

void Preamble ();

#endif                          /* INFORMATION_H_ */
