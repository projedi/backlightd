#pragma once

#define BACKLIGHT_PATH "/sys/class/backlight/intel_backlight"

extern void backlight_read(int* current_val, int* max_val);

extern void backlight_write(int new_val);
