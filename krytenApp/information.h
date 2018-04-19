/* $File: //depot/sw/epics/kryten/information.h $
 * $Revision: #12 $
 * $DateTime: 2015/11/01 15:48:18 $
 * Last checked in by: $Author: andrew $
 *
 * Copyright (C) 2011-2015  Andrew C. Starritt
 * See kryten.c for details.
 *
 */

#ifndef INFORMATION_H_
#define INFORMATION_H_

#define KRYTEN_VERSION    "2.2.1"
#define BUILD_DATETIME    __DATE__ " " __TIME__

/* All these functions print information only.
 */
void Version ();

void usage ();

void Help ();

void Preamble ();

#endif                          /* INFORMATION_H_ */
