#include <dbus/dbus.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void dbus_sendsignal(char const* name, DBusPendingCall** callback) {
	DBusError err;
	dbus_error_init(&err);

	DBusConnection* connection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if(dbus_error_is_set(&err)) {
		fprintf(stderr, "Connection error: %s\n", err.message);
		dbus_error_free(&err);
	}
	if(!connection) {
		fprintf(stderr, "Connection is NULL\n");
		exit(1);
	}

	DBusMessage* message = dbus_message_new_method_call("org.backlightd.daemon",
			"/org/backlightd/Object", "org.backlightd.Backlight", name);
	if(!message) {
		fprintf(stderr, "Message is NULL\n");
		exit(1);
	}

	dbus_uint32_t serial = 0;
	if(callback)
		dbus_connection_send_with_reply(connection, message, callback, DBUS_TIMEOUT_INFINITE);
	else
		dbus_connection_send(connection, message, &serial);
	dbus_connection_flush(connection);
	dbus_message_unref(message);
}

void handleCurrentLevel(DBusPendingCall* callback) {
	dbus_pending_call_block(callback);
	DBusMessage* message = dbus_pending_call_steal_reply(callback);
	dbus_int32_t v;
	dbus_message_get_args(message, 0, DBUS_TYPE_INT32, &v, DBUS_TYPE_INVALID);
	printf("Got %d\n", v);
}

void handleMaxLevel(DBusPendingCall* callback) {
	dbus_pending_call_block(callback);
	DBusMessage* message = dbus_pending_call_steal_reply(callback);
	dbus_int32_t v;
	dbus_message_get_args(message, 0, DBUS_TYPE_INT32, &v, DBUS_TYPE_INVALID);
	printf("Got %d\n", v);
}

void usage(char const* progname) {
	fprintf(stderr, "USAGE: %s {up|down|current}\n", progname);
}

int main(int argc, char** argv) {
	if(argc != 2) {
		usage(argv[0]);
		return 1;
	}
	if(!strcmp(argv[1], "up")) {
		dbus_sendsignal("Increase", 0);
		return 0;
	} else if(!strcmp(argv[1], "down")) {
		dbus_sendsignal("Decrease", 0);
		return 0;
	} else if(!strcmp(argv[1], "current")) {
		DBusPendingCall* callback;
		dbus_sendsignal("CurrentLevel", &callback);
		handleCurrentLevel(callback);
		return 0;
	} else if(!strcmp(argv[1], "max")) {
		DBusPendingCall* callback;
		dbus_sendsignal("MaxLevel", &callback);
		handleMaxLevel(callback);
		return 0;
	} else {
		usage(argv[0]);
		return 1;
	}
}
