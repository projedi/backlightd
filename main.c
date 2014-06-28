#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "backlight.h"
#include "io.h"

void usage(char const* program_name) {
	printf("USAGE: %s {up|down}\n", program_name);
}

void backlight_up() {
	int current_val;
	int max_val;
	backlight_read(&current_val, &max_val);
	int current_level = get_backlight_level(current_val, max_val);
	int new_val = get_backlight_value(current_level + 1, max_val);
	backlight_write(new_val);
}

void backlight_down() {
	int current_val;
	int max_val;
	backlight_read(&current_val, &max_val);
	int current_level = get_backlight_level(current_val, max_val);
	int new_val = get_backlight_value(current_level - 1, max_val);
	backlight_write(new_val);
}

int main(int argc, char** argv) {
	if(argc != 2) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if(!strcmp(argv[1], "up")) backlight_up();
	else if(!strcmp(argv[1], "down")) backlight_down();
	else {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
