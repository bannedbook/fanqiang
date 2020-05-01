#ifdef __MINGW32__

#include <time.h>

// From git project: compact/mingw.h

#ifndef S_IFLNK
#define S_IFLNK    0120000 /* Symbolic link */
#endif

#ifndef S_ISLNK
#define S_ISLNK(x) (((x) & S_IFMT) == S_IFLNK)
#endif

struct tm *gmtime_r(const time_t *timep, struct tm *result);
struct tm *localtime_r(const time_t *timep, struct tm *result);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
int clearenv(void);

#endif
