/* $File: //depot/sw/epics/kryten/kryten.h $
 * $Revision: #5 $
 * $DateTime: 2012/02/26 16:06:27 $
 * Last checked in by: $Author: andrew $
 */

#ifndef KRYTEN_H_
#define KRYTEN_H_

#ifndef __cplusplus
typedef enum ebool {
   false = 0,
   true = !false
} bool;
#endif

#define BOOL_IMAGE(zz)  ((zz) ?  "true " : "false")

extern bool is_verbose;

#endif                          /* KRYTEN_H_ */
