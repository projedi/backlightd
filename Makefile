.PHONY: all clean

DESTBIN := $(DESTDIR)usr/bin
DESTPRIVATE := $(DESTDIR)usr/lib/backlightd
DESTSYSTEMDSERVICE := $(DESTDIR)usr/lib/systemd/system
DESTDBUSCONF := $(DESTDIR)etc/dbus-1/system.d

SOURCEDIR := src
BUILDDIR := build

TARGET_DAEMON := backlightd
TARGET_CLIENT := backlightctl

SOURCES_DAEMON := backlightd.c io.c
OBJECTS_DAEMON := $(SOURCES_DAEMON:%.c=$(BUILDDIR)/%.o)
DEPENDS_DAEMON := $(SOURCES_DAEMON:%.c=$(BUILDDIR)/%.d)

SOURCES_CLIENT := backlightctl.c backlight.c
OBJECTS_CLIENT := $(SOURCES_CLIENT:%.c=$(BUILDDIR)/%.o)
DEPENDS_CLIENT := $(SOURCES_CLIENT:%.c=$(BUILDDIR)/%.d)

CC ?= gcc

CFLAGS += -g -std=c99 -Wall -Wextra -pthread $(shell pkg-config --cflags dbus-1 libudev)

LDFLAGS += -pthread $(shell pkg-config --libs-only-L dbus-1 libudev)

LIBS += -lm $(shell pkg-config --libs-only-l dbus-1 libudev)

all: $(TARGET_DAEMON) $(TARGET_CLIENT)

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.d: $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) -MF $@ -MG -MM -MP -MT $(^:.c=.o) $<

$(TARGET_DAEMON): $(OBJECTS_DAEMON)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(TARGET_DAEMON)

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(TARGET_CLIENT)

clean:
	rm -f $(TARGET_DAEMON) $(TARGET_CLIENT)
	rm -f $(OBJECTS_DAEMON) $(DEPENDS_DAEMON) $(OBJECTS_CLIENT) $(DEPENDS_CLIENT)

install:
	mkdir -m 755 -p $(DESTBIN)
	mkdir -m 755 -p $(DESTPRIVATE)
	mkdir -m 755 -p $(DESTSYSTEMDSERVICE)
	mkdir -m 755 -p $(DESTDBUSCONF)
	install -m 755 $(TARGET_CLIENT) $(DESTBIN)
	install -m 755 $(TARGET_DAEMON) $(DESTPRIVATE)
	install -m 644 extra/backlightd.service $(DESTSYSTEMDSERVICE)
	install -m 644 extra/org.backlightd.daemon.conf $(DESTDBUSCONF)

include $(DEPENDS_DAEMON) $(DEPENDS_CLIENT)
