C API
=====

Structures
----------

.. _sec_c_api_input:

Input
^^^^^

.. c:struct:: zsf_param_t

   The parameter structure both for phase-wise calculation, and for steady state calculation.

   .. c:var:: double lock_length

      The length of the lock in meters.

   .. c:var:: double lock_width

      The width of the lock in meters.

   .. c:var:: double lock_bottom

      The bottom of the lock in meters with respect to datum (e.g. mNAP).

   .. c:var:: double num_cycles

      (Steady) The number of cycles (leveling from lake to sea and back again) the
      lock makes in a day.

   .. c:var:: double door_time_to_open

      (Steady) The time it takes for the door to go from fully closed to fully
      (open in seconds.

   .. c:var:: double leveling_time

      (Steady) The average time it takes to level the lock to the sea or lake level in seconds.

   .. c:var:: double calibration_coefficient

      (Steady) The calibration coefficient on the time that the door is open.

   .. c:var:: double symmetry_coefficient

      (Steady) The imbalance between the lake door and right door being open, should be in the range (0, 2).
      A value of 1.0 means that the lake and sea side door are open equally long.

   .. c:var:: double ship_volume_sea_to_lake

      The water displacement of ships going from the sea to the lake in :math:`m^3`.

   .. c:var:: double ship_volume_lake_to_sea

      The water displacement of ships going from the sea to the lake in :math:`m^3`.

   .. c:var:: double salinity_lock

      The (initial) salinity of the lock in :math:`kg/m^3`.

   .. c:var:: double head_sea

      The head of the sea in meters with respect to datum (e.g. mNAP).

   .. c:var:: double salinity_sea

      The salinity of the sea in :math:`kg/m^3`.

   .. c:var:: double temperature_sea

      The temperature of the sea in degrees Celcius.

   .. c:var:: double head_lake

      The head of the lake in meters with respect to datum (e.g. mNAP).

   .. c:var:: double salinity_lake

      The salinity of the lake in :math:`kg/m^3`.

   .. c:var:: double temperature_lake

      The temperature of the lake in degrees Celcius.

   .. c:var:: double flushing_discharge_high_tide

      The flushing discharge in :math:`m3/s` when the sea level is higher than (or equal to) lake level.

   .. c:var:: double flushing_discharge_low_tide

      The flushing discharge in :math:`m3/s` when the lake level is strictly below the sea level.

   .. c:var:: double density_current_factor_sea

      The factor by which to multiply the velocity of the density current on the sea side.

   .. c:var:: double density_current_factor_lake

      The factor by which to multiply the velocity of the density current on the lake side.

   .. c:var:: double distance_door_bubble_screen_sea

      Distance of the bubble screen on the lake side to the door in meters.
      Positive values mean that the screen is outside the lock.
      Negative values mean that the screen is inside the lock.

   .. c:var:: double distance_door_bubble_screen_lake

      Distance of the bubble screen on the lake side to the door in meters.
      Positive values mean that the screen is outside the lock.
      Negative values mean that the screen is inside the lock.

   .. c:var:: double sill_height_sea

      Sill height on the sea side in meters above the bottom of the lock.

   .. c:var:: double sill_height_lake

      Sill height on the lake side in meters above the bottom of the lock.

   .. c:var:: double rtol

      (Steady) The relative tolerance of the salinity in the lock after phase 4 to determine whether convergence has been reached.

   .. c:var:: double atol

      (Steady) The absolute tolerance of the salinity in the lock after phase 4 to determine whether convergence has been reached.


Steady state output
^^^^^^^^^^^^^^^^^^^

These structures are output by :c:func:`zsf_calc_steady`.
Note that :c:struct:`zsf_aux_results_t` is an optional output of this function, and is not output/calculated by default.

.. c:struct:: zsf_results_t

      For mass and salt transport the definition is such that positive values are in the direction lake → lock → sea.
      Negative values mean that there is a net withdrawal of salt from the sea or net salt load on the lake.

   .. c:var:: double mass_transport_lake

      The mass transport of salt over the lake head in :math:`kg`.

   .. c:var:: double salt_load_lake

      The average salt transport to the lake :math:`kg/s`.

   .. c:var:: double discharge_from_lake

      The average discharge from the lake to the lock in :math:`m3/s`.

   .. c:var:: double discharge_to_lake

      The average discharge from the lock to the lake in :math:`m3/s`.

   .. c:var:: double salinity_to_lake

      The average salinity of the water going from the lock to the lake in :math:`kg/m^3`.

   .. c:var:: double volume_ship_to_lake

      The net water displacement of ships from the lock to the lake in :math:`m^3`.

   .. c:var:: double mass_transport_sea

      The mass transport of salt over the sea head in :math:`kg/m^3`.

   .. c:var:: double salt_load_sea

      The average salt transport to the sea :math:`kg/s`.

   .. c:var:: double discharge_from_sea

      The average discharge from the sea to the lock in :math:`m3/s`.

   .. c:var:: double discharge_to_sea

      The average discharge from the lock to the sea in :math:`m3/s`.

   .. c:var:: double salinity_to_sea

      The average salinity of the water going from the lock to the sea in :math:`kg/m^3`.

   .. c:var:: double volume_ship_to_sea

      The net water displacement of ships from the lock to the sea in :math:`m^3`.


.. c:struct:: zsf_aux_results_t

   Additional results that can be calculated for steady state operation.

   .. c:var:: double z_fraction

      The dimensionless salt transport per cycle.
      It is defined as a factor on the lock volume times the difference in salinity between the lake and sea side.

      .. math::

         M = Z_{fraction} V_{lock} \cdot \Delta S

      This way it represents what part of the lock, in the regular locking process, is exchanged by the density current and contributes to the salt transport.

      Because the salt transports are per head, and the volume of the lock is not always equal on sea and lake side, average values are used for these.

      .. math::

         Z_{fraction}= \frac{\overline M}{\overline V_{lock} \cdot \left( S_{sea} - S_{lake} \right) } = \frac{0.5 \cdot \left( M_{lake} + M_{sea} \right) }{0.5 \cdot \left( V_{lock,lake} + V_{lock,sea} \right) \cdot \left( S_{sea} - S_{lake} \right) }

   .. c:var:: double dimensionless_door_open_time

      The dimensionless door open time is :math:`T_{LE} / T_{open}`.
      In this calculation the value for :math:`T_{LE}` is calculated per lock head, with the corresponding salinity difference.
      That means that :math:`T_{LE}` is not just a function of the input, but is dependent on the calculation routines, and there are two values (one for each lock head).
      To get to a single :math:`T_{LE}` that is determined by the geometry and boundary conditions, we define a variant of :math:`T_{LE,lake-sea}` based on the density difference over the lock.

      .. math::

         c_{i,lake-sea} = \frac{1}{2} \sqrt{g' \overline H } = \frac{1}{2} \sqrt{ g \frac{\Delta \rho}{\bar \rho} \overline H } \approx \frac{1}{2} \sqrt{g \frac{0.8 \left( S_{sea} - S_{lake} \right) }{{\bar \rho}_{lake-sea}} \left( {\frac{{H_{lake} + H_{sea}}}{2}} \right)}

      .. math::

         T_{LE,lake-sea} = \frac{2L}{c_{i,lake-sea}}

   .. c:var:: double volume_to_lake

      The volume that is discharged to the lake per locking cycle in :math:`m^3`.

   .. c:var:: double volume_from_lake

      The volume that is withdrawn from the lake per locking cycle in :math:`m^3`.

   .. c:var:: double volume_to_sea

      The volume that is discharged to the sea per locking cycle in :math:`m^3`.

   .. c:var:: double volume_from_sea

      The volume that is withdrawn from the sea per locking cycle in :math:`m^3`.

   .. c:var:: double volume_lock_at_lake

      The volume of the lock when it is at sea level in :math:`m^3`.

   .. c:var:: double volume_lock_at_sea

      The volume of the lock when it is at sea level in :math:`m^3`.

   .. c:var:: double t_cycle

      The time it takes to complete one locking cycle in seconds.

   .. c:var:: double t_open

      The average value of the door open time on the lake and sea side, i.e. the average of :c:struct:`zsf_aux_results_t.t_open_lake` and :c:struct:`zsf_aux_results_t.t_open_sea`.

   .. c:var:: double t_open_lake

      The time the door is open on the lake side per locking cycle in seconds.

   .. c:var:: double t_open_sea

      The time the door is open on the sea side per locking cycle in seconds.

   .. c:var:: double salinity_lock_1

      The salinity of the lock after phase 1 in :math:`kg/m^3`.

   .. c:var:: double salinity_lock_2

      The salinity of the lock after phase 2 in :math:`kg/m^3`.

   .. c:var:: double salinity_lock_3

      The salinity of the lock after phase 3 in :math:`kg/m^3`.

   .. c:var:: double salinity_lock_4

      The salinity of the lock after phase 4 in :math:`kg/m^3`.

   .. c:var:: zsf_phase_transports_t transports_phase_1

      The phase transports in phase 1. See :c:struct:`zsf_phase_transports_t`

   .. c:var:: zsf_phase_transports_t transports_phase_2

      The phase transports in phase 2. See :c:struct:`zsf_phase_transports_t`

   .. c:var:: zsf_phase_transports_t transports_phase_3

      The phase transports in phase 3. See :c:struct:`zsf_phase_transports_t`

   .. c:var:: zsf_phase_transports_t transports_phase_4

      The phase transports in phase 4. See :c:struct:`zsf_phase_transports_t`


Phase-wise output
^^^^^^^^^^^^^^^^^

.. c:struct:: zsf_phase_state_t

   The state of the lock.
   Note that some of these values are redundant, but it is faster to store them than recalculate them every time.

   .. c:var:: double salinity_lock

      The salinity of the water in the lock in :math:`kg/m^3`.

   .. c:var:: double saltmass_lock

      The amount of salt in the lock in :math:`kg`.

   .. c:var:: double head_lock

      The head of the lock in meters with respect to datum (e.g. mNAP).

   .. c:var:: double volume_ship_in_lock

      The water displacement of a ship inside the lock in :math:`m^3`.


.. c:struct:: zsf_phase_transports_t

   For mass and salt transport the definition is such that positive values are in the direction lake → lock → sea.
   Negative values mean that there is a net withdrawal of salt from the sea or net salt load on the lake.

   .. c:var:: double mass_transport_lake

      The mass transport of salt over the lake head in :math:`kg`.

   .. c:var:: double volume_from_lake

      The volume of water that goes from the lake to the lock in :math:`m^3`.

   .. c:var:: double volume_to_lake

      The volume of water that goes from the the lock to the lake in :math:`m^3`.

   .. c:var:: double discharge_from_lake

      The average discharge of water going from the lake to the lock in :math:`m3/s`.

   .. c:var:: double discharge_to_lake

      The average discharge of water going from the lock to the lake in :math:`m3/s`.

   .. c:var:: double salinity_to_lake

      The average salinity of the water going from the lock to the lake in :math:`kg/m^3`.

   .. c:var:: double volume_ship_to_lake

      The net water displacement of ships from the lock to the lake in :math:`m^3`.

   .. c:var:: double mass_transport_sea

      The mass transport of salt over the sea head in :math:`kg`.

   .. c:var:: double volume_from_sea

      The volume of water that goes from the sea to the lock in :math:`m^3`.

   .. c:var:: double volume_to_sea

      The volume of water that goes from the the lock to the sea in :math:`m^3`.

   .. c:var:: double discharge_from_sea

      The average discharge of water going from the sea to the lock in :math:`m3/s`.

   .. c:var:: double discharge_to_sea

      The average discharge of water going from the lock to the sea in :math:`m3/s`.

   .. c:var:: double salinity_to_sea

      The average salinity of the water going from the lock to the sea in :math:`kg/m^3`.

   .. c:var:: double volume_ship_to_sea

      The net water displacement of ships from the lock to the sea in :math:`m^3`.


Functions
---------

.. c:function:: int zsf_initialize_state(const zsf_param_t *p, zsf_phase_state_t *state, double salinity_lock, double head_lock)

   Fill the state with an initial condition for an empty (no ships) lock.

.. c:function:: int zsf_step_phase_1(const zsf_param_t *p, double t_level, zsf_phase_state_t *state, zsf_phase_transports_t *results)

   Perform step 1: leveling to lake side

.. c:function:: int zsf_step_phase_2(const zsf_param_t *p, double t_open_lake, zsf_phase_state_t *state, zsf_phase_transports_t *results)

   Perform step 2: door open to lake side:

      - Ships leaving the lock chamber
      - Lock exchange with or without flushing
      - Ships entering the lock chamber

.. c:function:: int zsf_step_phase_3(const zsf_param_t *p, double t_level, zsf_phase_state_t *state, zsf_phase_transports_t *results)

   Perform step 3: leveling to sea side

.. c:function:: int zsf_step_phase_4(const zsf_param_t *p, double t_open_sea, zsf_phase_state_t *state, zsf_phase_transports_t *results)

   Perform step 4: door open to sea side:

      - Ships leaving the lock chamber
      - Lock exchange with or without flushing
      - Ships entering the lock chamber

.. c:function:: int zsf_step_flush_doors_closed(const zsf_param_t *p, double t_flushing, zsf_phase_state_t *state, zsf_phase_transports_t *results)

   Flush the lock with the doors closed.

.. c:function:: void zsf_param_default(zsf_param_t *p)

   Fill a :c:struct:`zsf_param_t` with default values.

.. c:function:: int zsf_calc_steady(const zsf_param_t *p, zsf_results_t *results, zsf_aux_results_t *aux_results)

   Calculate the salt intrusion for a set of parameters, assuming steady operation.

.. c:function:: const char * zsf_error_msg(int code)

   Get error message corresponding to error code.

.. c:function:: const char * zsf_version()

   Get version string.
