/** @file */

#include "io_fuzzer.h"

#include "input.h"
#include "io.h"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PORTS 65536
#define MAX_STRING (sizeof(uint32_t) * UINT16_MAX)

struct _io_fuzzer {
    const int *ports;
    size_t num_ports;
    io_fuzzer_log_handler_t *log_handler;
    FILE *log_stream;
};

static io_fuzzer_error_handler_t *error_handler = NULL;

void io_fuzzer_error(io_fuzzer_t *restrict io_fuzzer, int status, int error, const char *restrict format, ...);
void io_fuzzer_log(io_fuzzer_t *restrict io_fuzzer, const char *restrict format, ...);

io_fuzzer_t *
io_fuzzer_create(const int *ports, size_t num_ports)
{
    io_fuzzer_t *io_fuzzer = (io_fuzzer_t *)calloc(1, sizeof(*io_fuzzer));
    if (io_fuzzer == NULL) {
        io_fuzzer_error(io_fuzzer, 0, errno, __func__);
        return NULL;
    }

    io_fuzzer->ports = ports;
    io_fuzzer->num_ports = num_ports;
    return io_fuzzer;
}

void
io_fuzzer_destroy(io_fuzzer_t *restrict io_fuzzer)
{
    if (io_fuzzer == NULL) {
        return;
    }

    free(io_fuzzer);
}

void
io_fuzzer_error(io_fuzzer_t *restrict io_fuzzer, int status, int error, const char *restrict format, ...)
{
    if (error_handler == NULL) {
        return;
    }

    va_list ap;
    va_start(ap, format);
    (*error_handler)(status, error, format, ap);
    va_end(ap);
}

void
io_fuzzer_iterate(io_fuzzer_t *restrict io_fuzzer, FILE *restrict stream)
{
    uint16_t port = 0;
    if (io_fuzzer->ports == NULL || io_fuzzer->num_ports == 0) {
        port = input_derive_range(stream, 0, MAX_PORTS - 1);
    } else {
        size_t port_num = input_derive_range(stream, 0, io_fuzzer->num_ports - 1);
        port = io_fuzzer->ports[port_num];
    }

    uint8_t string[MAX_STRING];
    switch (input_derive_range(stream, 0, 11)) {
    case 0: {
        io_fuzzer_log(io_fuzzer, "su", "function", "io_read16", "port", port);
        io_read16(port);
        break;
    }

    case 1: {
        io_fuzzer_log(io_fuzzer, "su", "function", "io_read32", "port", port);
        io_read32(port);
        break;
    }

    case 2: {
        io_fuzzer_log(io_fuzzer, "su", "function", "io_read8", "port", port);
        io_read8(port);
        break;
    }

    case 3: {
        size_t count = input_read16(stream);
        io_fuzzer_log(
                io_fuzzer, "suuu", "function", "io_read_string16", "port", port, "string", string, "count", count);
        io_read_string16(port, (uint16_t *)string, count);
        break;
    }

    case 4: {
        size_t count = input_read16(stream);
        io_fuzzer_log(
                io_fuzzer, "suuu", "function", "io_read_string32", "port", port, "string", string, "count", count);
        io_read_string32(port, (uint32_t *)string, count);
        break;
    }

    case 5: {
        size_t count = input_read16(stream);
        io_fuzzer_log(io_fuzzer, "suuu", "function", "io_read_string8", "port", port, "string", string, "count", count);
        io_read_string8(port, string, count);
        break;
    }

    case 6: {
        uint16_t value = input_read16(stream);
        io_fuzzer_log(io_fuzzer, "suu", "function", "io_write16", "port", port, "value", value);
        io_write16(port, value);
        break;
    }

    case 7: {
        uint32_t value = input_read32(stream);
        io_fuzzer_log(io_fuzzer, "suu", "function", "io_write32", "port", port, "value", value);
        io_write32(port, value);
        break;
    }

    case 8: {
        uint8_t value = input_read8(stream);
        io_fuzzer_log(io_fuzzer, "suu", "function", "io_write8", "port", port, "value", value);
        io_write8(port, value);
        break;
    }

    case 9: {
        size_t count = input_read16(stream);
        input_read_string16(stream, (uint16_t *)string, count);
        io_fuzzer_log(
                io_fuzzer, "suuu", "function", "io_write_string16", "port", port, "string", string, "count", count);
        io_write_string16(port, (uint16_t *)string, count);
        break;
    }

    case 10: {
        size_t count = input_read16(stream);
        input_read_string32(stream, (uint32_t *)string, count);
        io_fuzzer_log(
                io_fuzzer, "suuu", "function", "io_write_string32", "port", port, "string", string, "count", count);
        io_write_string32(port, (uint32_t *)string, count);
        break;
    }

    case 11: {
        size_t count = input_read16(stream);
        input_read_string8(stream, string, count);
        io_fuzzer_log(
                io_fuzzer, "suuu", "function", "io_write_string8", "port", port, "string", string, "count", count);
        io_write_string8(port, string, count);
        break;
    }

    default:
        abort();
    }
}

void
io_fuzzer_log(io_fuzzer_t *restrict io_fuzzer, const char *restrict format, ...)
{
    if (io_fuzzer->log_handler == NULL) {
        return;
    }

    va_list ap;
    va_start(ap, format);
    (*io_fuzzer->log_handler)(io_fuzzer->log_stream, format, ap);
    va_end(ap);
}

io_fuzzer_error_handler_t *
io_fuzzer_set_error_handler(io_fuzzer_error_handler_t *handler)
{
    io_fuzzer_error_handler_t *previous_handler = error_handler;
    error_handler = handler;
    return previous_handler;
}

io_fuzzer_log_handler_t *
io_fuzzer_set_log_handler(io_fuzzer_t *restrict io_fuzzer, io_fuzzer_log_handler_t *handler)
{
    io_fuzzer_log_handler_t *previous_handler = io_fuzzer->log_handler;
    io_fuzzer->log_handler = handler;
    return previous_handler;
}

FILE *
io_fuzzer_set_log_stream(io_fuzzer_t *restrict io_fuzzer, FILE *stream)
{
    FILE *previous_stream = io_fuzzer->log_stream;
    io_fuzzer->log_stream = stream;
    return previous_stream;
}
