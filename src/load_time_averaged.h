
#pragma once

#include "csv/load_csv.h"

#ifndef _LOAD_TIME_AVERAGED_H_
#  define _LOAD_TIME_AVERAGED_H_

#  if defined(__cplusplus)
extern "C" {
#  endif

int load_time_averaged_timeseries(csv_context_t *context, char *filepath);

#  if defined(__cplusplus)
}
#  endif

#endif // _LOAD_TIME_AVERAGED_H_
