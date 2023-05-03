import math

import pyzsf


lock_parameters = {
    "lock_length": 400.0,
    "lock_width": 50.0,
    "lock_bottom": -15.5,
}

boundary_conditions = {
    "head_lake": -0.40,
    "head_sea": 0.05,
    "salinity_lake": 5,
    "salinity_sea": 25,
}

operational_parameters = {
    "num_cycles": 15,
    "door_time_to_open": 210.0,
    "leveling_time": 600.0,
    "calibration_coefficient": 0.77,
}

dt = 60 * 10


class Reservoir:
    def __init__(self, length, width, bottom_level, water_level, salinity_initial):
        self.length = length
        self.width = width
        self.bottom_level = bottom_level
        self.water_level = water_level
        self.mass = salinity_initial * self.volume

    def update(self, discharge_in, salinity_in, mass_d, c_spui, mass_lockage):
        mass_in = discharge_in * salinity_in * dt
        mass_out = -discharge_in * c_spui * self.salinity * dt
        mass_d = mass_d
        mass_lockage = mass_lockage
        self.mass += mass_in + mass_out + mass_d + mass_lockage
        return self.mass

    @property
    def volume(self):
        return self.length * self.width * (self.water_level - self.bottom_level)

    @property
    def salinity(self):
        assert self.mass > 0
        return max(self.mass / self.volume, 0)


class Connection:
    def __init__(self, a: Reservoir, b: Reservoir):
        self.a = a
        self.b = b

    def dispersion(self, c_d):
        delta_salinity = self.a.salinity - self.b.salinity
        red_g = (
            9.81
            * (0.8 * delta_salinity)
            / (
                1000
                + 0.8
                * (self.b.salinity + self.a.salinity)
                / 2
            )
        )
        h_min = abs(
            min(
                (self.b.bottom_level - self.b.water_level),
                (self.a.bottom_level - self.a.water_level),
            )
        )
        if delta_salinity > 0:
            c_le = (1 / 2) * math.sqrt(red_g * h_min)
        elif delta_salinity < 0:
            c_le = -(1 / 2) * math.sqrt(-red_g * h_min)
        else:
            c_le = 0
        discharge_le = c_le * self.b.width * (h_min / 2)
        discharge_d = c_d * discharge_le
        mass_d = discharge_d * delta_salinity * dt
        return discharge_d, mass_d


approach_harbor = Reservoir(7000, 280, -15, -0.4, 10.3)
canal = Reservoir(40000, 220, -15, -0.4, 8.0)
ah_canal = Connection(approach_harbor, canal)


def ztm(head_sea=0.05, head_lake=-0.40, salinity_lake=10.3):
    salinity = approach_harbor.salinity
    parameters = {
        **lock_parameters,
        **boundary_conditions,
        "head_sea": head_sea,
        "salinity_lake": salinity,
        "head_lake": head_lake,
        **operational_parameters,
    }
    results = pyzsf.zsf_calc_steady(**parameters)
    discharge_spui = 68
    discharge_level = results["discharge_from_lake"] - results["discharge_to_lake"]
    discharge_in = discharge_spui + discharge_level
    discharge_d, mass_d = ah_canal.dispersion(c_d=0.55)
    approach_harbor.update(
        discharge_in,
        canal.salinity,
        -mass_d,
        c_spui=0.782,
        mass_lockage=-(results["salt_load_lake"] * dt),
    )
    canal.update(discharge_in, 0.2, mass_d, c_spui=1, mass_lockage=0)
    salinity = approach_harbor.salinity
    return results["salt_load_lake"]
