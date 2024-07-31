
#include "zsf_config.h"
#include "csv/load_csv.h"
#include "ini/ini_read.h"
#include "timestamp.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// Parse a comma separated list of doubles in value string.
// Returns number of elements found in the list or 0 on error.
// If array_ptr is not NULL, the array will be filled with converted double values.
// NOTE: The caller is responsible for deallocating the array.
static int parse_double_list(char* value, double** array_ptr) {
  int num_items = 0;
  double *array = NULL;
  char *tempstr = strdup(value);
  char *token;

  token = strtok(tempstr, ",");
  while (token) {
    num_items++;
    token = strtok(NULL, ",");
  }
  free(tempstr);

  if (num_items && array_ptr != NULL) {
    array = malloc(num_items * sizeof(double));
    if (array) {
      token = strtok(value, ",");
      for (int i = 0; i < num_items && token; i++) {
        array[i] = strtod(token, NULL);
        token = strtok(NULL, ",");
      }
    }
    *array_ptr = array;
  }

  return num_items;
}


static int zsf_ini_handler(char *section, char *key, char *value, void *data_ptr) {
  zsf_config_t *config_ptr = (zsf_config_t *)data_ptr;
  char *end_ptr = NULL;

  assert(section);
  assert(key);
  assert(value);
  assert(data_ptr);

  errno = 0;
  if (!strcmp(section, "sealock")) {
    sealock_index_t lock_index = config_ptr->num_locks - 1;
    assert(!*key || lock_index >= 0);
    if (!*key) {
      if (config_ptr->num_locks < ZSF_MAX_LOCKS) {
        config_ptr->num_locks++;
      } else {
        return INI_FAIL;
      }
    } else if (!strcmp(key, "id")) {
      config_ptr->locks[lock_index].id = strdup(value);
    } else if (!strcmp(key, "computation_mode")) {
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
    } else if (!strcmp(key, "flow_profile")) {
      int length = 0;
      double *values = NULL;
      length = parse_double_list(value, &values);
      if (length) {
        config_ptr->locks[lock_index].flow_profile.length = length;
        config_ptr->locks[lock_index].flow_profile.values = values;
      } else {
        return INI_FAIL;
      }
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
  config_ptr->num_locks = 0;
  config_ptr->start_time = 0.0;
  config_ptr->end_time = 0.0;
  config_ptr->current_time = 0.0;
  return ini_read(filepath, zsf_ini_handler, config_ptr);
}

void zsf_config_unload(zsf_config_t *config_ptr) {
  if (config_ptr) {
    while (config_ptr->num_locks) {
      config_ptr->num_locks--;
      unload_csv(&config_ptr->locks[config_ptr->num_locks].timeseries_data);
      free(config_ptr->locks[config_ptr->num_locks].operational_parameters_file);
      free(config_ptr->locks[config_ptr->num_locks].id);
    }
  }
  return;
}

// Find sealock in config by id.
// Returns -1 if not found.
sealock_index_t zsf_config_get_lock_index(const zsf_config_t *config_ptr, const char *lock_id) {
  for (sealock_index_t index = 0; index < config_ptr->num_locks; index++) {
    if (!strcmp(lock_id, config_ptr->locks[index].id)) {
      return index;
    }
  }
  return -1;
}
