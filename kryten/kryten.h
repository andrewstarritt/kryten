/* $File: //depot/sw/epics/kryten/kryten.h $
 * $Revision: #4 $
 * $DateTime: 2011/05/22 15:50:28 $
 * $Author: andrew $
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
