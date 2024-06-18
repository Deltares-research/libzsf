
#include "dimr_bmi.h"
#include "config.h"
#include "timestamp.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "zsf_config.h"

#define ZSF_VERBOSE 1
const char *zsf_key_separator = "/";
static inline int zsf_to_dimr_status(int s) { return ((s) == 0 ? DIMR_BMI_OK : DIMR_BMI_FAILURE); }

// Global conbfiguration.
zsf_config_t config;

// Exported
int initialize(const char *config_file) {
  int status = 0;
  sealock_index_t lock_index = 0; // Fixed to 0 for now.

#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\" ) called.\n", __func__, config_file);
#endif

  // Init calculation parameters with defaults.
  zsf_param_default(&config.locks[lock_index].parameters);

  // Read ini file.
  status = zsf_config_load(&config, config_file);
  if (status)
    return zsf_to_dimr_status(status);

  // Load timeseries data.
  status = sealock_load_timeseries(&config.locks[lock_index],
                             config.locks[lock_index].operational_parameters_file);
  if (status)
    return zsf_to_dimr_status(status);

  // Do one update to properly populate all parameters from timeseries for current time.
  status = sealock_set_parameters_for_time(&config.locks[lock_index], config.current_time);
  if (status)
    return zsf_to_dimr_status(status);

  // Initialize parameters consistent with current and given settings.
  status = zsf_initialize_state(&config.locks[lock_index].parameters,
                                &config.locks[lock_index].phase_state,
                                config.locks[lock_index].phase_state.salinity_lock,
                                config.locks[lock_index].phase_state.head_lock);

  return zsf_to_dimr_status(status);
}

// Exported
int finalize() {
#if ZSF_VERBOSE
  printf("ZSF: %s() called.\n", __func__);
#endif
  zsf_config_unload(&config);
  return DIMR_BMI_OK; // Should always return DIMR_BMI_OK
}

// Parse bmi key into type, lock_id and quantity.
// We assume a bmi request key to be of one of the following forms:
// 1. "Quantity"
// 2. "LockID/Quantity"
// 3. "VarType/LockID/Quantity"
// 4. "**/VarType/LockId/Quantity" This ignores all parts before.
// There is no checking done with regards to any configured settings, so
// we don't make any attempt to recognize what each part is.
// Note: This function DOES change the content of the supplied key string.
inline int parse_key(char *key, char **vartype_ptr, char **lock_id_ptr, char **quantity_ptr) {
  char *save_ptr;
  char *token;

  assert(vartype_ptr != NULL);
  assert(lock_id_ptr != NULL);
  assert(quantity_ptr != NULL);

  *vartype_ptr = NULL;
  *lock_id_ptr = NULL;
  *quantity_ptr = NULL;

  if (key == NULL || *key == '\0') {
    return 0; // Fail on empty strings.
  }

  token = strtok(key, zsf_key_separator, &save_ptr);  
  while (token) {
    *vartype_ptr = *lock_id_ptr;
    *lock_id_ptr = *quantity_ptr;
    *quantity_ptr = token;
    token = strtok(NULL, zsf_key_separator, &save_ptr);
  }
  return 1;
}

// Checks if a previously retrieved (sub) key matches a defined key.
inline int match_key(char *key, char *defined_key) {
  return !strcmp(key, defined_key);
}

// Exported
// In BMI 2.0 = set_value
int set_var(const char *key, void *src_ptr) {
  sealock_index_t lock_index = 0;
  double *dest_ptr = NULL;
  char *quantity = NULL;
  char *vartype = NULL;
  char *lock_id = NULL;

#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\", *src_ptr = %g) called.\n", __func__, key, *(double *)src_ptr);
#endif

  if (!parse_key(key, &vartype, &lock_id, &quantity)) {
    return DIMR_BMI_FAILURE;
  }

  if (lock_id) {
    lock_index = zsf_config_get_lock_index(&config, lock_id);
    if (lock_index < 0) {
      return DIMR_BMI_FAILURE;
    }
  }

#if ZSF_VERBOSE
  printf("ZSF: %s: lock_index = %d, quantity = %s\n", __func__, lock_index, quantity);
#endif

  // Set dest_ptr for intended lock and quantity.
  if (match_key(quantity, "salinity_lake")) {
    dest_ptr = &config.locks[lock_index].parameters.salinity_lake;
  } else if (match_key(quantity, "head_lake")) {
    dest_ptr = &config.locks[lock_index].parameters.head_lake;
  } else if (match_key(quantity, "salinity_sea")) {
    dest_ptr = &config.locks[lock_index].parameters.salinity_sea;
  } else if (match_key(quantity, "head_sea")) {
    dest_ptr = &config.locks[lock_index].parameters.head_sea;
  }

  if (src_ptr == NULL || dest_ptr == NULL) {
    return DIMR_BMI_FAILURE;
  }

#if ZSF_VERBOSE
  printf("ZSF: %s set value for %s to %g at %p.\n", __func__, quantity, *(double *)src_ptr,
         dest_ptr);
#endif
  *dest_ptr = *(double *)src_ptr;
  return DIMR_BMI_OK;
}

// Exported
// In BMI 2.0 = get_value
int get_var(const char *key, void **dst_ptr) {
#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\", %p ) called.\n", __func__, key, dst_ptr);
#endif

  sealock_index_t lock_index = 0;
  double *source_ptr = NULL;
  char *quantity = NULL;
  char *vartype = NULL;
  char *lock_id = NULL;

  if (!parse_key(key, &vartype, &lock_id, &quantity)) {
    return DIMR_BMI_FAILURE;
  }

  if (lock_id) {
    lock_index = zsf_config_get_lock_index(&config, lock_id);
    if (lock_index < 0) {
      return DIMR_BMI_FAILURE;
    }
  }

  // Set source based on key(s)...
  if (match_key(quantity, "mass_transport_lake")) {
    source_ptr = &config.locks[lock_index].results.mass_transport_lake;
  } else if (match_key(quantity, "salt_load_lake")) {
    source_ptr = &config.locks[lock_index].results.salt_load_lake;
  } else if (match_key(quantity, "discharge_from_lake")) {
    source_ptr = &config.locks[lock_index].results.discharge_from_lake;
  } else if (match_key(quantity, "discharge_to_lake")) {
    source_ptr = &config.locks[lock_index].results.discharge_to_lake;
  } else if (match_key(quantity, "salinity_to_lake")) {
    source_ptr = &config.locks[lock_index].results.salinity_to_lake;
  } else if (match_key(quantity, "mass_transport_sea")) {
    source_ptr = &config.locks[lock_index].results.mass_transport_sea;
  } else if (match_key(quantity, "salt_load_sea")) {
    source_ptr = &config.locks[lock_index].results.salt_load_sea;
  } else if (match_key(quantity, "discharge_from_sea")) {
    source_ptr = &config.locks[lock_index].results.discharge_from_sea;
  } else if (match_key(quantity, "discharge_to_sea")) {
    source_ptr = &config.locks[lock_index].results.discharge_to_sea;
  } else if (match_key(quantity, "salinity_to_sea")) {
    source_ptr = &config.locks[lock_index].results.salinity_to_sea;
  }

  if (dst_ptr == NULL || source_ptr == NULL) {
    return DIMR_BMI_FAILURE;
  }

  *(double **)dst_ptr = source_ptr;
#if ZSF_VERBOSE
  printf("ZSF: %s yielded the value %g for quantity '%s' of lock %d.\n", __func__, *source_ptr,
         quantity, lock_index);
#endif
  return DIMR_BMI_OK;
}

// In DIMR **dst_ptr always is a double.
int get_value_ptr(char *key, void **dst_ptr) {
#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\", %p ) called.\n", __func__, key, dst_ptr);
#endif

  return DIMR_BMI_OK;
}

// Exported
// Update ZSF state.
// Advances the current time by dt seconds.
int update(double dt) {
  int status = DIMR_BMI_OK;
  sealock_index_t lock_index = 0;

#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, dt);
#endif
  config.current_time += (time_t)dt;

  for (sealock_index_t lock_index = 0; lock_index < config.num_locks; lock_index++) {
    if (sealock_update(&config.locks[lock_index], config.current_time)) {
      status = DIMR_BMI_FAILURE;
    }
  }

  return status;
}

int get_var_shape(char *key, int *dims) { // dims -> int[6]
#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\", %d ) called.\n", __func__, key, *dims);
#endif
  // TODO: Implement me?
  return DIMR_BMI_OK;
}

/* Not needed? (also mostly not BMI standard) */
int update_until(double update_time) { return DIMR_BMI_OK; }

void get_version_string(char **version_string) {
  if (version_string != NULL) {
    *version_string = ZSF_GIT_DESCRIBE;
  }
}

void get_attribute(char *name, char *value) {
  // TODO: Implement me?
}

// Exported
void get_start_time(double *start_time_ptr) {
#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, *start_time_ptr);
#endif
  *start_time_ptr = time_to_timestamp(config.start_time);
}

// Exported
void get_end_time(double *end_time_ptr) {
#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, *end_time_ptr);
#endif
  *end_time_ptr = time_to_timestamp(config.end_time);
}

// Exported
void get_time_step(double *time_step_ptr) {
#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, *time_step_ptr);
#endif
  // TODO: Implement me
}

// Exported
void get_current_time(double *current_time_ptr) {
#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, *current_time_ptr);
#endif
  *current_time_ptr = time_to_timestamp(config.current_time);
}

