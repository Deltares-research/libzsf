

#include "timestamp.h"

#include <errno.h>
#include <stdlib.h>

// Convert 'double' timestamp to time_t.
// Returns -1 on error.
time_t timestamp_to_time(double timestamp) {
  long long cts = timestamp;
  struct tm ts;

  ts.tm_sec = 0;
  ts.tm_min = cts % 100;
  ts.tm_hour = (cts / 100) % 100;
  ts.tm_mday = (cts / 10000) % 100;
  ts.tm_mon = (cts / 1000000) % 100 - 1;
  ts.tm_year = (cts / 100000000) - 1900;
  ts.tm_wday = 0;
  ts.tm_yday = 0;
  ts.tm_isdst = -1;

  return mktime(&ts);
}

// Duplicate and convert timestamp double array to a time_t array.
// Note: The caller is responsible for deallocating the time_t array.
time_t *timestamp_array_to_times(double *timestamps, size_t length) {
  time_t *times = NULL;

  if (length) {
    times = (time_t *)malloc(sizeof(time_t) * length);
    if (times) {
      for (size_t i = 0; i < length; i++) {
        times[i] = timestamp_to_time(timestamps[i]);
      }
    }
  }

  return times;
}

// Convert time_t time to timstamp.
// Returns -1.0 on error.
double time_to_timestamp(time_t t) {
  double dts = -1.0;
  struct tm *ts;

  if (t >= 0) {
    ts = localtime(&t);
    dts = (ts->tm_year + 1900.0) * 100000000.0;
    dts += (ts->tm_mon + 1.0) * 1000000.0;
    dts += ts->tm_hour * 100.0;
    dts += ts->tm_min;
  }

  return dts;
}

// Convert timestamp 'YYYYMMDDhhmm' string format to time_t.
// Returns -1 on error.
time_t timestamp_string_to_time(const char *str, char **end_ptr) {
  double timestamp = strtod(str, end_ptr);

  if (!errno && ((end_ptr && *end_ptr != str) || !end_ptr)) {
    return timestamp_to_time(timestamp);
  }

  return -1;
}
