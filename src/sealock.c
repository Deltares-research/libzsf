

#include "sealock.h"
#include "load_phase_wise.h"
#include "load_time_averaged.h"
#include "timestamp.h"

#include <stdlib.h>


int sealock_init(sealock_state_t* lock, time_t start_time) {
  int status = SEALOCK_OK;
  // Init calculation parameters with defaults.
  zsf_param_default(&lock->parameters);

  // Load timeseries data when required.
  if (lock->operational_parameters_file) {
    status = sealock_load_timeseries(lock, lock->operational_parameters_file);
  }

  if (status == SEALOCK_OK) {
    // Do one update to properly populate all parameters from timeseries for current time.
    status = sealock_set_parameters_for_time(lock, start_time);
  }

  if (status == SEALOCK_OK) {
    // Initialize parameters consistent with current and given settings.
    status = zsf_initialize_state(&lock->parameters, &lock->phase_state,
                                  lock->phase_state.salinity_lock, lock->phase_state.head_lock);
  }

  return status;
}

int sealock_load_timeseries(sealock_state_t *lock, char *filepath) {
  int status = SEALOCK_OK;
  double *time_column = NULL;
  size_t num_rows = 0;

  // read csv data
  switch (lock->computation_mode) {
  case cycle_average_mode:
    status = load_time_averaged_timeseries(&lock->timeseries_data, filepath) ? SEALOCK_ERROR
                                                                             : SEALOCK_OK;
    break;
  case phase_wise_mode:
    status = load_phase_wise_timeseries(&lock->timeseries_data, filepath) ? SEALOCK_ERROR
                                                                          : SEALOCK_OK;
    break;
  default:
    status = SEALOCK_ERROR;
    break;
  }

  if (status == SEALOCK_OK) {
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
          if (times_strictly_increasing(lock->times, num_rows)) {
            lock->times_len = num_rows;
          } else {
            free(lock->times);
            lock->times = NULL;
            status = SEALOCK_ERROR;
          }
        } else {
          status = SEALOCK_ERROR;
        }
      }
      free(time_column);
    }
  }

  return status;
}

// Returns 1 if we skipped to a new row.
static int sealock_update_current_row(sealock_state_t *lock, time_t time) {
  size_t row = lock->current_row;
  size_t previous_row = row;
  while (time > lock->times[row] && row < lock->times_len) {
    row++;
  }
  lock->current_row = row ? row - 1 : 0;
  return lock->current_row != previous_row;
}

static int sealock_update_cycle_average_parameters(sealock_state_t *lock, time_t time) {
  sealock_update_current_row(lock, time);
  return get_csv_row_data(&lock->timeseries_data, lock->current_row, &lock->parameters) == CSV_OK
             ? SEALOCK_OK
             : SEALOCK_ERROR;
}

static int sealock_cycle_average_step(sealock_state_t *lock, time_t time) {
  int status = SEALOCK_OK;

  status = zsf_calc_steady(&lock->parameters, &lock->results, NULL);
  // Adjust result value signs to align with D-Flow FM conventions.
  if (status == SEALOCK_OK) {
    lock->results.discharge_from_lake = -lock->results.discharge_from_lake;
    lock->results.discharge_from_sea = -lock->results.discharge_from_sea;
  }

  return status;
}

static int sealock_update_phase_wise_parameters(sealock_state_t *lock, time_t time) {
  int status = SEALOCK_OK;
  phase_wise_row_t row_data = PHASE_WISE_CLEAR_ROW();
  if (sealock_update_current_row(lock, time)) {
    status = get_csv_row_data(&lock->timeseries_data, lock->current_row, &row_data);
    if (status == SEALOCK_OK) {
      lock->phase_args = PHASE_WISE_CLEAR_ARGS();
      // copy relevant args to lock.
      lock->phase_args.run_update = 1;
      lock->phase_args.routine = row_data.routine;
      lock->parameters.density_current_factor_sea = row_data.density_current_factor_sea;
      lock->parameters.density_current_factor_lake = row_data.density_current_factor_lake;
      lock->parameters.ship_volume_sea_to_lake = 0;
      lock->parameters.ship_volume_lake_to_sea = 0;
      switch (row_data.routine) {
      case 1:
      case 3:
        lock->phase_args.t_level = row_data.t_level;
        break;
      case 2:
        lock->phase_args.t_open_lake = row_data.t_open_lake;
        lock->parameters.ship_volume_lake_to_sea = row_data.ship_volume_lake_to_sea;
        break;
      case 4:
        lock->phase_args.t_open_sea = row_data.t_open_sea;
        lock->parameters.ship_volume_sea_to_lake = row_data.ship_volume_sea_to_lake;
        break;
      default:
        if (row_data.routine < 0) {
          lock->phase_args.t_flushing = row_data.t_flushing;
        } else {
          status = SEALOCK_ERROR;
        }
        break;
      }
    }
  } else {
    lock->phase_args.run_update = 0;
  }
  return status;
}

static int sealock_phase_wise_step(sealock_state_t *lock, time_t time) {
  int status = SEALOCK_OK;

  switch (lock->phase_args.routine) {
  case 1:
    status = zsf_step_phase_1(&lock->parameters, lock->phase_args.t_level, &lock->phase_state,
                              &lock->phase_results);
    break;
  case 2:
    status = zsf_step_phase_2(&lock->parameters, lock->phase_args.t_open_lake, &lock->phase_state,
                              &lock->phase_results);
    break;
  case 3:
    status = zsf_step_phase_3(&lock->parameters, lock->phase_args.t_level, &lock->phase_state,
                              &lock->phase_results);
    break;
  case 4:
    status = zsf_step_phase_4(&lock->parameters, lock->phase_args.t_open_sea, &lock->phase_state,
                              &lock->phase_results);
    break;
  default:
    if (lock->phase_args.routine < 0) {
      status = zsf_step_flush_doors_closed(&lock->parameters, lock->phase_args.t_flushing,
                                           &lock->phase_state, &lock->phase_results);
    } else {
      status = SEALOCK_ERROR;
    }
    break;
  }

  return status;
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
      status = sealock_cycle_average_step(lock, time);
      break;
    case phase_wise_mode:
      status = sealock_phase_wise_step(lock, time);
      break;
    default:
      status = SEALOCK_ERROR; // Should never happen.
      break;
    }
  }
  return status;
}

// Check if none of
int sealock_delta_time_ok(sealock_state_t* lock, time_t delta_time) {
  for (int i = 0; i < lock->times_len-1; i++) {
    if (lock->times[i + 1] - lock->times[i] < delta_time) {
      return 0;
    }
  }
  return 1;
}