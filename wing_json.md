
Sample Live data JSON object:

```json
{
  "type": "live",
  "fan": {
    "pwm": 1100,
    "rpm": 0
  },
  "servo": {
    "pos": 89.8,
    "rpm": 0,
    "cur": 0,
    "vol": 6291,
    "tmp": 269,
    "stat": 1
  }
}
```

Sample CFG JSON object:

```json
{
  "type": "cfg",
  "fan": {
    "min": 1125,
    "max": 1300
  },
  "servo": {
    "model": "LSS-HT1",
    "fw": "370",
    "stiff": -2,
    "hStiff": 2,
    "acc": 2000,
    "dec": 200,
    "maxSpd": 0,
    "led": 0,
    "gyre": 1
  }
}
```