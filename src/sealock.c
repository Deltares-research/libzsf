
#include <stdlib.h>

#include "load_time_averaged.h"
#include "sealock.h"

int sealock_load_timeseries(sealock_state_t *lock, char *filepath) {
  int status = SEALOCK_OK;
  // read csv data
  if (!load_time_averaged_timeseries(&lock->timeseries_data, filepath)) {
    lock->current_row = 0;
    lock->times_len = get_csv_num_rows(&lock->timeseries_data);
    if (lock->times_len == 0) {
      return SEALOCK_ERROR;
    }
    lock->times = malloc(lock->times_len * sizeof(double));
    if (lock->times != NULL) {
      return get_csv_column_data(&lock->timeseries_data, "time", lock->times, lock->times_len);
    } else {
      lock->times_len = 0;
      return SEALOCK_ERROR;
    }
  }
  return SEALOCK_ERROR;
}

void _sealock_update_current_row(sealock_state_t *lock, double time) {
  size_t row = lock->current_row;
  while (time > lock->times[row] && row < lock->times_len) {
    row++;
  }
  lock->current_row = row ? row - 1 : 0;
}

int _sealock_update_cycle_average_parameters(sealock_state_t *lock, double time) {
  _sealock_update_current_row(lock, time);
  return get_csv_row_data(&lock->timeseries_data, lock->current_row, &lock->parameters) == CSV_OK
             ? SEALOCK_OK
             : SEALOCK_ERROR;
}

int _sealock_update_phase_wise_parameters(sealock_state_t *lock, double time) {
  _sealock_update_current_row(lock, time);
  // TODO: implement me: See UNST-7866.
  return SEALOCK_OK;
}

int sealock_set_parameters_for_time(sealock_state_t *lock, double time) {
  int status = SEALOCK_OK;

  switch (lock->computation_mode) {
  case cycle_average_mode:
    status = _sealock_update_cycle_average_parameters(lock, time);
    break;
  case phase_wise_mode:
    status = _sealock_update_phase_wise_parameters(lock, time);
    break;
  default:
    status = SEALOCK_ERROR; // Should never happen.
    break;
  }
  return status;
}

int sealock_update(sealock_state_t *lock, double time) {
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
