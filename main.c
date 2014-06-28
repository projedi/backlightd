#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define clamp(v, vmin, vmax) ((v) > (vmax) ? (vmax) : (v) < (vmin) ? (vmin) : (v))

#define BACKLIGHT_PATH "/sys/class/backlight/intel_backlight"
#define BACKLIGHT_LEVEL_COUNT 10

// level is clamped between 0 and 1.
// returns a value between 0 and 1.
double backlight_function(double level) {
	double x = clamp(level, 0, 1);
	double y = (x * x * x + x * x * x * x) / 2;
	return clamp(y, 0, 1);
}

int get_backlight_value(int current_level, int max_val) {
	int l = clamp(current_level, 0, BACKLIGHT_LEVEL_COUNT - 1);
	return (int)round(backlight_function((double)l / (BACKLIGHT_LEVEL_COUNT - 1)) * max_val);
}

// I assume that BACKLIGHT_LEVEL_COUNT is low
int get_backlight_level(int current_val, int max_val) {
	int level;
	int diff = abs(get_backlight_value(0, max_val) - current_val);
	for(level = 0; level != BACKLIGHT_LEVEL_COUNT; ++level) {
		int cur_diff = abs(get_backlight_value(level, max_val) - current_val);
		if(cur_diff > diff) break;
		diff = cur_diff;
	}
	return clamp(level - 1, 0, BACKLIGHT_LEVEL_COUNT - 1);
}

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
