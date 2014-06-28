#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "backlight.h"
#include "io.h"

#define FIFO_PATH "/tmp/backlightd.fifo"

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

int open_fifo() {
	int ret;
	if((ret = mkfifo(FIFO_PATH, 0666)) == -1) {
		if(errno == EEXIST) {
			if(unlink(FIFO_PATH) == -1) {
				perror("Cannot unlink " FIFO_PATH);
				exit(EXIT_FAILURE);
			}
			ret = mkfifo(FIFO_PATH, 0666);
		}
		if(ret == -1) {
			perror("Cannot create fifo " FIFO_PATH);
			exit(EXIT_FAILURE);
		}
	}
	return open(FIFO_PATH, O_RDONLY);
}

int main() {
	umask(0);
	for(;;) {
		int fifo = open_fifo();
		int v = 0;
		int ret;
		if((ret = read(fifo, &v, 1)) != 1) {
			if(ret == -1) {
				perror("Cannot read from " FIFO_PATH);
				exit(EXIT_FAILURE);
			}
			fprintf(stderr, "Read %d bytes from " FIFO_PATH " expected 1\n", ret);
			exit(EXIT_FAILURE);
		}
		if(v == 0) backlight_down();
		else if(v == 1) backlight_up();
		else fprintf(stderr, "Expected either 0 or 1, got %d\n", v);
		close(fifo);
	}
	return EXIT_SUCCESS;
}
