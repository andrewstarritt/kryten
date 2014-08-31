/* $File: //depot/sw/epics/kryten/filter.h $
 * $Revision: #3 $
 * $DateTime: 2012/02/04 09:44:25 $
 * $Author: andrew $
 */

#ifndef PV_FILTER_H_
#define PV_FILTER_H_

#include "kryten.h"
#include "pv_client.h"

void Process_PV_Update (CA_Client * pClient);
void Process_PV_Disconnect (CA_Client * pClient);

#endif                          /* PV_FILTER_H_ */