.PHONY: all clean

TARGET_DAEMON := backlightd
TARGET_CLIENT := backlightctl

SOURCES_DAEMON := backlightd.c io.c backlight.c
OBJECTS_DAEMON := $(SOURCES_DAEMON:.c=.o)
DEPENDS_DAEMON := $(SOURCES_DAEMON:.c=.d)

SOURCES_CLIENT := backlightctl.c
OBJECTS_CLIENT := $(SOURCES_CLIENT:.c=.o)
DEPENDS_CLIENT := $(SOURCES_CLIENT:.c=.d)

CC ?= gcc

CFLAGS += -g -std=c99 -Wall -Wextra -pthread $(shell pkg-config --cflags dbus-1 libudev)

LDFLAGS += -pthread $(shell pkg-config --libs-only-L dbus-1 libudev)

LIBS += -lm $(shell pkg-config --libs-only-l dbus-1 libudev)

all: $(TARGET_DAEMON) $(TARGET_CLIENT)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.d: %.c
	$(CC) $(CFLAGS) -MF $@ -MG -MM -MP -MT $(^:.c=.o) $<

$(TARGET_DAEMON): $(OBJECTS_DAEMON)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(TARGET_DAEMON)

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(TARGET_CLIENT)

clean:
	rm -f $(TARGET_DAEMON) $(TARGET_CLIENT)
	rm -f $(OBJECTS_DAEMON) $(DEPENDS_DAEMON) $(OBJECTS_CLIENT) $(DEPENDS_CLIENT)

include $(DEPENDS_DAEMON) $(DEPENDS_CLIENT)
