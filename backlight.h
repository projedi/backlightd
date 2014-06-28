#pragma once

#define BACKLIGHT_LEVEL_COUNT 10

extern int get_backlight_value(int current_level, int max_val);

// I assume that BACKLIGHT_LEVEL_COUNT is low
extern int get_backlight_level(int current_val, int max_val);
