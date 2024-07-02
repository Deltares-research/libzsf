

#include "sealock.h"
#include "load_time_averaged.h"
#include "timestamp.h"

#include <stdlib.h>

int sealock_load_timeseries(sealock_state_t *lock, char *filepath) {
  int status = SEALOCK_OK;
  double *time_column = NULL;
  size_t num_rows = 0;

  // read csv data
  if (!load_time_averaged_timeseries(&lock->timeseries_data, filepath)) {
    lock->current_row = 0;
    num_rows = get_csv_num_rows(&lock->timeseries_data);
    if (!num_rows)
      return SEALOCK_ERROR;
    time_column = malloc(num_rows * sizeof(double));
    if (time_column != NULL) {
      status = get_csv_column_data(&lock->timeseries_data, "time", time_column, num_rows);
      if (status == SEALOCK_OK) {
        lock->times = timestamp_array_to_times(time_column, num_rows);
        if (lock->times) {
          lock->times_len = num_rows;
        } else {
          status = SEALOCK_ERROR;
        }
      }
      free(time_column);
      return status;
    }
  }

  return SEALOCK_ERROR;
}

static void sealock_update_current_row(sealock_state_t *lock, time_t time) {
  size_t row = lock->current_row;
  while (time > lock->times[row] && row < lock->times_len) {
    row++;
  }
  lock->current_row = row ? row - 1 : 0;
}

static int sealock_update_cycle_average_parameters(sealock_state_t *lock, time_t time) {
  sealock_update_current_row(lock, time);
  return get_csv_row_data(&lock->timeseries_data, lock->current_row, &lock->parameters) == CSV_OK
             ? SEALOCK_OK
             : SEALOCK_ERROR;
}

static int sealock_update_phase_wise_parameters(sealock_state_t *lock, time_t time) {
  sealock_update_current_row(lock, time);
  // TODO: implement me: See UNST-7866.
  return SEALOCK_OK;
}

int sealock_set_parameters_for_time(sealock_state_t *lock, time_t time) {
  int status = SEALOCK_OK;

  switch (lock->computation_mode) {
  case cycle_average_mode:
    status = sealock_update_cycle_average_parameters(lock, time);
    break;
  case phase_wise_mode:
    status = sealock_update_phase_wise_parameters(lock, time);
    break;
  default:
    status = SEALOCK_ERROR; // Should never happen.
    break;
  }
  return status;
}

int sealock_update(sealock_state_t *lock, time_t time) {
  int status = sealock_set_parameters_for_time(lock, time);
  if (status == SEALOCK_OK) {
    switch (lock->computation_mode) {
    case cycle_average_mode:
      status = zsf_calc_steady(&lock->parameters, &lock->results, NULL);
      break;
    case phase_wise_mode:
      // TODO: Implement me: See UNST-7866.
      break;
    default:
      status = SEALOCK_ERROR; // Should never happen.
      break;
    }
  }
  return status;
}
