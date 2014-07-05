#include <errno.h>
#include <dbus/dbus.h>
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

void dbus_listen() {
	DBusError err;
	dbus_error_init(&err);

	DBusConnection* connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err)) {
		fprintf(stderr, "Connection error: %s\n", err.message);
		dbus_error_free(&err);
	}
	if(!connection) {
		fprintf(stderr, "Connection is NULL\n");
		exit(1);
	}

	int res = dbus_bus_request_name(connection, "org.backlightd.daemon",
			DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if(dbus_error_is_set(&err)) {
		fprintf(stderr, "Requesting name error: %s\n", err.message);
		dbus_error_free(&err);
	}
	if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != res) {
		fprintf(stderr, "Not a primary owner: %d\n", res);
		exit(1);
	}

	while(1) {
		// TODO: Blocking does not work for some reason
		dbus_connection_read_write(connection, 0);
		DBusMessage* message = dbus_connection_pop_message(connection);
		if(!message) {
			usleep(10000);
			continue;
		}

		if(dbus_message_is_method_call(message, "org.backlightd.Backlight", "Increase"))
			backlight_up();
		else if(dbus_message_is_method_call(message, "org.backlightd.Backlight", "Decrease"))
			backlight_down();
		else {
			fprintf(stderr, "Unknown dbus message\n");
			fprintf(stderr, "\tInterface: %s\n", dbus_message_get_interface(message));
			fprintf(stderr, "\tMember: %s\n", dbus_message_get_member(message));
			fprintf(stderr, "\tPath: %s\n", dbus_message_get_path(message));
		}

		dbus_message_unref(message);
	}
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
	dbus_listen();
	return EXIT_SUCCESS;
}
