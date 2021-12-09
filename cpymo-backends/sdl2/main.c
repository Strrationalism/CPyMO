#include <stdio.h>
#include <cpymo_error.h>
#include <SDL.h>
#include <cpymo_gameconfig.h>
#include <cpymo_parser.h>
#include <string.h>

int main(int argc, char **argv) {
	cpymo_gameconfig config;
	cpymo_gameconfig_parse_from_file(&config, "D:/Pymo/pymo_v1_2_0_for_PC/MO1_test/gameconfig.txt");

	return 0;
}
