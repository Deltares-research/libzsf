
#include "dimr_bmi.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "sealock.h"

#define ZSF_VERBOSE 1
#define ZSF_MAX_LOCKS 50
#define ZSF_KEY_SEPARATOR '/'

#define ZSF_TO_DIMR_STATUS(s) ((s) == 0 ? DIMR_BMI_OK : DIMR_BMI_FAILURE)


// Keep track of multiple sea-lock instances.
sealock_state_t locks[ZSF_MAX_LOCKS];
double start_time = 0;
double current_time = 0;
double end_time = 0;

// Exported
int initialize(const char *config_file) {
  // TODO: Read these items from .ini file.
  // (MOCKUP) Start ini config items.
  zsf_computation_mode_t computation_mode = cycle_average_mode;
  char *sealock_operational_parameters = "sealock_A.csv";
  double initial_head_lock = 0.0;
  double initial_salinity_lock = 15.0;
  double initial_saltmass_lock = 12344.0;
  double initial_temperature_lock = 10.1; // Where do we map this?
  double initial_volume_ship_in_lock = 0.0;
  double lock_length = 300.0;
  double lock_width = 25.0;
  double lock_bottom = -7.0;
  // (MOCKUP) End ini config items.

  int status = 0;
  sealock_index_t lock_index = 0; // Fixed for now.
  size_t num_rows = 0;

#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\" ) called.\n", __func__, config_file);
#endif

  // init calculation parameters with defaults.
  zsf_param_default(&locks[lock_index].parameters);

  // Set up user provided initial lock parameters.
  locks[lock_index].computation_mode = cycle_average_mode;
  locks[lock_index].parameters.lock_length = lock_length;
  locks[lock_index].parameters.lock_width = lock_width;
  locks[lock_index].parameters.lock_bottom = lock_bottom;
  locks[lock_index].phase_state.saltmass_lock = initial_saltmass_lock;
  locks[lock_index].phase_state.volume_ship_in_lock = initial_volume_ship_in_lock;
  locks[lock_index].phase_state.salinity_lock = initial_salinity_lock;
  locks[lock_index].phase_state.head_lock = initial_head_lock;
  // TODO: Map temperature somewhere?

  status = sealock_load_data(&locks[lock_index], sealock_operational_parameters);
  status = sealock_update(&locks[lock_index], current_time);

  // Initialize parameters consistent with current and given settings.
  status |= zsf_initialize_state(&locks[lock_index].parameters, &locks[lock_index].phase_state,
                                 initial_salinity_lock, initial_head_lock);

  return ZSF_TO_DIMR_STATUS(status);
}

// Exported
int finalize() {
#if ZSF_VERBOSE
  printf("ZSF: %s() called.\n", __func__);
#endif
  // TODO: Implement me
  return DIMR_BMI_OK; // Should always return DIMR_BMI_OK
}

// parse key of format '<vartype>/<lock_id>/<quantity>'.
inline int parse_key(char *key, char **vartype_ptr, char **lock_id_ptr, char **quantity_ptr) {
  char *ptr = key;
  assert(vartype_ptr != NULL);
  assert(lock_id_ptr != NULL);
  assert(quantity_ptr != NULL);

  if (key == NULL || *key == '\0') {
    return 0;
  }

  // We assume a bmi request key to be of one of the following forms:
  // 1. "Quantity"
  // 2. "LockID/Quantity"
  // 3. "VarType/LockID/Quantity"
  // 4. "**/VarType/LockId/Quantity" This ignores all parts before.
  // There is no checking done with regards to any configured settings, so
  // we don't make any attempt to recognize what each part is.
  *vartype_ptr = NULL;
  *lock_id_ptr = NULL;
  *quantity_ptr = ptr;
  while (*ptr) {
    if (*ptr == ZSF_KEY_SEPARATOR) {
      *vartype_ptr = *lock_id_ptr;
      *lock_id_ptr = *quantity_ptr;
      *quantity_ptr = ptr+1;
    }
    ptr++;
  }
  return 1;
}

// Checks if a previously retrieved (sub) key matches a defined key.
inline int match_key(char *key, char *defined_key) {
  size_t defined_key_length = strlen(defined_key);
  return !strncmp(key, defined_key, defined_key_length) &&
         (!key[defined_key_length] || key[defined_key_length] == ZSF_KEY_SEPARATOR);
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

  // lock_index = get_lock_index(lock_id); // The lock_id is currently ignored and kept at 0.
  if (lock_index < 0) {
    return DIMR_BMI_FAILURE;
  }

#if ZSF_VERBOSE
  printf("ZSF: %s: lock_index = %d, quantity = %s\n", __func__, lock_index, quantity);
#endif

  // Set dest_ptr for intended lock and quantity.
  if (match_key(quantity, "salinity_lake")) {
    dest_ptr = &locks[lock_index].parameters.salinity_lake;
  } else if (match_key(quantity, "head_lake")) {
    dest_ptr = &locks[lock_index].parameters.head_lake;
  } else if (match_key(quantity, "salinity_sea")) {
    dest_ptr = &locks[lock_index].parameters.salinity_sea;
  } else if (match_key(quantity, "head_sea")) {
    dest_ptr = &locks[lock_index].parameters.head_sea;
  }

  if (src_ptr == NULL || dest_ptr == NULL) {
    return DIMR_BMI_FAILURE;
  }

#if ZSF_VERBOSE
  printf("ZSF: %s set value for %s to %g at %p.\n", __func__, quantity, *(double *)src_ptr, dest_ptr);
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

  // lock_index = get_lock_index(lock_id);
  if (lock_index < 0) {
    return DIMR_BMI_FAILURE;
  }

  // Set source based on key(s)...
  if (match_key(quantity, "mass_transport_lake")) {
    source_ptr = &locks[lock_index].results.mass_transport_lake;
  } else if (match_key(quantity, "salt_load_lake")) {
    source_ptr = &locks[lock_index].results.salt_load_lake;
  } else if (match_key(quantity, "discharge_from_lake")) {
    source_ptr = &locks[lock_index].results.discharge_from_lake;
  } else if (match_key(quantity, "discharge_to_lake")) {
    source_ptr = &locks[lock_index].results.discharge_to_lake;
  } else if (match_key(quantity, "salinity_to_lake")) {
    source_ptr = &locks[lock_index].results.salinity_to_lake;
  } else if (match_key(quantity, "mass_transport_sea")) {
    source_ptr = &locks[lock_index].results.mass_transport_sea;
  } else if (match_key(quantity, "salt_load_sea")) {
    source_ptr = &locks[lock_index].results.salt_load_sea;
  } else if (match_key(quantity, "discharge_from_sea")) {
    source_ptr = &locks[lock_index].results.discharge_from_sea;
  } else if (match_key(quantity, "discharge_to_sea")) {
    source_ptr = &locks[lock_index].results.discharge_to_sea;
  } else if (match_key(quantity, "salinity_to_sea")) {
    source_ptr = &locks[lock_index].results.salinity_to_sea;
  }

  if (dst_ptr == NULL || source_ptr == NULL) {
    return DIMR_BMI_FAILURE;
  }

  *(double **)dst_ptr = source_ptr;
#if ZSF_VERBOSE
  printf("ZSF: %s yielded the value %g for quantity '%s' of lock %d.\n", __func__, *source_ptr, quantity,
         lock_index);
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
int update(double dt) { 
  int status = 0;
  sealock_index_t lock_index = 0;

#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, dt);
#endif
  current_time += dt;

  // TODO: Add loop over locks.
  status = sealock_update(&locks[lock_index], current_time);

  return ZSF_TO_DIMR_STATUS(status);
}

int get_var_shape(char *key, int *dims) { // dims -> int[6]
#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\", %d ) called.\n", __func__, key, *dims);
#endif
  // TODO: Implement me?
  return DIMR_BMI_OK;
}

/* Not needed? (also mostly not BMI standard) */
int  update_until(double update_time) {
  return DIMR_BMI_OK;
}

void get_version_string(char **version_string)
{
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
  // TODO: Implement me
}

// Exported
void get_end_time(double *end_time_ptr) {
#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, *end_time_ptr);
#endif
  // TODO: Implement me
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
  *current_time_ptr = current_time;
}

// Points to a Log object (see log.h in DIMR)
void set_dimr_logger(void *logptr) {
  // TODO: Implement me?
}

// Takes a function pointer (not sure what it is)
void set_logger_c_callback(void (*callback)(char *msg)) {
  // TODO: Implement me?
}
