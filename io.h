#pragma once

extern void backlight_read(char const* backlight_path, int* current_val, int* max_val);

extern void backlight_write(char const* backlight_path, int new_val);
