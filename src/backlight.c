#include <math.h>
#include <stdlib.h>

#include "backlight.h"

#define clamp(v, vmin, vmax) ((v) > (vmax) ? (vmax) : (v) < (vmin) ? (vmin) : (v))

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
