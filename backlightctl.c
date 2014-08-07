#include <dbus/dbus.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "backlight.h"
#include "util.h"

static DBusConnection* init_dbus() {
	DBusError err;
	dbus_error_init(&err);

	DBusConnection* connection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if(dbus_error_is_set(&err)) {
		fprintf(stderr, "Connection error: %s\n", err.message);
		dbus_error_free(&err);
		return NULL;
	}
	return connection;
}

typedef int (*append_args_t)(DBusMessage*, void*);

static int append_no_args(DBusMessage* UNUSED(message), void* UNUSED(data)) { return 0; }

static int append_int_arg(DBusMessage* message, void* data) {
	dbus_int32_t v = *(int*)data;
	if(!dbus_message_append_args(message, DBUS_TYPE_INT32, &v, DBUS_TYPE_INVALID)) return 1;
	return 0;
}

static int run_method(DBusConnection* connection, const char* method_name, append_args_t f, int* res,
		void* data) {
	int ret = 1;
	DBusError err;
	dbus_error_init(&err);

	DBusMessage* message = dbus_message_new_method_call("org.backlightd.daemon",
			"/org/backlightd/Object", "org.backlightd.Backlight", method_name);
	if(!message) {
		fprintf(stderr, "Message is NULL\n");
		goto cleanup;
	}
	if(f(message, data)) {
		fprintf(stderr, "Error appending arguments to method call\n");
		goto cleanup;
	}

	DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &err);
	if(dbus_error_is_set(&err)) {
		fprintf(stderr, "Message sending error: %s\n", err.message);
		dbus_error_free(&err);
		goto cleanup;
	}

	dbus_int32_t v;
	dbus_message_get_args(reply, &err, DBUS_TYPE_INT32, &v, DBUS_TYPE_INVALID);
	if(dbus_error_is_set(&err)) {
		fprintf(stderr, "Error getting args from reply: %s\n", err.message);
		dbus_error_free(&err);
		goto cleanup;
	}
	*res = v;
	ret = 0;
cleanup:
	if(message)
		dbus_message_unref(message);
	if(reply)
		dbus_message_unref(reply);
	return ret;
}

typedef int (*calculate_value_t)(int, int);

static int backlight_up(int current_value, int max_value) {
	int level = get_backlight_level(current_value, max_value);
	return get_backlight_value(level + 1, max_value);
}

static int backlight_down(int current_value, int max_value) {
	int level = get_backlight_level(current_value, max_value);
	return get_backlight_value(level - 1, max_value);
}

static int handle_set_value(DBusConnection* connection, calculate_value_t f) {
	int ret = 1;
	int current_value;
	if((ret = run_method(connection, "CurrentValue", append_no_args, &current_value, NULL)))
		goto cleanup;
	int max_value;
	if((ret = run_method(connection, "MaxValue", append_no_args, &max_value, NULL)))
		goto cleanup;
	int new_value = f(current_value, max_value);
	int set_value_ret;
	if((ret = run_method(connection, "SetValue", append_int_arg, &set_value_ret, &new_value)))
		goto cleanup;
	if(set_value_ret) {
		fprintf(stderr, "Error returned from SetValue\n");
		ret = set_value_ret;
		goto cleanup;
	}
	ret = 0;
cleanup:
	return ret;
}

static int handle_increase(DBusConnection* connection) {
	return handle_set_value(connection, backlight_up);
}

static int handle_decrease(DBusConnection* connection) {
	return handle_set_value(connection, backlight_down);
}

static int handle_current_value(DBusConnection* connection) {
	int ret = 1;
	int current_value;
	if((ret = run_method(connection, "CurrentValue", append_no_args, &current_value, NULL)))
		goto cleanup;
	printf("%d\n", current_value);
	ret = 0;
cleanup:
	return ret;
}

int handle_max_value(DBusConnection* connection) {
	int max_value;
	int ret = 1;
	if((ret = run_method(connection, "MaxValue", append_no_args, &max_value, NULL)))
		goto cleanup;
	printf("%d\n", max_value);
	ret = 0;
cleanup:
	return ret;
}

static void usage(char const* progname) {
	fprintf(stderr, "USAGE: %s {up|down|current|max}\n", progname);
}

int main(int argc, char** argv) {
	int ret = 1;
	if(argc != 2) {
		usage(argv[0]);
		return ret;
	}
	DBusConnection* connection = init_dbus();
	if(!connection) return ret;
	if(!strcmp(argv[1], "up"))
		ret = handle_increase(connection);
	else if(!strcmp(argv[1], "down"))
		ret = handle_decrease(connection);
	else if(!strcmp(argv[1], "current"))
		ret = handle_current_value(connection);
	else if(!strcmp(argv[1], "max"))
		ret = handle_max_value(connection);
	else
		usage(argv[0]);

	return ret;
}
