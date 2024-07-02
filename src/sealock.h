

#ifndef SEALOCK_H
#define SEALOCK_H

#include "csv/load_csv.h"
#include "zsf.h"
#include <time.h>

#define SEALOCK_OK (0)
#define SEALOCK_ERROR (-1)

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned int sealock_index_t;

typedef enum zsf_computation_mode_enum {
  cycle_average_mode = 0,
  phase_wise_mode
} zsf_computation_mode_t;

typedef struct sealock_state_struct {
  zsf_param_t parameters;
  zsf_phase_state_t phase_state;
  zsf_results_t results;
  zsf_aux_results_t aux_results;
  zsf_computation_mode_t computation_mode;
  char *operational_parameters_file;
  csv_context_t timeseries_data;
  size_t current_row;
  time_t *times;
  size_t times_len;
} sealock_state_t;

int sealock_load_timeseries(sealock_state_t *lock, char *filepath);
int sealock_set_parameters_for_time(sealock_state_t *lock, time_t time);
int sealock_update(sealock_state_t *lock, time_t time);

#if defined(__cplusplus)
}
#endif

#endif // SEALOCK_H
