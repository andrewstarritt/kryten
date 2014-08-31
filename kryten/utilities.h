/* $File: //depot/sw/epics/kryten/utilities.h $
 * $Revision: #7 $
 * $DateTime: 2011/05/22 15:50:28 $
 * $Author: andrew $
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <epicsTypes.h>
#include <db_access.h>

#include "kryten.h"

/* colour escape sequences.
 */
extern const char *red;
extern const char *green;
extern const char *yellow;
extern const char *gray;
extern const char *reset;

#define min(a, b)   ((a) < (b) ? (a) : (b))
#define max(a, b)   ((a) > (b) ? (a) : (b))


/*------------------------------------------------------------------------------
 * Uses strcmp to test s against s1 and/or s2.
 */
bool is_either (const char *s, const char *s1, const char *s2);


/*------------------------------------------------------------------------------
 * prefix of form:  --xxx=value  where xxx is name
 */
void check_argument (const char *arg, const char *name,
                     bool * matches, bool * is_found, const char **value);


/*------------------------------------------------------------------------------
 */
void check_flag (const char *arg, const char *name1, const char *name2,
                 bool * matches, bool * is_found);


/*------------------------------------------------------------------------------
 * own varient type
 */
typedef enum eVarient_Kind {
   vkVoid,
   vkString,
   vkInteger,
   vkFloating
} Varient_Kind;

union Varient_Union {
   char sval[max (MAX_STRING_SIZE, MAX_ENUM_STRING_SIZE) + 1];
   long ival;                   /* used for long, short and char and enum value */
   double dval;                 /* used for double and float */
};


typedef struct sVarient_Value {
   Varient_Kind kind;
   union Varient_Union value;
} Varient_Value;


const char *vkImage (const Varient_Kind kind);

/* Performs a Left <= Right test on two varient values.
 */
bool Varient_Le (const Varient_Value * left, const Varient_Value * right);

/* Upon  successful return (value >= 0), this function returns the number of
 * characters written to character string str (not including the trailing '\0'
 * used to end output to strings). The function does not write  more than
 * size bytes (including the trailing '\0').
*/
int Varient_Image (char *str, size_t size, const Varient_Value * item);

#endif                          /* UTILITIES_H_ */
