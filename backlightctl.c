#include <dbus/dbus.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "backlight.h"

DBusConnection* init_dbus() {
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

int appendNoArgs(DBusMessage* message, void* data) { return 0; }

int appendIntArg(DBusMessage* message, void* data) {
	dbus_int32_t v = *(int*)data;
	if(!dbus_message_append_args(message, DBUS_TYPE_INT32, &v, DBUS_TYPE_INVALID)) return 1;
	return 0;
}

int runMethod(DBusConnection* connection, const char* method_name, append_args_t f, int* res, void* data) {
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

int backlightUp(int current_value, int max_value) {
	int level = get_backlight_level(current_value, max_value);
	return get_backlight_value(level + 1, max_value);
}

int backlightDown(int current_value, int max_value) {
	int level = get_backlight_level(current_value, max_value);
	return get_backlight_value(level - 1, max_value);
}

int handleSetValue(DBusConnection* connection, calculate_value_t f) {
	int ret = 1;
	int current_value;
	if((ret = runMethod(connection, "CurrentValue", appendNoArgs, &current_value, NULL)))
		goto cleanup;
	int max_value;
	if((ret = runMethod(connection, "MaxValue", appendNoArgs, &max_value, NULL)))
		goto cleanup;
	int new_value = f(current_value, max_value);
	int set_value_ret;
	if((ret = runMethod(connection, "SetValue", appendIntArg, &set_value_ret, &new_value)))
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

int handleIncrease(DBusConnection* connection) {
	return handleSetValue(connection, backlightUp);
}

int handleDecrease(DBusConnection* connection) {
	return handleSetValue(connection, backlightDown);
}

int handleCurrentValue(DBusConnection* connection) {
	int ret = 1;
	int current_value;
	if((ret = runMethod(connection, "CurrentValue", appendNoArgs, &current_value, NULL)))
		goto cleanup;
	printf("%d\n", current_value);
	ret = 0;
cleanup:
	return ret;
}

int handleMaxValue(DBusConnection* connection) {
	int max_value;
	int ret = 1;
	if((ret = runMethod(connection, "MaxValue", appendNoArgs, &max_value, NULL)))
		goto cleanup;
	printf("%d\n", max_value);
	ret = 0;
cleanup:
	return ret;
}

void usage(char const* progname) {
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
		ret = handleIncrease(connection);
	else if(!strcmp(argv[1], "down"))
		ret = handleDecrease(connection);
	else if(!strcmp(argv[1], "current"))
		ret = handleCurrentValue(connection);
	else if(!strcmp(argv[1], "max"))
		ret = handleMaxValue(connection);
	else
		usage(argv[0]);

	return ret;
}
