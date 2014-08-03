#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"

char* BACKLIGHT_PATH = 0;

char* strconcat(char const* str1, char const* str2) {
	char* res = calloc(strlen(str1) + strlen(str2) + 1, sizeof(char));
	strcpy(res, str1);
	strcat(res, str2);
	return res;
}

void backlight_read(int* current_val, int* max_val) {
	char* cur_filename = strconcat(BACKLIGHT_PATH, "/brightness");
	FILE* cur_file = fopen(cur_filename, "r");
	if(!cur_file) {
		perror("Can't open brightness for reading");
		exit(EXIT_FAILURE);
	}
	if(fscanf(cur_file, "%d", current_val) != 1) {
		perror("Can't read an int from brightness");
		exit(EXIT_FAILURE);
	}
	fclose(cur_file);
	free(cur_filename);
	char* max_filename = strconcat(BACKLIGHT_PATH, "/max_brightness");
	FILE* max_file = fopen(max_filename, "r");
	if(!max_file) {
		perror("Can't open max_brightness for reading");
		exit(EXIT_FAILURE);
	}
	if(fscanf(max_file, "%d", max_val) != 1) {
		perror("Can't read an int from max_brightness");
		exit(EXIT_FAILURE);
	}
	fclose(max_file);
	free(max_filename);
}

void backlight_write(int new_val) {
	char* cur_filename = strconcat(BACKLIGHT_PATH, "/brightness");
	FILE* cur_file = fopen(cur_filename, "w");
	if(!cur_file) {
		perror("Can't open brightness for writing");
		exit(EXIT_FAILURE);
	}
	fprintf(cur_file, "%d", new_val);
	fclose(cur_file);
	free(cur_filename);
}
