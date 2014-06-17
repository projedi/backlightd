#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BACKLIGHT_PATH "/sys/class/backlight/intel_backlight"
#define BACKLIGHT_STEP 80

void usage(char const* program_name) {
	printf("USAGE: %s {up|down}\n", program_name);
}

void backlight_read(int* current_val, int* max_val) {
	FILE* cur_file = fopen(BACKLIGHT_PATH"/brightness", "r");
	if(!cur_file) {
		perror("Can't open brightness for reading");
		exit(EXIT_FAILURE);
	}
	if(fscanf(cur_file, "%d", current_val) != 1) {
		perror("Can't read an int from brightness");
		exit(EXIT_FAILURE);
	}
	fclose(cur_file);
	FILE* max_file = fopen(BACKLIGHT_PATH"/max_brightness", "r");
	if(!max_file) {
		perror("Can't open max_brightness for reading");
		exit(EXIT_FAILURE);
	}
	if(fscanf(max_file, "%d", max_val) != 1) {
		perror("Can't read an int from max_brightness");
		exit(EXIT_FAILURE);
	}
	fclose(max_file);
}

void backlight_write(int new_val) {
	FILE* cur_file = fopen(BACKLIGHT_PATH"/brightness", "w");
	if(!cur_file) {
		perror("Can't open brightness for writing");
		exit(EXIT_FAILURE);
	}
	fprintf(cur_file, "%d", new_val);
	fclose(cur_file);
}

int clamp(int v, int vmin, int vmax) {
	return v > vmax ? vmax : v < vmin ? vmin : v;
}

void backlight_up() {
	int current_val;
	int max_val;
	backlight_read(&current_val, &max_val);
	int new_val = clamp(current_val + BACKLIGHT_STEP, 0, max_val);
	backlight_write(new_val);
}

void backlight_down() {
	int current_val;
	int max_val;
	backlight_read(&current_val, &max_val);
	int new_val = clamp(current_val - BACKLIGHT_STEP, 0, max_val);
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
