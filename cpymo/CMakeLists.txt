cmake_minimum_required (VERSION 3.8)

add_library (cpymolib STATIC
	"cpymo_error.h" "cpymo_error.c"
	"cpymo_package.h" "cpymo_package.c"
	"cpymo_parser.h" "cpymo_parser.c"
	"cpymo_gameconfig.h" "cpymo_gameconfig.c" 
	"cpymo_utils.h" "cpymo_utils.c" 
	"cpymo_color.h" "cpymo_color.c" 
	"cpymo_engine.h" "cpymo_engine.c" 
	 "cpymo_assetloader.h" "cpymo_assetloader.c")

