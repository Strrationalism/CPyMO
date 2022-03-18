#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#include <direct.h>
#define alloca _alloca
#define mkdir(A, B) _mkdir(A);

#endif
