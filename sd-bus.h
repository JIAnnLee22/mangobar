#ifndef MANGOBAR_SD_BUS_H
#define MANGOBAR_SD_BUS_H

#if defined(SDBUS_LIBSYSTEMD)
#include <systemd/sd-bus.h>
#include <systemd/sd-bus-vtable.h>
#elif defined(SDBUS_LIBELOGIND)
#include <elogind/sd-bus.h>
#include <elogind/sd-bus-vtable.h>
#elif defined(SDBUS_BASU)
#include <basu/sd-bus.h>
#include <basu/sd-bus-vtable.h>
#else
#error "No supported sd-bus implementation selected"
#endif

#endif
