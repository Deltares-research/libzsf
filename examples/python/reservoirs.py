import pyzsf
import math

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
    "num_cycles":15,
    "door_time_to_open": 210.0,
    "leveling_time": 600.0,
    "calibration_coefficient": 0.77,
}

dt = (60 * 10)

class reservoir():
    def __init__(self, length, width, bottom_level, water_level, salinity_initial):
        self.length = length
        self.width = width
        self.bottom_level = bottom_level
        self.water_level = water_level
        self.mass = salinity_initial * self.volume_reservoir
        
    def update(self,Q_in,S_in,mass_d,c_spui,mass_lockage):
        mass_in = Q_in * S_in * dt
        mass_out = -Q_in * c_spui * self.salinity_reservoir * dt
        mass_d = mass_d
        mass_lockage = mass_lockage
        self.mass += mass_in + mass_out + mass_d + mass_lockage
        return self.mass
     
    @property
    def volume_reservoir(self):
        return self.length * self.width * (self.water_level - self.bottom_level)
     
    @property
    def salinity_reservoir(self):
        assert self.mass > 0
        return max(self.mass / self.volume_reservoir,0)

approachharbour = reservoir(7000, 280, -15, -0.4, 10.3)
canal = reservoir(40000, 220, -15, -0.4, 8.0)

class connection():
    def __init__(self, reservoir1, reservoir2):        
        self.reservoir1 = reservoir1
        self.reservoir2 = reservoir2

    def dispersion(self, c_d):
        delta_S = self.reservoir1.salinity_reservoir - self.reservoir2.salinity_reservoir
        red_g = 9.81 * (0.8 * delta_S) / (1000 + 0.8 * (self.reservoir2.salinity_reservoir + self.reservoir1.salinity_reservoir) / 2)
        h_min = abs(min((self.reservoir2.bottom_level - self.reservoir2.water_level), (self.reservoir1.bottom_level - self.reservoir1.water_level)))
        if delta_S > 0:
            c_le = (1/2) * math.sqrt(red_g * h_min)
        elif delta_S < 0:
            c_le = -(1/2) * math.sqrt(-red_g * h_min)
        else: 
            c_le = 0
        Q_le = c_le * self.reservoir2.width * (h_min / 2)
        Q_d = c_d * Q_le
        mass_d = Q_d * delta_S * dt 
        return Q_d, mass_d
  
ah_canal = connection(approachharbour,canal) 

def ZTM(head_sea = 0.05,head_lake = -0.40, salinity_lake = 10.3):
    salinity = approachharbour.salinity_reservoir 
    parameters = {**lock_parameters, **boundary_conditions, "head_sea": head_sea, "salinity_lake": salinity, "head_lake":head_lake, **operational_parameters}
    results = pyzsf.zsf_calc_steady(**parameters) 
    Q_spui = 68
    Q_level = results["discharge_from_lake"] - results["discharge_to_lake"]
    Q_in = Q_spui + Q_level 
    Q_d, mass_d = ah_canal.dispersion(c_d = 0.55) 
    approachharbour.update(Q_in, canal.salinity_reservoir, -mass_d, c_spui = 0.782, mass_lockage = -(results["salt_load_lake"]*dt))
    canal.update(Q_in, 0.2, mass_d, c_spui = 1, mass_lockage = 0)
    salinity = approachharbour.salinity_reservoir
    return results["salt_load_lake"]