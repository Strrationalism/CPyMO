cmake_minimum_required (VERSION 3.8)

include_directories (${FFMPEG_INCLUDE_DIRS})

add_library (cpymolib STATIC
	"cpymo_error.h" "cpymo_error.c"
	"cpymo_package.h" "cpymo_package.c"
	"cpymo_parser.h" "cpymo_parser.c"
	"cpymo_gameconfig.h" "cpymo_gameconfig.c" 
	"cpymo_utils.h" "cpymo_utils.c" 
	"cpymo_color.h" "cpymo_color.c" 
	"cpymo_engine.h" "cpymo_engine.c" 
	"cpymo_assetloader.h" "cpymo_assetloader.c" 
	"cpymo_interpreter.h" "cpymo_interpreter.c" 
	"cpymo_vars.h" "cpymo_vars.c" 
	"cpymo_wait.h" "cpymo_wait.c" 
	"cpymo_flash.h" "cpymo_flash.c" 
	"cpymo_fade.h" "cpymo_fade.c"
	"cpymo_tween.h" 
	"cpymo_bg.h" "cpymo_bg.c" 
	"cpymo_anime.h" "cpymo_anime.c" 
	"cpymo_select_img.h" "cpymo_select_img.c" 
	"cpymo_charas.h" "cpymo_charas.c" 
	"cpymo_scroll.h" "cpymo_scroll.c" 
	"cpymo_floating_hint.h" "cpymo_floating_hint.c" 
	"cpymo_textbox.h" "cpymo_textbox.c"
	"cpymo_say.h" "cpymo_say.c" 
	"cpymo_text.h" "cpymo_text.c" 
	"cpymo_save_global.h" "cpymo_save_global.c" 
	"cpymo_hash_flags.h" "cpymo_hash_flags.c" 
	"cpymo_ui.h" "cpymo_ui.c"
	"cpymo_album.c" "cpymo_album.h" 
	"cpymo_audio.h" "cpymo_audio.c")

