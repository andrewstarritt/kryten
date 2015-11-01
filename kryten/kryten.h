/* $File: //depot/sw/epics/kryten/kryten.h $
 * $Revision: #6 $
 * $DateTime: 2015/11/01 15:48:18 $
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
extern bool quit_invoked;
extern int exit_code;

#endif                          /* KRYTEN_H_ */
