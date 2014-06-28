#include <stdio.h>
#include <stdlib.h>

#include "io.h"

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
