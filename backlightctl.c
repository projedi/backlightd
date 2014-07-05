#include <dbus/dbus.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void dbus_sendsignal(char const* name) {
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

	// TODO: not sure about that REPLACE_EXISTING thing
	int res = dbus_bus_request_name(connection, "org.backlightd.client",
			DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if(dbus_error_is_set(&err)) {
		fprintf(stderr, "Requesting name error: %s\n", err.message);
		dbus_error_free(&err);
	}
	if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != res) {
		fprintf(stderr, "Not a primary owner: %d\n", res);
		exit(1);
	}

	DBusMessage* message = dbus_message_new_method_call("org.backlightd.daemon",
			"/org/backlightd/Object", "org.backlightd.Backlight", name);
	if(!message) {
		fprintf(stderr, "Message is NULL\n");
		exit(1);
	}

	dbus_uint32_t serial = 0;
	dbus_connection_send(connection, message, &serial);
	dbus_connection_flush(connection);
	dbus_message_unref(message);
}

void usage(char const* progname) {
	fprintf(stderr, "USAGE: %s {up|down}\n", progname);
}

int main(int argc, char** argv) {
	if(argc != 2) {
		usage(argv[0]);
		return 1;
	}
	if(!strcmp(argv[1], "up")) {
		dbus_sendsignal("Increase");
		return 0;
	} else if(!strcmp(argv[1], "down")) {
		dbus_sendsignal("Decrease");
		return 0;
	} else {
		usage(argv[0]);
		return 1;
	}
}
