#pragma once

extern char* BACKLIGHT_PATH;

extern void backlight_read(int* current_val, int* max_val);

extern void backlight_write(int new_val);
