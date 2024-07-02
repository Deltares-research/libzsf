#ifndef IO_LAYER_DISTRIBUTION_H
#define IO_LAYER_DISTRIBUTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

static const struct profile_T {
  int number_of_positions; // Number of elements in each following array
  double *
      relative_z_position; // Relative positions in the z-direction for which profile is provided (0 = bed level, 1 = water level)
  double *
      relative_discharge_from_lock; // Relative discharge at corresponding relative z-position (non-negative), need not be normalized
} profile_default = {
    .number_of_positions = 0, .relative_z_position = NULL, .relative_discharge_from_lock = NULL};

typedef struct profile_T profile;

void cleanup_profile(profile *profile);

static const struct layers_T {
  int number_of_layers;
  double *
      normalized_target_volumes; // Percentage of water volume per layer, discharge will be transferred proportionally
} layers_default = {.number_of_layers = 0, .normalized_target_volumes = NULL};

typedef struct layers_T layers;

void cleanup_layers(layers *layers);

static const struct layered_discharge_T {
  int number_of_layers;
  double *discharge_per_layer;
} layered_discharge_default = {.number_of_layers = 0, .discharge_per_layer = NULL};

typedef struct layered_discharge_T layered_discharge;

void cleanup_layered_discharge(layered_discharge *layered_discharge);

double integrate_piecewise_linear_profile(const profile *profile, const double lower_bound,
                                          const double upper_bound);

int distribute_discharge_over_layers(double total_discharge, const profile *profile,
                                     const layers *layers,
                                     layered_discharge *layered_discharge_result);
#ifdef __cplusplus
}
#endif
#endif // IO_LAYER_DISTRIBUTION_H
