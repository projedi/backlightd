#include <errno.h>
#include <dbus/dbus.h>
#include <libudev.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "backlight.h"
#include "io.h"

static int CURRENT_BACKLIGHT_VALUE = -1;

static char* BACKLIGHT_PATH = 0;

void backlight_up(char const* backlight_path) {
	int current_val;
	int max_val;
	backlight_read(backlight_path, &current_val, &max_val);
	int current_level = get_backlight_level(current_val, max_val);
	int new_val = get_backlight_value(current_level + 1, max_val);
	CURRENT_BACKLIGHT_VALUE = new_val;
	backlight_write(backlight_path, new_val);
}

void backlight_down(char const* backlight_path) {
	int current_val;
	int max_val;
	backlight_read(backlight_path, &current_val, &max_val);
	int current_level = get_backlight_level(current_val, max_val);
	int new_val = get_backlight_value(current_level - 1, max_val);
	CURRENT_BACKLIGHT_VALUE = new_val;
	backlight_write(backlight_path, new_val);
}

void backlight_save(char const* backlight_path) {
	int max_val;
	backlight_read(backlight_path, &CURRENT_BACKLIGHT_VALUE, &max_val);
}

void backlight_restore(char const* backlight_path) {
	if(CURRENT_BACKLIGHT_VALUE == -1) return;
	backlight_write(backlight_path, CURRENT_BACKLIGHT_VALUE);
}

void dbus_listen() {
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

		if(dbus_message_is_signal(message, "org.freedesktop.DBus", "NameAcquired"))
			continue;
		if(dbus_message_is_method_call(message, "org.backlightd.Backlight", "Increase"))
			backlight_up(BACKLIGHT_PATH);
		else if(dbus_message_is_method_call(message, "org.backlightd.Backlight", "Decrease"))
			backlight_down(BACKLIGHT_PATH);
		else {
			fprintf(stderr, "Unknown dbus message\n");
			fprintf(stderr, "\tInterface: %s\n", dbus_message_get_interface(message));
			fprintf(stderr, "\tMember: %s\n", dbus_message_get_member(message));
			fprintf(stderr, "\tPath: %s\n", dbus_message_get_path(message));
		}

		dbus_message_unref(message);
	}
}

void* udev_listen(void* data) {
	struct udev* udev = udev_new();
	if(!udev) {
		perror("Cannot create udev device");
		return NULL;
	}
	struct udev_monitor* udev_monitor = udev_monitor_new_from_netlink(udev, "udev");
	if(!udev) {
		perror("Cannot create udev_monitor");
		goto cleanup;
	}
	if(udev_monitor_filter_add_match_subsystem_devtype(udev_monitor, "power_supply", 0) < 0) {
		perror("Cannot add a filter to udev_monitor");
		goto cleanup;
	}
	if(udev_monitor_enable_receiving(udev_monitor) < 0) {
		perror("Cannot receive data on monitor");
		goto cleanup;
	}
	int fd = udev_monitor_get_fd(udev_monitor);
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	while(1) {
		int ret = select(fd + 1, &fds, NULL, NULL, NULL);
		if(ret > 0 && FD_ISSET(fd, &fds)) {
			struct udev_device* dev = udev_monitor_receive_device(udev_monitor);
			if(dev) {
				const char* action = udev_device_get_action(dev);
				const char* prop = udev_device_get_property_value(dev, "POWER_SUPPLY_ONLINE");
				if(prop && !strcmp("change", action) && strcmp("(null)", prop) != 0) {
					backlight_restore(BACKLIGHT_PATH);
				}
				udev_device_unref(dev);
			} else {
				perror("No device from receive device");
			}
		}
	}
cleanup:
	if(udev) udev_unref(udev);
	if(udev_monitor) udev_monitor_unref(udev_monitor);
	return NULL;
}

int detect_backlight_device() {
	int res = 0;
	struct udev* udev = udev_new();
	if(!udev) {
		perror("Cannot create udev device");
		goto cleanup;
	}
	struct udev_enumerate* udev_enumerate = udev_enumerate_new(udev);
	if(udev_enumerate_add_match_subsystem(udev_enumerate, "backlight") < 0) {
		perror("Cannot add match rule");
		goto cleanup;
	}
	if(udev_enumerate_scan_devices(udev_enumerate) < 0) {
		perror("Cannot scan devices");
		goto cleanup;
	}
	struct udev_list_entry* udev_list_entry = udev_enumerate_get_list_entry(udev_enumerate);
	if(!udev_list_entry) {
		perror("No backlight devices found");
		goto cleanup;
	}
	char const* devname = udev_list_entry_get_name(udev_list_entry);
	if(BACKLIGHT_PATH)
		free(BACKLIGHT_PATH);
	BACKLIGHT_PATH = calloc(strlen(devname) + 1, sizeof(char));
	strcpy(BACKLIGHT_PATH, devname);
	if(udev_list_entry_get_next(udev_list_entry))
		fprintf(stderr, "Warning: more than one backlight device present, selecting the first\n");
	res = 1;
cleanup:
	if(udev) udev_unref(udev);
	if(udev_enumerate) udev_enumerate_unref(udev_enumerate);
	return res;
}

int main() {
	if(!detect_backlight_device())
		return EXIT_FAILURE;
	backlight_save(BACKLIGHT_PATH);
	umask(0);
	pthread_t t;
	if(pthread_create(&t, NULL, udev_listen, NULL)) {
		perror("Cannot create a thread");
		exit(EXIT_FAILURE);
	}
	dbus_listen();
	return EXIT_SUCCESS;
}
