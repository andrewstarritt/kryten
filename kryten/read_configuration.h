/* $File: //depot/sw/epics/kryten/read_configuration.h $
 * $Revision: #3 $
 * $DateTime: 2012/03/03 23:48:38 $
 * Last checked in by: $Author: andrew $
 */

#ifndef READ_PV_LIST_H_
#define READ_PV_LIST_H_

#include <ellLib.h>

#include "kryten.h"
#include "pv_client.h"

bool Scan_Configuration_File (const char *filename,
                              const Allocate_Client_Handle allocate);

#endif                          /* READ_PV_LIST_H_ */
