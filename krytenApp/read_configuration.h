/* $File: //depot/sw/epics/kryten/read_configuration.h $
 * $Revision: #4 $
 * $DateTime: 2015/11/01 15:48:18 $
 * Last checked in by: $Author: andrew $
 */

#ifndef READ_PV_LIST_H_
#define READ_PV_LIST_H_

#include <ellLib.h>

#include "kryten.h"
#include "pv_client.h"

bool Scan_Configuration_File (const char *filename,
                              const Allocate_Client_Handle allocate);

bool Scan_Configuration_String (const char *buffer,
                                const size_t size,
                                const Allocate_Client_Handle allocate);

#endif                          /* READ_PV_LIST_H_ */
