/* $File: //depot/sw/epics/kryten/information.h $
 * $Revision: #11 $
 * $DateTime: 2013/02/03 17:35:29 $
 * Last checked in by: $Author: andrew $
 *
 * Copyright (C) 2011-2013  Andrew C. Starritt
 * See kryten.c for details.
 *
 */
#ifndef INFORMATION_H_
#define INFORMATION_H_

#define KRYTEN_VERSION    "2.1.5"
#define BUILD_DATETIME    __DATE__ " " __TIME__

/* All these functions print information only.
 */
void Version ();

void Usage ();

void Help ();

void Preamble ();

#endif                          /* INFORMATION_H_ */
