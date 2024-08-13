
#pragma once

#include "csv/load_csv.h"

#ifndef LOAD_PHASE_WISE_H
#  define LOAD_PHASE_WISE_H

#  if defined(__cplusplus)
extern "C" {
#  endif

typedef struct phase_wise_row_struct {
  int routine;
  double time;
  double ship_volume_lake_to_sea;
  double ship_volume_sea_to_lake;
  double t_flushing;
  double t_level;
  double t_open_lake;
  double t_open_sea;
  double density_current_factor_sea;
  double density_current_factor_lake;
} phase_wise_row_t;

int load_phase_wise_timeseries(csv_context_t *context, char *filepath);

#  if defined(__cplusplus)
}
#  endif

#endif // LOAD_TIME_AVERAGED_H
