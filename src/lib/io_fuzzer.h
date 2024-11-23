/** @file */

#ifndef IO_FUZZER_H
#define IO_FUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#define IO_FUZZER_MAX_INPUT (20 + (sizeof(uint32_t) * UINT16_MAX))

typedef struct _io_fuzzer io_fuzzer_t; /**< I/O address space fuzzer. */

typedef void io_fuzzer_error_handler_t(int status, int error, const char *restrict format, va_list ap);
typedef void io_fuzzer_log_handler_t(FILE *restrict stream, const char *restrict format, va_list ap);

/**
 * Creates an I/O address space fuzzer.
 *
 * @param [in] pci_device PCI device.
 * @param [in] ports List of I/O port addresses.
 * @param [in] num_ports Number of I/O port addresses.
 * @return An I/O address space fuzzer.
 */
io_fuzzer_t *io_fuzzer_create(const int *ports, size_t num_ports);

/**
 * Destroys the I/O address space fuzzer.
 *
 * @param [in] io_fuzzer I/O address space fuzzer.
 */
void io_fuzzer_destroy(io_fuzzer_t *restrict io_fuzzer);

/**
 * Performs an iteration.
 *
 * @param [in] io_fuzzer I/O address space fuzzer.
 * @param [in] stream Input stream.
 */
void io_fuzzer_iterate(io_fuzzer_t *restrict io_fuzzer, FILE *restrict stream);

/**
 * Sets the error handler for the I/O address space fuzzer.
 *
 * @param [in] handler Error handler.
 * @return Previous error handler.
 */
io_fuzzer_error_handler_t *io_fuzzer_set_error_handler(io_fuzzer_error_handler_t *handler);

/**
 * Sets the log handler for the I/O address space fuzzer.
 *
 * @param [in] handler Log handler.
 * @return Previous log handler.
 */
io_fuzzer_log_handler_t *io_fuzzer_set_log_handler(io_fuzzer_t *restrict io_fuzzer, io_fuzzer_log_handler_t *handler);

/**
 * Sets the log stream for the I/O address space fuzzer.
 *
 * @param [in] stream Log stream.
 * @return Previous log stream.
 */
FILE *io_fuzzer_set_log_stream(io_fuzzer_t *restrict io_fuzzer, FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* IO_FUZZER_H */
