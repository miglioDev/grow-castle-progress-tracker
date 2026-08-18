# Grow Castle Progress Tracker Documentation

This document explains the main program logic and formulas I used. The tracker reads and
writes CSV files in the local `data` directory.
I am maintaining backwards compatibility between the various versions of the CSV.

## Player Data

The **Player Data** tab stores the current wave, Infinity Castle level, Leader
level, Town Archer level, and Castle level. When the data is saved, the
application adds a timestamp and appends a new record to
`data/player_data.csv`.

The Custom area stores additional heroes or towers in
`data/custom_heroes.csv`. Each custom entry has a name, current level, and
target ratio. Custom heroes are included in ratio analysis and investment
planning.

## Ratios, Levels, and Economy

For every tracked unit, the current ratio is calculated as:

```text
current ratio = unit level / current wave
```

Leader, Town Archer, and Castle can be compared with editable recommended
ratios. The level gap is calculated as:

```text
level gap = current level - (current wave * target ratio)
```

(A negative gap means that the unit is below its target). 
I excluded Infinity Castle from this metric as it is a special

The colony gold calculation is:

```text
base colony gold = 4500 * Infinity Castle level + 10000
gold with XP skill = base colony gold * 1.20
gold with Whip and skill = base colony gold * 1.35
```

### Investment and Cost Planning

The investment section calculates the cumulative gold currently invested in
each unit using its cost model. It also calculates the target level for the
current wave and for a future projected wave:

```text
target level now = target ratio * current wave
projected wave = current wave + WPH * projection hours
target level next period = target ratio * projected wave
```

For each unit:

```text
investment gold = cost(current level)
cost to target now = max(cost(target level now) - investment gold, 0)
cost to target next period = max(cost(target level next period)
								- cost(target level now), 0)
```

The investment percentage is:

```text
investment percentage = unit investment gold / total investment gold
```

The projection period is selected in days in the UI and converted to hours
when used by the investment calculation.

In future, I might add GP by calculating it as Total Investment / by the expected cost for that wave, but I’m not yet sure how to implement and calibrate it. 

## Pace and Season Analysis

The **Pace & Season Analysis** tab accepts Devil Horn level, game speed (2x or
3x), Golden Horn, Horn, Chrono, OB, and MBF. The base time per wave is
calculated from game speed, bonuses, and a fixed overhead:

```text
multiplier = Chrono multiplier * Golden Horn multiplier * Horn multiplier
base time per wave = (100 / game speed) / multiplier + 4
RWPH base = 3600 / base time per wave
```

My current implementation uses these multipliers:

```text
None = 1.00, Passive = 1.10, Yellow = 1.14, Blue = 1.20
Golden Horn = 10 / 7
Horn = 10 / 9
```

In my program DH and tap heroes affect the wave count through skip fractions:

```text
skip fraction = Devil Horn level
			   + 0.20 if OB is enabled
			   + 0.40 if MBF is enabled
WPH = RWPH base * (1 + skip fraction)
waves per day = WPH * 24
waves per five-day season = WPH * 120
```

RWPH is rounded for display and for the profit estimate, while WPH is kept as
a floating-point value for wave projections.

### Historical Pace and Downtime

The program parses saved timestamps and compares the first and last valid
snapshot in the selected period. The actual historical pace is:

```text
actual WPH = (last wave - first wave) / elapsed hours
```

The expected time for those waves is based on the calculated WPH:

```text
expected hours = completed waves / expected WPH
downtime hours = max(elapsed hours - expected hours, 0)
downtime percentage = downtime hours / elapsed hours * 100
```

Available periods are All Time, Last Month, Last 5 Days, and Last 24 Hours.
At least two valid snapshots with different timestamps are required.

## Infinity Castle Stats and History

The **IC Stats & History** tab shows the current Infinity Castle ratio:

```text
Infinity Castle ratio = Infinity Castle level / current wave
```

It also displays colony gold values and IC ratio for
saved progress records. The history table includes the timestamp, wave,
Infinity Castle level, and ratio for each displayed record.

## Upgrading Cost and Profit

The **Upgrading Cost & Profit** tab contains the upgrade calculator and the
profit estimate section.

### Upgrade Cost Models

The cumulative cost function has the general form:

```text
cost(level) = quadratic * level^2 + linear * level + constant
```

The current parameters in my program are:

| Unit type | Quadratic | Linear | Constant |
| --- | ---: | ---: | ---: |
| Castle | 1250 | 0 | 0 |
| Town Archers | 500 | 0 | 0 |
| Infinity Castle | 1250 | 0 | 0 |
| Leader | 2500 | 0 | 0 |
| Custom Hero | 2500 | 0 | 0 |

The cost between two levels is calculated as:

```text
upgrade cost = max(cost(target level) - cost(start level), 0)
```

### Profit Estimate

The profit estimate uses data already available from the tracker. It does not
ask for separate gold, time, or gameplay input.

For a historical interval, the period is the time between the first and last
valid snapshots in the selected history range. The reference wave is the wave
at the beginning of that interval. Active time is the period after downtime.

TAB profit uses hours:

```text
active hours = max(period hours - downtime hours, 0)
TAB profit = RWPH * 449 * reference wave * active hours
```

The constant `449` is defined in `include/profit.h`. The interface shows both
the latest valid historical estimate and a future projection based on the
existing Projection Days setting. 
I worked out this constant myself from the game, and it might be slightly off, This is the method I used
K = (pure TAB gold = excluding IC) / (rwph * wave * active time of TAB)

The future projection uses the current wave
and assumes zero future downtime.

These values are estimates, not an exact balance. Saving progress at
regular intervals gives the historical calculation a better representation of
the player's real activity.

## Local Data and Backup

The application stores data locally in CSV files:

```text
data/player_data.csv    Player snapshots and recommended ratios
data/custom_heroes.csv  Custom hero names, levels, and target ratios
data/pace_data.csv      Saved pace settings
```

Copy these files to make a backup or move progress to another installation.
Restoring the files and restarting the application reloads the saved data.