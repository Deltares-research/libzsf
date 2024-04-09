
#include "dimr_bmi.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ZSF_VERBOSE 1
#define ZSF_MAX_LOCKS 50
#define ZSF_KEY_SEPARATOR '/'

#define ZSF_TO_DIMR_STATUS(s) ((s) == 0 ? DIMR_BMI_OK : DIMR_BMI_FAILURE)

typedef unsigned int zsf_lock_index;

typedef enum zsf_operation_mode_e { time_averaged_mode = 0, phase_wise_mode } zsf_operation_mode_t;

typedef struct zsf_lock_state_struct {
  
  zsf_param_t          parameters;
  zsf_phase_state_t    phase_state;
  zsf_results_t        results;
  zsf_aux_results_t    aux_results;
  zsf_operation_mode_t operation_mode;
} zsf_lock_state;

// Keep track of multiple sea-lock instances.
zsf_lock_state locks[ZSF_MAX_LOCKS];
double start_time = 0;
double current_time = 0;
double end_time = 0;

// Exported
int initialize(const char *config_file)
{
  int status = 0;
  zsf_lock_index lock_index = 0;
  double sal_lock = 0.0, head_lock = 0.0;


  zsf_param_default(&locks[lock_index].parameters);
  // TODO: Implement reading supplied config file(s).

  locks[lock_index].operation_mode = time_averaged_mode;

#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\" ) called.\n", __func__, config_file);
#endif

  status |= zsf_initialize_state(&locks[lock_index].parameters, &locks[lock_index].phase_state, sal_lock, head_lock);

  return ZSF_TO_DIMR_STATUS(status);
}

// Exported
int finalize()
{
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

  // TODO: implement properly?
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

inline int match_key(char *key, char *defined_key) {
  size_t defined_key_length = strlen(defined_key);
  return !strncmp(key, defined_key, defined_key_length) &&
         (!key[defined_key_length] || key[defined_key_length] == ZSF_KEY_SEPARATOR);
}

// Exported
// In BMI 2.0 = set_value
int set_var(const char *key, void *src_ptr) {
  zsf_lock_index lock_index = 0;
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

  // lock_index = get_lock_index(lock_id);
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
int get_var(const char *key, void **dst_ptr)
{
#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\", %p ) called.\n", __func__, key, dst_ptr);
#endif

  zsf_lock_index lock_index = 0;
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
int get_value_ptr(char *key, void **dst_ptr) 
{
#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\", %p ) called.\n", __func__, key, dst_ptr);
#endif

  return DIMR_BMI_OK;
}

// Exported
int update(double dt)
{ 
  int status = 0;
  zsf_lock_index lock_index = 0;

#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, dt);
#endif

  current_time += dt;

  switch (locks[lock_index].operation_mode)
  {
    case time_averaged_mode:
      // TODO: Update parameters from CSV data?
      status = zsf_calc_steady(&locks[lock_index].parameters, &locks[lock_index].results, NULL);
      break;
    case phase_wise_mode:
      // TODO: implement me
      break;
    default:
      status = -1; // Should never happen.
      break;
  }

  return ZSF_TO_DIMR_STATUS(status);
}

int get_var_shape(char *key, int *dims) // dims -> int[6]
{
#if ZSF_VERBOSE
  printf("ZSF: %s( \"%s\", %d ) called.\n", __func__, key, *dims);
#endif
  // TODO: Implement me?
  return DIMR_BMI_OK;
}

/* Not needed? (also mostly not BMI standard) */
int  update_until(double update_time)
{
  return DIMR_BMI_OK;
}

void get_version_string(char **version_string)
{
  if (version_string != NULL) {
    *version_string = ZSF_GIT_DESCRIBE;
  }
}

void get_attribute(char *name, char *value)
{
  // TODO: Implement me?
}

// Exported
void get_start_time(double *start_time_ptr)
{
#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, *start_time_ptr);
#endif
  // TODO: Implement me
}

// Exported
void get_end_time(double *end_time_ptr)
{
#if ZSF_VERBOSE
    printf("ZSF: %s( %g ) called.\n", __func__, *end_time_ptr);
#endif
    // TODO: Implement me
}

// Exported
void get_time_step(double *time_step_ptr)
{
#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, *time_step_ptr);
#endif
  // TODO: Implement me
}

// Exported
void get_current_time(double *current_time_ptr)
{
#if ZSF_VERBOSE
  printf("ZSF: %s( %g ) called.\n", __func__, *current_time_ptr);
#endif
  // TODO: Implement me
}


// Points to a Log object (see log.h in DIMR)
void set_dimr_logger(void *logptr)
{
  // TODO: Implement me?
}

// Takes a function pointer (not sure what it is)
void set_logger_c_callback(void (*callback)(char *msg))
{
  // TODO: Implement me?
}
