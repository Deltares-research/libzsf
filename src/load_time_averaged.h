
#pragma once

#include "csv.h"

#ifndef _READ_TIME_AVERAGED_H_
#  define _READ_TIME_AVERAGED_H_

#  if defined(__cplusplus)
extern "C" {
#  endif

int load_time_averaged_data(csv_context_t *context, char *filepath);

#  if defined(__cplusplus)
}
#  endif

#endif // _READ_TIME_AVERAGED_H_
