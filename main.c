#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "backlight.h"
#include "io.h"

#define FIFO_PATH "/tmp/backlightd.fifo"
#define ACPID_SOCKET "/var/run/acpid.socket"

static int CURRENT_BACKLIGHT_VALUE = -1;

void backlight_up() {
	int current_val;
	int max_val;
	backlight_read(&current_val, &max_val);
	int current_level = get_backlight_level(current_val, max_val);
	int new_val = get_backlight_value(current_level + 1, max_val);
	CURRENT_BACKLIGHT_VALUE = new_val;
	backlight_write(new_val);
}

void backlight_down() {
	int current_val;
	int max_val;
	backlight_read(&current_val, &max_val);
	int current_level = get_backlight_level(current_val, max_val);
	int new_val = get_backlight_value(current_level - 1, max_val);
	CURRENT_BACKLIGHT_VALUE = new_val;
	backlight_write(new_val);
}

void backlight_save() {
	int max_val;
	backlight_read(&CURRENT_BACKLIGHT_VALUE, &max_val);
}

void backlight_restore() {
	if(CURRENT_BACKLIGHT_VALUE == -1) return;
	backlight_write(CURRENT_BACKLIGHT_VALUE);
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

void handle_acpi_event(char const* buf) {
	char copy_buf[4097];
	strncpy(copy_buf, buf, sizeof(copy_buf));
	char const* type = copy_buf;
	char const* name = NULL;
	char const* data1 = NULL;
	char const* data2 = NULL;
	char const** splits[] = { &name, &data1, &data2 };
	int on_splitter = 0;
	int split_idx = 0;
	char* sbuf = copy_buf;
	for(; *sbuf; ++sbuf) {
		if(*sbuf == ' ' || *sbuf == '\t') {
			on_splitter = 1;
			*sbuf = 0;
			continue;
		}
		if(on_splitter)
			*splits[split_idx++] = sbuf;
		on_splitter = 0;
	}
	if(split_idx != 3) return;
	if(!strcmp(type, "ac_adapter")) {
		backlight_restore();
	}
}

void* acpi_listen(void* data) {
	int s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s == -1) {
		perror("Cannot open socket");
		exit(EXIT_FAILURE);
	}
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, ACPID_SOCKET, sizeof(addr.sun_path) - 1);
	if(connect(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("Cannot connect to acpid socket " ACPID_SOCKET);
		exit(EXIT_FAILURE);
	}
	char buf[4097];
	for(;;) {
		int len = read(s, buf, sizeof(buf) - 1);
		if(len == -1 && errno != EAGAIN) {
			perror("Cannot read from ACPID_SOCKET " ACPID_SOCKET);
			exit(EXIT_FAILURE);
		}
		if(len <= 0) continue;
		buf[len] = 0;
		handle_acpi_event(buf);
	}
	return NULL;
}

int main() {
	backlight_save();
	umask(0);
	pthread_t t;
	if(pthread_create(&t, NULL, acpi_listen, NULL)) {
		perror("Cannot create a thread");
		exit(EXIT_FAILURE);
	}
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
