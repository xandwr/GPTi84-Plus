/*
 * libtilemcore expects the embedder to provide these glue functions
 * (memory + logging). The vendored TilEm GUI implements them on top of
 * glib in vendor/tilem/gui/memory.c; we intentionally do without glib
 * here so the headless harness has no extra runtime deps.
 *
 * Logging is routed to stderr unconditionally. tilem_internal aborts
 * on the assumption that an internal-error message indicates a bug
 * worth surfacing immediately rather than silently continuing.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tilem.h>

void  tilem_free(void *p)               { free(p); }
void *tilem_malloc(size_t s)            { return malloc(s); }
void *tilem_malloc0(size_t s)           { return calloc(1, s); }
void *tilem_malloc_atomic(size_t s)     { return malloc(s); }
void *tilem_try_malloc(size_t s)        { return malloc(s); }
void *tilem_try_malloc0(size_t s)       { return calloc(1, s); }
void *tilem_try_malloc_atomic(size_t s) { return malloc(s); }
void *tilem_realloc(void *p, size_t s)  { return realloc(p, s); }

const char *tilem_gettext(const char *msg) { return msg; }

void tilem_message(TilemCalc *calc, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "x%c: ", calc->hw.model_id);
    vfprintf(stderr, msg, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void tilem_warning(TilemCalc *calc, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "x%c: WARNING: ", calc->hw.model_id);
    vfprintf(stderr, msg, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void tilem_internal(TilemCalc *calc, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "x%c: INTERNAL ERROR: ", calc->hw.model_id);
    vfprintf(stderr, msg, ap);
    fputc('\n', stderr);
    va_end(ap);
}
