
#include <stdlib.h>

#include "sealock.h"
#include "load_time_averaged.h"


int sealock_load_data(sealock_state_t* lock, char* filepath) {
  int status = SEALOCK_OK;
  // read csv data
  if (!load_time_averaged_data(&lock->timeseries_data, filepath))
  {
    lock->current_row = 0;
    lock->times_len = get_csv_num_rows(&lock->timeseries_data);
    if (lock->times_len == 0) { return SEALOCK_ERROR; }
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


int _sealock_update_cycle_average(sealock_state_t* lock, double time) {
  size_t row = lock->current_row;
  while (time > lock->times[row] && row < lock->times_len) { row++; }
  lock->current_row = row ? row - 1 : SEALOCK_OK;
  get_csv_row_data(&lock->timeseries_data, lock->current_row, &lock->parameters);
  return zsf_calc_steady(&lock->parameters, &lock->results, NULL);
}


int _sealock_update_phase_wise(sealock_state_t *lock, double time) {
  // TODO: implement me
  return SEALOCK_OK;
}


int sealock_update(sealock_state_t *lock, double time) {
  int status = SEALOCK_OK;

  switch (lock->computation_mode) {
    case cycle_average_mode:
      status = _sealock_update_cycle_average(lock, time);
      break;
    case phase_wise_mode:
      status = _sealock_update_phase_wise(lock, time);
      break;
    default:
      status = SEALOCK_ERROR; // Should never happen.
      break;
  }
  return status;
}
