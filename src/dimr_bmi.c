
#include "dimr_bmi.h"
#include "config.h"
#include <stdio.h>

#define ZSF_TO_DIMR_STATUS(s) ((s) == 0 ? DIMR_BMI_OK : DIMR_BMI_FAILURE)

// TODO: Keep track of multiple sea-lock instances.
zsf_param_t _parameters;
zsf_phase_state_t _state;
zsf_phase_transports_t _results;
zsf_aux_results_t _aux_results;

int initialize(const char *config_file)
{
  int status = 0;
  double sal_lock = 0.0, head_lock = 0.0;
  zsf_param_default(&_parameters);
  // TODO: Implement reading supplied config file.

  status = zsf_initialize_state(&_parameters, &_state, sal_lock, head_lock);

  return ZSF_TO_DIMR_STATUS(status);
}

int finalize()
{
  // TODO: Implement me
  return DIMR_BMI_OK; // Should always return DIMR_BMI_OK
}

// In BMI 2.0 = set_value
int set_var(const char *key, void *src_ptr)
{
  // TODO: Implement me
  return DIMR_BMI_OK;
}

// In BMI 2.0 = get_value
int get_var(const char *key, void *dst_ptr)
{
  // TODO: Implement me
  if (dst_ptr != NULL) {
    //*(double *)dst_ptr = 5.0;
  }
  return DIMR_BMI_OK;
}

// In DIMR **dst_ptr always is a double.
int get_value_ptr(char *key, void **dst_ptr) 
{ 
  return DIMR_BMI_OK; 
}

int update(double dt)
{ 
  int status = zsf_calc_steady(&_parameters, &_results, NULL);
  return ZSF_TO_DIMR_STATUS(status);
}

int get_var_shape(char *key, int *dims) // dims -> int[6]
{
  // TODO: Implement me
  return DIMR_BMI_OK;
}

/* Not needed? (also mostly not BMI standard) */
int  update_until(double update_time)
{
  return DIMR_BMI_OK;
}

void get_version_string(char *version_string)
{
  if (version_string != NULL) {
    *version_string = ZSF_GIT_DESCRIBE;
  }
}

void get_attribute(char *name, char *value)
{
  // TODO: Implement me
}

void get_start_time(double *start_time_ptr)
{
  // TODO: Implement me
}

void get_end_time(double *end_time_ptr)
{
  // TODO: Implement me
}

void get_time_step(double *time_step_ptr)
{
  // TODO: Implement me
}

void get_current_time(double *current_time_ptr)
{
  // TODO: Implement me
}

// Points to a Log object (see log.h in DIMR)
void set_dimr_logger(void *logptr)
{
  // TODO: Implement me
}

// Takes a function pointer (not sure what it is)
void set_logger_c_callback(void (*callback)(char *msg))
{
  // TODO: Implement me
}
