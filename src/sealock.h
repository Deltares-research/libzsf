

#ifndef SEALOCK_H
#define SEALOCK_H

#include "csv/load_csv.h"
#include "zsf.h"
#include "io_layer_distribution.h"
#include <time.h>

#define SEALOCK_OK (0)
#define SEALOCK_ERROR (-1)

#if defined(__cplusplus)
extern "C" {
#endif

typedef int sealock_index_t;

typedef enum zsf_computation_mode_enum {
  cycle_average_mode = 0,
  phase_wise_mode
} zsf_computation_mode_t;

typedef struct zsf_phase_wise_args_struct {
  int run_update;
  int routine;
  double t_level;
  double t_open_lake;
  double t_open_sea;
  double t_flushing;
} zsf_phase_wise_args_t;

#define PHASE_WISE_CLEAR_ARGS()                                                                    \
  (zsf_phase_wise_args_t) {                                                                        \
    .run_update = 0, .routine = 0, .t_level = 0, .t_open_lake = 0, .t_open_sea = 0,                \
    .t_flushing = 0                                                                                \
  }

typedef struct sealock_state_struct {
  char *id;
  // ZSF
  zsf_param_t parameters;
  zsf_phase_wise_args_t phase_args;
  zsf_phase_state_t phase_state;
  zsf_phase_transports_t phase_results;
  zsf_results_t results;
  zsf_aux_results_t aux_results;
  zsf_computation_mode_t computation_mode;
  // Cycle average
  char *operational_parameters_file;
  csv_context_t timeseries_data;
  size_t current_row;
  time_t *times;
  size_t times_len;
  profile_t flow_profile;
} sealock_state_t;


int sealock_init(sealock_state_t *lock, time_t start_time);
int sealock_load_timeseries(sealock_state_t *lock, char *filepath);
int sealock_set_parameters_for_time(sealock_state_t *lock, time_t time);
int sealock_update(sealock_state_t *lock, time_t time);
int sealock_delta_time_ok(sealock_state_t *lock, time_t delta_time);

#if defined(__cplusplus)
}
#endif

#endif // SEALOCK_H
