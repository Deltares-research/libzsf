

#include "sealock.h"
#include "load_phase_wise.h"
#include "load_time_averaged.h"
#include "timestamp.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int sealock_defaults(sealock_state_t* lock) {
  // Init calculation parameters with defaults.
  zsf_param_default(&lock->parameters);
  // Set up default volumes/profile for '2D' case.
  lock->lake_volumes.volumes[0] = 1.0;
  lock->lake_volumes.first_active_cell = 0;
  lock->lake_volumes.num_active_cells = 1;
  lock->sea_volumes.volumes[0] = 1.0;
  lock->sea_volumes.first_active_cell = 0;
  lock->sea_volumes.num_active_cells = 1;
  return io_layer_init_2d(&lock->flow_profile);
}


int sealock_init(sealock_state_t* lock, time_t start_time) {
  int status = SEALOCK_OK;

  // Load timeseries data when required.
  if (status == SEALOCK_OK && lock->operational_parameters_file) {
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
    lock->current_row = NO_CURRENT_ROW;
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
  size_t previous_row = lock->current_row;
  size_t row = 0;
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

/* copy phase_results to general results */
static int sealock_phase_results_to_results(sealock_state_t* lock) {
  // Copy phase_results to equivelent results struct.
  // Also apply the sign convention for DIMR/BMI for discharge.
  lock->results.discharge_from_lake = -lock->phase_results.discharge_from_lake;
  lock->results.discharge_from_sea = -lock->phase_results.discharge_from_sea;
  lock->results.discharge_to_lake = lock->phase_results.discharge_to_lake;
  lock->results.discharge_to_sea = lock->phase_results.discharge_to_sea;
  lock->results.mass_transport_lake = lock->phase_results.mass_transport_lake;
  lock->results.mass_transport_sea = lock->phase_results.mass_transport_sea;
  lock->results.salinity_to_lake = lock->phase_results.salinity_to_lake;
  lock->results.salinity_to_sea = lock->phase_results.salinity_to_sea;
  return SEALOCK_OK;
}

/* Set a correction for the current phase results for possibly skipping a bit of the start. */
static int sealock_apply_phase_wise_result_correction(sealock_state_t *lock, time_t time, time_t duration,
                                                      zsf_results_t *previous) {
  time_t phase_start = lock->times[lock->current_row];
  time_t phase_end = phase_start + duration;
  time_t skipped_time = time - phase_start;
  time_t new_phase_len = duration - skipped_time;

  // Should always be true due to update before calculation.
  assert(time >= phase_start);
  assert(new_phase_len > 0);

  if (skipped_time == 0) {
    // No correction required.
    return SEALOCK_OK;
  }

  if (time <= phase_end) {
    // Calculate (corrected) volume, salt mass and resulting salinity for lake and sea.
    double new_volume_to_lake = lock->results.discharge_to_lake * new_phase_len;
    double missing_volume_to_lake =
        (lock->results.discharge_to_lake - previous->discharge_to_lake) * skipped_time;
    double total_volume_to_lake = new_volume_to_lake + missing_volume_to_lake;

    double new_volume_to_sea = lock->results.discharge_to_sea * new_phase_len;
    double missing_volume_to_sea =
        (lock->results.discharge_to_sea - previous->discharge_to_sea) * skipped_time;
    double total_volume_to_sea = new_volume_to_sea + missing_volume_to_sea;

    double new_salt_to_lake = lock->results.salinity_to_lake * new_volume_to_lake;
    double missing_salt_to_lake =
        (lock->results.salinity_to_lake * lock->results.discharge_to_lake -
         previous->salinity_to_lake * previous->discharge_to_lake) *
        skipped_time;
    double total_salt_to_lake = new_salt_to_lake + missing_salt_to_lake;

    double new_salt_to_sea = lock->results.salinity_to_sea * new_volume_to_sea;
    double missing_salt_to_sea = (lock->results.salinity_to_sea * lock->results.discharge_to_sea -
                                  previous->salinity_to_sea * previous->discharge_to_sea) *
                                 skipped_time;
    double total_salt_to_sea = new_salt_to_sea + missing_salt_to_sea;

    // Only do discharges 'from' lake and sea
    double new_volume_from_lake = lock->results.discharge_from_lake * new_phase_len;
    double missing_volume_from_lake =
        (lock->results.discharge_from_lake - previous->discharge_from_lake) * skipped_time;
    double total_volume_from_lake = new_volume_from_lake + missing_volume_from_lake;

    double new_volume_from_sea = lock->results.discharge_from_sea * new_phase_len;
    double missing_volume_from_sea =
        (lock->results.discharge_from_sea - previous->discharge_from_sea) * skipped_time;
    double total_volume_from_sea = new_volume_from_sea + missing_volume_from_sea;

    // Store corrected results
    lock->results.discharge_to_lake = total_volume_to_lake / new_phase_len;
    lock->results.discharge_to_sea = total_volume_to_sea / new_phase_len;
    lock->results.salinity_to_lake = total_salt_to_lake / total_volume_to_lake;
    lock->results.salinity_to_sea = total_salt_to_sea / total_volume_to_sea;
    lock->results.discharge_from_lake = total_volume_from_lake / new_phase_len;
    lock->results.discharge_from_sea = total_volume_from_sea / new_phase_len;
  } else {
    // We should only get here if we ran out of rows in the timeline.
    assert(time > phase_end);
    assert(lock->current_row == lock->times_len - 1);
    assert(lock->phase_args.time == time);

    // We are beyond the loaded timeline, apply correction for one step beyond ...
    time_t dimr_interval = lock->phase_args.time_step;
    time_t prev_time = time - dimr_interval;
    if (prev_time < phase_end) {
      lock->results.discharge_to_lake *= new_phase_len / dimr_interval;
      lock->results.discharge_to_sea *= new_phase_len / dimr_interval;
      lock->results.discharge_from_lake *= new_phase_len / dimr_interval;
      lock->results.discharge_from_sea *= new_phase_len / dimr_interval;
    } else {
      lock->results.discharge_to_lake = 0;
      lock->results.discharge_to_sea = 0;
      lock->results.salinity_to_lake = 0;
      lock->results.salinity_to_sea = 0;
      lock->results.discharge_from_lake = 0;
      lock->results.discharge_from_sea = 0;
    }
  }

  return SEALOCK_OK;
}

static int sealock_update_phase_wise_parameters(sealock_state_t *lock, time_t time) {
  int status = SEALOCK_OK;
  phase_wise_row_t row_data = PHASE_WISE_CLEAR_ROW();
  if (sealock_update_current_row(lock, time)) {
    status = get_csv_row_data(&lock->timeseries_data, lock->current_row, &row_data);
    if (status == SEALOCK_OK) {
      time_t prev_time = lock->phase_args.time;
      lock->phase_args = PHASE_WISE_CLEAR_ARGS();
      // copy relevant args to lock.
      lock->phase_args.run_update = 1;
      lock->phase_args.routine = row_data.routine;
      lock->phase_args.time = time;
      lock->phase_args.time_step = time - prev_time;
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
  }
  return status;
}

static int sealock_phase_wise_step(sealock_state_t *lock, time_t time) {
  int status = SEALOCK_OK;
  time_t duration = 0;

  if (lock->phase_args.run_update) {
    zsf_results_t previous_step_results = lock->results;
    switch (lock->phase_args.routine) {
    case 1:
      duration = lock->phase_args.t_level;
      status = zsf_step_phase_1(&lock->parameters, lock->phase_args.t_level, &lock->phase_state,
                                &lock->phase_results);
      break;
    case 2:
      duration = lock->phase_args.t_open_lake;
      status = zsf_step_phase_2(&lock->parameters, lock->phase_args.t_open_lake, &lock->phase_state,
                                &lock->phase_results);
      break;
    case 3:
      duration = lock->phase_args.t_level;
      status = zsf_step_phase_3(&lock->parameters, lock->phase_args.t_level, &lock->phase_state,
                                &lock->phase_results);
      break;
    case 4:
      duration = lock->phase_args.t_open_sea;
      status = zsf_step_phase_4(&lock->parameters, lock->phase_args.t_open_sea, &lock->phase_state,
                                &lock->phase_results);
      break;
    default:
      if (lock->phase_args.routine < 0) {
        duration = lock->phase_args.t_flushing;
        status = zsf_step_flush_doors_closed(&lock->parameters, lock->phase_args.t_flushing,
                                             &lock->phase_state, &lock->phase_results);
      } else {
        status = SEALOCK_ERROR;
      }
      break;
    }
    if (status == SEALOCK_OK) {
      status = sealock_phase_results_to_results(lock);
    }
    if (status == SEALOCK_OK) {
      status = sealock_apply_phase_wise_result_correction(lock, time, duration, &previous_step_results);
    }
  }

  lock->phase_args.run_update = 0; // Done with update, so reset flag.
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

static void sealock_get_active_cells(dfm_volumes_t* volumes) {
  // Determine amount of active volumes.
  unsigned amount = 0;
  unsigned first = 0;
  unsigned last = MAX_NUM_VOLUMES-1;
  while (first < MAX_NUM_VOLUMES && volumes->volumes[first] <= 0) {
    first++;
  }
  while (last > 0 && volumes->volumes[last] <= 0) {
    last--;
  }
  if (last >= first) {
    amount = last - first + 1;
  }
  volumes->num_active_cells = amount;
  volumes->first_active_cell = first;
}

// Determine the number and start of active layers.
// Underlying assumption is that the volumes have one
// contiguous run of non-zero cells.
void sealock_get_active_layers(sealock_state_t *lock) {
  sealock_get_active_cells(&lock->lake_volumes);
  sealock_get_active_cells(&lock->sea_volumes);
}

// Collect aggregate of quantity from dfm layer buffer into scalar.
static double sealock_collect(dfm_volumes_t *volumes, double* buffer_ptr) { 
  double aggregate = 0.0;

  assert(volumes);
  assert(buffer_ptr);

  int first = volumes->first_active_cell;
  int last = first + volumes->num_active_cells - 1;

  for (int i = first; i <= last; i++) {
    aggregate += buffer_ptr[i]; // TODO: Check if we need * volumes->volumes[i] here?
  }

  return aggregate;
}

// Collect all scalar inputs that are provided in layers by dfm.
static int sealock_collect_layers(sealock_state_t *lock) {
  dfm_volumes_t *lake_volumes, *sea_volumes;

  assert(lock);

  lake_volumes = &lock->lake_volumes;
  sea_volumes = &lock->sea_volumes;

  lock->parameters.salinity_lake = sealock_collect(lake_volumes, lock->parameters3d.salinity_lake);
  lock->parameters.salinity_sea = sealock_collect(sea_volumes, lock->parameters3d.salinity_sea);

  return SEALOCK_OK;
}

// Distribute scalar quantity over layers into layer buffer.
static int sealock_distribute(dfm_volumes_t *volumes, profile_t *profile, double quantity, double* buffer_ptr) {
  layers_t layers;
  layered_discharge_t result;
  unsigned first_active;

  assert(volumes);
  assert(buffer_ptr);

  // Clear also non-active output cells to zero.
  memset(buffer_ptr, 0, MAX_NUM_VOLUMES * sizeof(double));

  first_active = volumes->first_active_cell;
  layers.number_of_layers = volumes->num_active_cells;
  layers.normalized_target_volumes = volumes->volumes;
  result.number_of_layers = layers.number_of_layers;
  result.discharge_per_layer = &buffer_ptr[first_active];

  return distribute_discharge_over_layers(quantity, profile, &layers, &result);
}

static int sealock_distribute_results(sealock_state_t *lock) {
  profile_t *lake_profile, *sea_profile;
  dfm_volumes_t *lake_volumes, *sea_volumes;

  assert(lock);

  lake_profile = &lock->flow_profile;
  sea_profile = &lock->flow_profile;
  lake_volumes = &lock->lake_volumes;
  sea_volumes = &lock->sea_volumes;

  if (sealock_distribute(lake_volumes, lake_profile, lock->results.mass_transport_lake, lock->results3d.mass_transport_lake) != 0) {
    return SEALOCK_ERROR;
  }
  if (sealock_distribute(lake_volumes, lake_profile, lock->results.salt_load_lake,
                         lock->results3d.salt_load_lake) != 0) {
    return SEALOCK_ERROR;
  }
  if (sealock_distribute(lake_volumes, lake_profile, lock->results.discharge_from_lake,
                             lock->results3d.discharge_from_lake) != 0) {
    return SEALOCK_ERROR;
  }
  if (sealock_distribute(lake_volumes, lake_profile, lock->results.discharge_to_lake,
                         lock->results3d.discharge_to_lake) != 0) {
    return SEALOCK_ERROR;
  }
  if (sealock_distribute(lake_volumes, lake_profile, lock->results.salinity_to_lake,
                         lock->results3d.salinity_to_lake) != 0) {
    return SEALOCK_ERROR;
  }
  if (sealock_distribute(sea_volumes, sea_profile, lock->results.mass_transport_sea,
                         lock->results3d.mass_transport_sea) != 0) {
    return SEALOCK_ERROR;
  }
  if (sealock_distribute(sea_volumes, sea_profile, lock->results.salt_load_sea, lock->results3d.salt_load_sea) != 0) {
    return SEALOCK_ERROR;
  }
  if (sealock_distribute(sea_volumes, sea_profile, lock->results.discharge_from_sea, lock->results3d.discharge_from_sea) != 0) {
    return SEALOCK_ERROR;
  }
  if (sealock_distribute(sea_volumes, sea_profile, lock->results.discharge_to_sea, lock->results3d.discharge_to_sea) != 0) {
    return SEALOCK_ERROR;
  }
  if (sealock_distribute(sea_volumes, sea_profile, lock->results.salinity_to_sea, lock->results3d.salinity_to_sea) != 0) {
    return SEALOCK_ERROR;
  }
  return SEALOCK_OK;
}

int sealock_update(sealock_state_t *lock, time_t time) {
  int status = sealock_set_parameters_for_time(lock, time);
  if (status == SEALOCK_OK) {
    status = sealock_collect_layers(lock);
  }
  if (status == SEALOCK_OK) {
    switch (lock->computation_mode) {
    case cycle_average_mode:
      status = sealock_cycle_average_step(lock, time);
      break;
    case phase_wise_mode:
      status = sealock_phase_wise_step(lock, time);
      break;
    default:
      assert(0);  // Should never happen.
      status = SEALOCK_ERROR;
      break;
    }
  }
  if (status == SEALOCK_OK) {
    status = sealock_distribute_results(lock);
  }
  return status;
}

// Check if none of the time steps in the timeseries is shorter than delta_time
int sealock_delta_time_ok(sealock_state_t* lock, time_t delta_time) {
  for (int i = 0; i < lock->times_len-1; i++) {
    if (lock->times[i + 1] - lock->times[i] < delta_time) {
      return 0;
    }
  }
  return 1;
}