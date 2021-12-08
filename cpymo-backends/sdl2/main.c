#include <stdio.h>
#include <cpymo_error.h>

int main(void) {
	printf("Hello, pymo!");
	const char *msg = cpymo_error_message(CPYMO_ERR_SUCC);
	printf("%s", msg);
	return 0;
}
