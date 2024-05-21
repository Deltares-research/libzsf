

#ifndef _SEALOCK_H_
#  define _SEALOCK_H_

#include "zsf.h"
#include "csv.h"

#define SEALOCK_OK (0)
#define SEALOCK_ERROR (-1)

#  if defined(__cplusplus)
extern "C" {
#  endif

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
  csv_context_t timeseries_data;
  size_t current_row;
  double *times;
  size_t times_len;
} sealock_state_t;


int sealock_load_data(sealock_state_t *lock, char *filepath);
int sealock_update(sealock_state_t *lock, double time);

#  if defined(__cplusplus)
}
#  endif

#endif // _SEALOCK_H_
