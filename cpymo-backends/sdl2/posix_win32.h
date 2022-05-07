#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#include <direct.h>

#ifndef alloca
#define alloca _alloca
#endif

#ifndef mkdir
#define mkdir(A, B) _mkdir(A);
#endif

#endif
