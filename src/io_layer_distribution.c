#include "io_layer_distribution.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

void cleanup_profile(profile_t *profile) {
  if (profile == NULL) {
    return;
  }
  free(profile->relative_z_position);
  free(profile->relative_discharge_from_lock);
}

void cleanup_layers(layers_t *layers) {
  if (layers == NULL) {
    return;
  }
  free(layers->normalized_target_volumes);
}

void cleanup_layered_discharge(layered_discharge_t *layered_discharge) {
  if (layered_discharge == NULL) {
    return;
  }
  free(layered_discharge->discharge_per_layer);
}

// Generate a default array linear z positions for supplied number_of_layers.
// Returns pointer to allocated array of doubles.
// Note: Caller is responsible to deallocating the created array.
double* io_layer_linear_z_positions(const int number_of_layers) {
  double *linear_z_positions = NULL;
  if (number_of_layers > 0) {
    linear_z_positions = calloc(number_of_layers, sizeof(double));
    if (linear_z_positions) {
      double z_step = 1.0 / (number_of_layers - 1.0);
      for (int i = 0; i < number_of_layers; i++) {
        linear_z_positions[i] = z_step * i;
      }
    }
  }
  return linear_z_positions;
}

// Set up a default '2D' profile.
int io_layer_init_2d(profile_t *profile) {
  int array_length = 0;
  double *value_array = calloc(2, sizeof(double));
  double *linear_z_positions = io_layer_linear_z_positions(2);
  if (value_array && linear_z_positions) {
    value_array[0] = 1.0;
    value_array[1] = 1.0;
    *profile = (profile_t){.number_of_positions = 2,
                .relative_discharge_from_lock = value_array,
                .relative_z_position = linear_z_positions};
  } else {
    free(value_array);
    free(linear_z_positions);
    return -1;
  }
  return 0;
}


// Integrate relative_discharge_from_lock(x) with x from 0 to 1, assuming that relative_z_position[0] == 0 and relative_z_position[number_of_positions - 1] == 1.
// Further, the profile is assumed to be piecewise linear, and is a linear interpolation between the given relative_z_positions.
double integrate_piecewise_linear_profile(const profile_t *profile, const double lower_bound,
                                          const double upper_bound) {
  assert(0.0 <= lower_bound && lower_bound <= upper_bound && upper_bound <= 1.0);
  assert(profile->number_of_positions >= 2); // 0 and 1 have to be in

  int index_below_lower_bound = 0;
  while (index_below_lower_bound + 1 < profile->number_of_positions &&
         profile->relative_z_position[index_below_lower_bound + 1] < lower_bound) {
    ++index_below_lower_bound;
  }
  int index_above_upper_bound = profile->number_of_positions - 1;
  while (index_above_upper_bound - 1 >= 0 &&
         profile->relative_z_position[index_above_upper_bound - 1] > upper_bound) {
    --index_above_upper_bound;
  }
  assert(index_below_lower_bound <= index_above_upper_bound);
  double integrated_profile = 0.0;

  // Integrate from first profile point below lower bound to first profile point above upper bound (overestimates the integral)
  for (int i = index_below_lower_bound; i < index_above_upper_bound; ++i) {
    const double z_below = profile->relative_z_position[i];
    const double z_above = profile->relative_z_position[i + 1];
    const double p_below = profile->relative_discharge_from_lock[i];
    const double p_above = profile->relative_discharge_from_lock[i + 1];
    integrated_profile += 0.5 * (p_above + p_below) * (z_above - z_below);
  }

  const double z_lower_below = profile->relative_z_position[index_below_lower_bound];
  const double z_lower_above = profile->relative_z_position[index_below_lower_bound + 1];
  const double p_lower_below = profile->relative_discharge_from_lock[index_below_lower_bound];
  const double p_lower_above = profile->relative_discharge_from_lock[index_below_lower_bound + 1];
  // Subtract integral over profile from first profile point below lower bound to lower bound
  integrated_profile -= p_lower_below * (lower_bound - z_lower_below) +
                        0.5 * (p_lower_above - p_lower_below) *
                            pow(lower_bound - z_lower_below, 2.0) / (z_lower_above - z_lower_below);

  const double z_upper_below = profile->relative_z_position[index_above_upper_bound - 1];
  const double z_upper_above = profile->relative_z_position[index_above_upper_bound];
  const double p_upper_below = profile->relative_discharge_from_lock[index_above_upper_bound - 1];
  const double p_upper_above = profile->relative_discharge_from_lock[index_above_upper_bound];
  // Subtract integral over profile from upper bound to first profile point above upper bound
  integrated_profile -= p_upper_above * (z_upper_above - upper_bound) +
                        0.5 * (p_upper_below - p_upper_above) *
                            pow(z_upper_above - upper_bound, 2.0) / (z_upper_above - z_upper_below);
  return integrated_profile;
}

// Distribute the total_discharge over layers.
// The discharge that each layer receives is given by a relative profile
int distribute_discharge_over_layers(double total_discharge, const profile_t *profile,
                                     const layers_t *layers,
                                     layered_discharge_t *layered_discharge_result) {
  assert(layers != NULL);
  assert(layers->number_of_layers > 0);
  assert(layered_discharge_result != NULL);
  assert(layered_discharge_result->number_of_layers == layers->number_of_layers);
  assert(layered_discharge_result->discharge_per_layer != NULL);
  assert(profile->relative_z_position != NULL);
  assert(profile->relative_discharge_from_lock != NULL);
  assert(profile->number_of_positions > 0);

  double total_relative_discharge = 0.0;
  if (profile->number_of_positions <= 0 || abs(profile->relative_z_position[0]) > FLT_EPSILON ||
      abs(profile->relative_z_position[profile->number_of_positions - 1] - 1.0) > FLT_EPSILON) {
    return -1;
  }

  const double relative_discharge_total = integrate_piecewise_linear_profile(profile, 0.0, 1.0);
  double previous_volume = 0.0;
  double next_volume = 0.0;
  for (int layer = 0; layer < layers->number_of_layers; ++layer) {
    next_volume += layers->normalized_target_volumes[layer];
    const double relative_discharge_layer =
        integrate_piecewise_linear_profile(profile, previous_volume, next_volume);
    layered_discharge_result->discharge_per_layer[layer] =
        relative_discharge_layer / relative_discharge_total * total_discharge;
    previous_volume = next_volume;
  }
  return 0;
}
