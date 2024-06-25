
#include "zsf_config.h"
#include "csv/load_csv.h"
#include "ini/ini_read.h"
#include "timestamp.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int _zsf_ini_handler(char *section, char *key, char *value, void *data_ptr) {
  zsf_config_t *config_ptr = (zsf_config_t *)data_ptr;
  sealock_index_t lock_index = 0;
  char *end_ptr = NULL;

  assert(section);
  assert(key);
  assert(value);
  assert(data_ptr);

  errno = 0;
  if (!strcmp(section, "sealock")) {
    config_ptr->num_locks = 1;
    if (!strcmp(key, "computation_mode")) {
      if (!strcmp(value, "cycle_average")) {
        config_ptr->locks[lock_index].computation_mode = cycle_average_mode;
      } else if (!strcmp(value, "phase_wise")) {
        config_ptr->locks[lock_index].computation_mode = phase_wise_mode;
      } else {
        return INI_FAIL;
      }
    } else if (!strcmp(key, "sealock_operational_parameters")) {
      config_ptr->locks[lock_index].operational_parameters_file = strdup(value);
    } else if (!strcmp(key, "initial_head_lock")) {
      config_ptr->locks[lock_index].phase_state.head_lock = strtod(value, &end_ptr);
    } else if (!strcmp(key, "initial_salinity_lock")) {
      config_ptr->locks[lock_index].phase_state.salinity_lock = strtod(value, &end_ptr);
    } else if (!strcmp(key, "initial_saltmass_lock")) {
      config_ptr->locks[lock_index].phase_state.saltmass_lock = strtod(value, &end_ptr);
    } else if (!strcmp(key, "initial_temperature_lock")) {
      // TODO: Add temperature to lock?
    } else if (!strcmp(key, "initial_volume_ship_in_lock")) {
      config_ptr->locks[lock_index].phase_state.volume_ship_in_lock = strtod(value, &end_ptr);
    } else if (!strcmp(key, "lock_length")) {
      config_ptr->locks[lock_index].parameters.lock_length = strtod(value, &end_ptr);
    } else if (!strcmp(key, "lock_width")) {
      config_ptr->locks[lock_index].parameters.lock_width = strtod(value, &end_ptr);
    } else if (!strcmp(key, "lock_bottom")) {
      config_ptr->locks[lock_index].parameters.lock_bottom = strtod(value, &end_ptr);
    }
  } else if (!strcmp(section, "general") || !*section) {
    if (!strcmp(key, "start_time")) {
      config_ptr->start_time = timestamp_string_to_time(value, &end_ptr);
      config_ptr->current_time = config_ptr->start_time;
    } else if (!strcmp(key, "end_time")) {
      config_ptr->end_time = timestamp_string_to_time(value, &end_ptr);
    }
  }

  if (errno || end_ptr == value || (end_ptr && *end_ptr)) {
    return INI_FAIL;
  }

  return INI_OK;
}

int zsf_config_load(zsf_config_t *config_ptr, const char *filepath) {
  assert(config_ptr);
  assert(filepath);
  return ini_read(filepath, _zsf_ini_handler, config_ptr);
}

void zsf_config_unload(zsf_config_t *config_ptr) {
  if (config_ptr) {
    while (config_ptr->num_locks) {
      config_ptr->num_locks--;
      unload_csv(&config_ptr->locks[config_ptr->num_locks].timeseries_data);
      free(config_ptr->locks[config_ptr->num_locks].operational_parameters_file);
    }
  }
  return;
}
