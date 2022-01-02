#include "cpymo_error.h"

const char * cpymo_error_message(error_t err) {
	switch (err) {
	case CPYMO_ERR_SUCC: return "Success.";
	case CPYMO_ERR_NOT_FOUND: return "Not found.";
	case CPYMO_ERR_CAN_NOT_OPEN_FILE: return "Can not open file.";
	case CPYMO_ERR_BAD_FILE_FORMAT: return "Bad file format.";
	case CPYMO_ERR_INVALID_ARG: return "Invalid arguments.";
	case CPYMO_ERR_OUT_OF_MEM: return "Out of memory.";
	case CPYMO_ERR_NO_MORE_CONTENT: return "No more content.";
	default: return "Unknown error.";
	}
}
