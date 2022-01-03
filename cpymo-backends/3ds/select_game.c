#include "select_game.h"
#include <3ds.h>
#include <stdio.h>

const char * select_game()
{
	if (R_FAILED(fsInit())) return NULL;

	FS_Archive archive;
	Result error = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (R_FAILED(error)) {
		fsExit();  
		return NULL;
	}

	Handle handle;
	error = FSUSER_OpenDirectory(&handle, archive, fsMakePath(PATH_ASCII, "/pymogames/"));
	if (R_FAILED(error)) {
		FSUSER_CloseArchive(archive);
		fsExit();
		return NULL;
	}

	u32 result = 0;
	do {
		FS_DirectoryEntry item;
		error = FSDIR_Read(handle, &result, 1, &item);
		if (R_FAILED(error)) continue;

		if (result == 1) {
			for (size_t i = 0; i < sizeof(item.name) / sizeof(u16); ++i) {
				if (item.name[i] == '\0') break;
				else putchar(item.name[i]);
			}

			putchar('\n');
		}
			
	} while (result);

	FSDIR_Close(handle);
	FSUSER_CloseArchive(archive);
	fsExit();

	return "pymogames/DAICHYAN";
}
