import pprint

import pandas as pd

import pyzsf


lock_parameters = {
    "lock_length": 300.0,
    "lock_width": 25.0,
    "lock_bottom": -7.0,
}

constant_boundary_conditions = {
    "head_lake": 0.0,
    "temperature_lake": 15.0,
    "temperature_sea": 15.0,
}

mitigation_parameters = {
    "density_current_factor_lake": 0.25,
    "density_current_factor_sea": 0.25,
    "distance_door_bubble_screen_lake": 10.0,
    "distance_door_bubble_screen_sea": 10.0,
    "flushing_discharge_high_tide": 0.0,
    "flushing_discharge_low_tide": 0.0,
    "sill_height_lake": 0.5,
}

# Initialize the lock
z = pyzsf.ZSFUnsteady(
    15.0, 0.0, **lock_parameters, **constant_boundary_conditions, **mitigation_parameters
)

# Read the lockages from a file
df_lockages = pd.read_csv("lockages.csv", index_col=0)
lockages = list(df_lockages.to_dict("records"))

# Go through all lockages
all_results = []

for parameters in lockages:
    routine = int(parameters.pop("routine"))
    t_open_lake = parameters.pop("t_open_lake")
    t_open_sea = parameters.pop("t_open_sea")
    t_level = parameters.pop("t_level")
    t_flushing = parameters.pop("t_flushing")

    parameters["ship_volume_sea_to_lake"] = 0.0
    parameters["ship_volume_lake_to_sea"] = 0.0

    if routine == 1:
        assert t_level > 0
        results = z.step_phase_1(t_level, **parameters)
    elif routine == 2:
        results = z.step_phase_2(t_open_lake, **parameters)
    elif routine == 3:
        assert t_level > 0
        results = z.step_phase_3(t_level, **parameters)
    elif routine == 4:
        results = z.step_phase_4(t_open_sea, **parameters)
    elif routine in {-2, -4}:
        results = z.step_flush_doors_closed(t_flushing, **parameters)
    else:
        raise Exception(f"Unknown routine '{routine}'")

    all_results.append(results)

# Aggregate results
duration = 60 * 24 * 3600  # 60 days

overall_results = {}
overall_mass_to_sea = 0.0
overall_mass_to_lake = 0.0

for results in all_results:
    for k, v in results.items():
        if k.startswith(("volume_", "mass_")):
            overall_results[k] = overall_results.get(k, 0.0) + v

    overall_mass_to_sea += results["volume_to_sea"] * results["salinity_to_sea"]
    overall_mass_to_lake += results["volume_to_lake"] * results["salinity_to_lake"]

overall_results["salinity_to_sea"] = overall_mass_to_sea / overall_results["volume_to_sea"]
overall_results["salinity_to_lake"] = overall_mass_to_lake / overall_results["volume_to_lake"]

overall_discharges = {}
for k, v in overall_results.items():
    if k.startswith("volume_"):
        overall_discharges[f"discharge_{k[7:]}"] = v / duration
overall_results.update(overall_discharges)

assert overall_results.keys() == all_results[0].keys()

# Log to console
print("Overall results (60 day aggregates and averages):")
pprint.pprint(overall_results)
