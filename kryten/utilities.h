/* $File: //depot/sw/epics/kryten/utilities.h $
 * $Revision: #11 $
 * $DateTime: 2012/02/26 16:10:02 $
 * Last checked in by: $Author: andrew $
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

#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) > (b) ? (a) : (b))


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
 * This function copies the src string upto and including the terminating '\0'
 * character or upto but excluding the character pointed to by upto or n - 1
 * bytes which is ever the shorter to the array pointed to by dest. The strings
 * may not overlap, and the destination string dest must be large enough to 
 * receive the copy.
 *
 * Not more than n bytes including an always present terminating '\0' character
 * are copyied to dest.
 *
 * The extract () function return a pointer to the destination string dest.
 *
 * extract () is essentially a wrapper around strncpy
 */
char * extract (char *dest, const size_t n, const char *src, const char *upto);


/*------------------------------------------------------------------------------
 * This function copies the src string (including the terminating'\0' character)
 * to the array pointed to by dest but also substitutes all occurences of the
 * find string with the replace string. The src and dest strings may not overlap.
 *
 * Not more than n bytes including an always present terminating '\0' character
 * are copyied to dest.
 *
 * The substitute () function return a pointer to the destination string dest.
 *
 * If no occurence of find are present in src or if find is an empty string
 * then substitute () behaves similar to the strncpy (dest, src, n-1)
 *
 * Bugs
 * If the destination string of a substitute() is not large enough (that is,
 * if the programmer was stupid/lazy, and failed to check  the  size  before
 * copying) then anything might happen. Overflowing fixed length strings is
 * a favourite cracker technique.
 */
char *substitute (char *dest, const size_t n, const char *src,
                  const char *find, const char *replace);


/*------------------------------------------------------------------------------
 * This function returns a long value given an image of the value as a string,
 * ignoring any leading or trailing spaces (as defined by isspace ()).
 * This is similar to Ada's long'Value ("...") except that instead of an
 * exception, the success or otherwise is return in status.
 */
long long_value (const char *image, bool * status);

/* Ditto double. Double muts be true doubles, e.g. "5.0", just "5" won't do.
 */
double double_value (const char *image, bool * status);


/*------------------------------------------------------------------------------
 * Reads an environment variable as long integer.
 */
long get_long_env (const char *name, bool * status);


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
   char sval[MAX (MAX_STRING_SIZE, MAX_ENUM_STRING_SIZE) + 1];
   long ival;                   /* used for long, short and char and enum value */
   double dval;                 /* used for double and float */
};


typedef struct sVarient_Value {
   Varient_Kind kind;
   union Varient_Union value;
} Varient_Value;


const char *vkImage (const Varient_Kind kind);

/* Performs a Left == Right test on two varient values.
 * Unlike Lt which perforce type conversion in order to do the 
 * comparison, this is a strict type check, i.e 
 * Same (Float 5.0, Int 5) returns false.
 *
 * !(Lt (A,B)) && !(Lt (B,A)) does not always give same result as Same
 */
bool Varient_Same (const Varient_Value * left,
                   const Varient_Value * right);

/* Performs a Left <= Right test on two varient values.
 */
bool Varient_Le (const Varient_Value * left, const Varient_Value * right);

/* Upon successful return (value >= 0), this function returns the number of
 * characters written to character string str (not including the trailing '\0'
 * used to end output to strings). The function does not write  more than
 * size bytes (including the trailing '\0').
*/
int Varient_Image (char *str, size_t size, const Varient_Value * item);

#endif                          /* UTILITIES_H_ */
