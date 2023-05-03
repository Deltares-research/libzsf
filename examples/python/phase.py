import pprint

import pyzsf


lock_parameters = {
    "lock_length": 148.0,
    "lock_width": 14.0,
    "lock_bottom": -4.4,
}

boundary_conditions = {
    "head_lake": 0.0,
    "salinity_lake": 5.0,
    "temperature_lake": 15.0,
    "head_sea": 2.0,
    "salinity_sea": 25.0,
    "temperature_sea": 15.0,
}

operational_parameters = {
    "ship_volume_sea_to_lake": 1000.0,
    "ship_volume_lake_to_sea": 1000.0,
}

z = pyzsf.ZSFUnsteady(15.0, 0.0, **lock_parameters, **boundary_conditions, **operational_parameters)

print("State after initialization")
pprint.pprint(z.state)

print("\nPhase 1:\n" + "*" * 8)

results = z.step_phase_1(300.0)
print("Transports:")
pprint.pprint(results)
print("State:")
pprint.pprint(z.state)

print("\nPhase 2:\n" + "*" * 8)
results = z.step_phase_2(840.0)
print("Transports:")
pprint.pprint(results)
print("State:")
pprint.pprint(z.state)

print("\nPhase 3:\n" + "*" * 8)
results = z.step_phase_3(300.0)
print("Transports:")
pprint.pprint(results)
print("State:")
pprint.pprint(z.state)

print("\nPhase 4:\n" + "*" * 8)
results = z.step_phase_4(840.0, ship_volume_sea_to_lake=800.0)
print("Transports:")
pprint.pprint(results)
print("State:")
pprint.pprint(z.state)
