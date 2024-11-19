
#include "log.h"

#include <assert.h>

logger_t logger;

void log_init(char *name, FILE *fp) {
  logger.name = name;
  logger.fp = fp;
  logger.level = logWARNING;
}

void log_set_level(log_level_t lvl) {
  logger.level = lvl;
}

static void vlog(log_level_t lvl, char *fmt, va_list ap) {
  if (lvl >= logger.level && lvl <= logERROR) {
    char *lvlstr[logERROR+1] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    FILE *stream = logger.fp ? logger.fp : stderr;
    fprintf(stream, "[%s] %s: ", lvlstr[lvl], logger.name);
    vfprintf(stream, fmt, ap);
  }
}

void log_msg(log_level_t lvl, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(lvl, fmt, ap);
  va_end(ap);
}

void log_debug(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(logDEBUG, fmt, ap);
  va_end(ap);
}

void log_info(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(logINFO, fmt, ap);
  va_end(ap);
}

void log_warning(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(logWARNING, fmt, ap);
  va_end(ap);
}

void log_error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(logERROR, fmt, ap);
  va_end(ap);
}
