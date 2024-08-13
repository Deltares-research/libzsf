/* read time averaged data from csv */

#include "load_phase_wise.h"
#include "zsf.h"

// -- Define setters, all are named set_(attribute_name)
CSV_INT_SETTER(phase_wise_row_t, routine);
CSV_DOUBLE_SETTER(phase_wise_row_t, ship_volume_lake_to_sea);
CSV_DOUBLE_SETTER(phase_wise_row_t, ship_volume_sea_to_lake);
CSV_DOUBLE_SETTER(phase_wise_row_t, t_flushing);
CSV_DOUBLE_SETTER(phase_wise_row_t, t_level);
CSV_DOUBLE_SETTER(phase_wise_row_t, t_open_lake);
CSV_DOUBLE_SETTER(phase_wise_row_t, t_open_sea);
CSV_DOUBLE_SETTER(phase_wise_row_t, density_current_factor_sea);
CSV_DOUBLE_SETTER(phase_wise_row_t, density_current_factor_lake);


// Load time averaged data from csv.
int load_phase_wise_timeseries(csv_context_t *context, char *filepath) {
  int status = init_csv_context(context);

  // Set up columns
  status = status || def_csv_column(context, "time", double_type, set_dummy);
  status = status || def_csv_column(context, "routine", int_type, set_routine);
  status = status || def_csv_column(context, "ship_volume_lake_to_sea", double_type,
                                    set_ship_volume_lake_to_sea);
  status = status || def_csv_column(context, "ship_volume_sea_to_lake", double_type,
                                    set_ship_volume_sea_to_lake);
  status = status || def_csv_column(context, "t_flushing", double_type, set_t_flushing);
  status = status || def_csv_column(context, "t_level", double_type, set_t_level);
  status = status || def_csv_column(context, "t_open_lake", double_type, set_t_open_lake);
  status = status || def_csv_column(context, "t_open_sea", double_type, set_t_open_sea);
  status = status || def_csv_column(context, "density_current_factor_lake", double_type,
                                    set_density_current_factor_lake);
  status = status || def_csv_column(context, "density_current_factor_lake", double_type,
                                    set_density_current_factor_lake);
  return status || load_csv(context, filepath) ? CSV_ERROR : CSV_OK;
}
