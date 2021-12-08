#include <stdio.h>
#include <cpymo_error.h>
#include <SDL.h>

int main(int argc, char **argv) {
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window *win = SDL_CreateWindow("Cpymo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768, 0);
	printf("Hello, pymo!");
	const char *msg = cpymo_error_message(CPYMO_ERR_SUCC);
	printf("%s", msg);

	SDL_Delay(5000);

	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
