# Grow Castle Progress Tracker

Hi, here’s all the maths behind the game that I used in my code. Generally, the formulas are taken from the game itself. If you have any other questions or have spotted any errors, please do get in touch!

## Player Data

The application stores local CSV data in `data/` and reloads it at startup.
player_data: 
Each saved record contains wave, Infinity Castle, Leader, Town Archer, Castle and a timestamp

custom_hero Custom heroes store a name, level, and
target ratio.

## Ratios & Economy

```text
current ratio = unit level / current wave
level gap = current level - (current wave * target ratio)
```

```text
base colony gold = 4500 * Infinity Castle level + 10000
XP skill gold = base colony gold * 1.20
Whip + skill gold = base colony gold * 1.35
```

## Investment and Cost

```text
target level = target ratio * current wave
projected wave = current wave + WPH * projection hours
projected target level = target ratio * projected wave

investment = cost(current level)
cost to target now = max(cost(target level) - investment, 0)
cost to target next period = max(cost(projected target level) - cost(target level), 0)
investment percentage = unit investment / total investment
```

## Gold Power

The section uses the current wave, the total gold invested in the build, a pace
source, and the estimated gold income per season.

```text
gp = total_gold_invested / (wave^2 * K)
future_gp_no_spend = total_gold_invested / ((wave + pace)^2 * K)
gp_loss_per_season = future_gp_no_spend - gp
gp_loss_pct = 1 - (future_gp_no_spend / gp)

max_gp = (income_per_season / pace) / (K * wave * 2)
gp_gain = income_per_season / ((wave + pace)^2 * K)
gp_gain_pct = gp_gain / gp
max_gp_gold_gap = (max_gp - gp) * (wave^2 * K)
```

Optional inputs are not calculated or displayed until provided:

```text
gp_plus_saved = (total_gold_invested + saved_gold_equivalent) / (wave^2 * K)
saved_gold_gap = (gp_plus_saved - gp) * (wave^2 * K)

gp_desired = desired_gold_target / (wave^2 * K)
desired_gold_gap = (gp_desired - gp) * (wave^2 * K)
```

The table displays the GP values in its first row. Its second row displays the
corresponding gold gap, or the loss/gain percentage for the last two columns. A
negative desired gold gap means the desired target has already been exceeded.

## Pace and Season

```text
bonus multiplier = Chrono * Golden Horn * Horn
base time per wave = (100 / game speed) / bonus multiplier + 4
RWPH = round(3600 / base time per wave)

skip fraction = Devil Horn level + (0.20 if OB) + (0.40 if MBF)
WPH = (3600 / base time per wave) * (1 + skip fraction)
waves per day = WPH * 24
waves per season = WPH * 120
```

Chrono multipliers: None `1.00`, Passive `1.10`, Yellow `1.14`, Blue `1.20`.
Golden Horn is `10/7`; Horn is `10/9`.

### Historical Pace and Downtime

```text
actual WPH = (last wave - first wave) / elapsed hours
expected hours = completed waves / calculated WPH
downtime hours = max(elapsed hours - expected hours, 0)
downtime percentage = downtime hours / elapsed hours * 100
```

## IC Stats and History

```text
Infinity Castle ratio = Infinity Castle level / current wave
```

## Upgrade Costs

```text
Castle cost(level) = 1250 * level^2
Town Archer cost(level) = 500 * level^2
upgrade cost = max(cost(to level) - cost(from level), 0)
```

For Leader, Hero, Tower, and Custom Hero, with $T(n) = n(n - 1) / 2$:

```text
hero cost(n) = 3000 * T(min(n, 5000))
             + 4000 * (T(min(n, 10000)) - T(5000)), when n > 5000
             + 5000 * (T(n) - T(10000)), when n > 10000
```

## GPW

Gold Auto Battle samples are temporary and are not saved. The current saved
wave is used with the fixed GAB cost of `456` gold per wave.

```text
average gold = sum(samples) / sample count
GPW average = average gold / current wave
GPW maximum = maximum sample / current wave
GPW minimum = minimum sample / current wave

profit per wave = GPW - 456
```

GAB is profitable when average profit per wave is greater than zero. Fewer than
five samples display a reliability warning.

## Local Data

```text
data/player_data.csv
data/custom_heroes.csv
data/pace_data.csv
```

Copy these files to back up or transfer data.
