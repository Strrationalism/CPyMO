#ifndef INCLUDE_CPYMO_ERROR
#define INCLUDE_CPYMO_ERROR

typedef int error_t;

#define CPYMO_ERR_SUCC 0
#define CPYMO_ERR_NOT_FOUND (-1)
#define CPYMO_ERR_CAN_NOT_OPEN_FILE (-2)
#define CPYMO_ERR_BAD_FILE_FORMAT (-3)
#define CPYMO_ERR_INVALID_ARG (-4)
#define CPYMO_ERR_OUT_OF_MEM (-5)
#define CPYMO_ERR_NO_MORE_CONTENT (-6)
#define CPYMO_ERR_UNSUPPORTED (-7)
#define CPYMO_ERR_SCRIPT_LABEL_NOT_FOUND (-8)
#define CPYMO_ERR_UNKNOWN (-65536)

extern const char * cpymo_error_message(error_t err);

#define CPYMO_THROW(ERR) if (ERR != CPYMO_ERR_SUCC) return ERR;

#endif
