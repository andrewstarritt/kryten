/* $File: //depot/sw/epics/kryten/read_configuration.h $
 * $Revision: #1 $
 * $DateTime: 2011/05/07 15:13:10 $
 * $Author: andrew $
 */

#ifndef READ_PV_LIST_H_
#define READ_PV_LIST_H_

#include <ellLib.h>
#include "kryten.h"

bool Scan_Configuration_File (const char *filename,
                              ELLLIST * ca_client_list);

#endif                          /* READ_PV_LIST_H_ */
