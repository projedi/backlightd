.PHONY: setuid-root

setuid-root: backlight_helper
	sudo chown root backlight_helper
	sudo chmod u+s backlight_helper

backlight_helper: main.c
	gcc -std=c99 -Wall -Wextra main.c -o backlight_helper -lm
