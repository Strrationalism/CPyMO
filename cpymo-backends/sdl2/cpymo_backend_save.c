#include <cpymo_backend_save.h>
#include <string.h>
#include "posix_win32.h"

FILE *cpymo_backend_read_save(const char * gamedir, const char * name)
{
	char *path = (char *)alloca(strlen(gamedir) + strlen(name) + 8);
	sprintf(path, "%s/save/%s", gamedir, name);
	return fopen(path, "rb");
}

FILE *cpymo_backend_write_save(const char * gamedir, const char * name)
{
	char *path = (char *)alloca(strlen(gamedir) + strlen(name) + 8);
	sprintf(path, "%s/save/%s", gamedir, name);
	return fopen(path, "wb");
}
