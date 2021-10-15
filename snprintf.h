#include <stdio.h>

#if defined(__MINGW32__) || defined(__MINGW64__)
// MinGW Snprintf version is defined based on Windows _Snprintf
// which is not standard confirming. Use __mingw_Snprintf which
// is standard conforming implementation of Snprintf.
#define Snprintf __mingw_snprintf
#else
// Other non-mingw the default Snprintf is standard conforming.
#define Snprintf snprintf
#endif