# Fixed_Wing_UAV_v2

Repository for AMRC & Dassault Systemes collaboration around "Verification and Validation of a Systems Engineering Model against a real-world artifact" - exact title TBC

Arduino code for running physical system among other files.

![FixedWing](/images/FixedWing.png)

## Physical Components used:

* Main controller - Arduino UNO 
* EDF fan -[EDF Ducted Fan JP Hobby 120mm + 12s Motor 760KV (CCW) - TURBINES RC](https://www.turbines-rc.com/en/jp-hobby/1620-edf-ducted-fan-jp-hobby-120mm-12s-motor-760kv.html)
* Servo - [Lynxmotion Smart Servo High Speed (LSS-HT1)](https://wiki.lynxmotion.com/info/wiki/lynxmotion/view/ses-v2/lynxmotion-smart-servo/)
* Servo controller - [LSS-ADA](https://wiki.lynxmotion.com/info/wiki/lynxmotion/view/servo-erector-set-system/ses-electronics/ses-modules/lss-adapter-board-type-c/)
* ESC / Fan Controller - [TURBINE RCJP Hobby 160A ESC 6-14S (OPTO) - TURBINES RC](https://www.turbines-rc.com/en/50a-and-over/2942-jp-hobby-160a-esc-6-14s-ubec-10a.html)
* DC-DC converter (24V to 12V) - [Matek Systems Matek Systems- Power Module PM12S-3](https://www.3dxr.co.uk/electronics-c78/power-management-c91/voltage-regulators-becs-c101/matek-systems-power-module-pm12s-3-p5375)
* Contactor -[TE Connectivity LEV200 Contactor, 24 V Coil, 1-Pole, 500 A 2…](https://uk.rs-online.com/web/p/contactors/7827082?gb=s)
* 24V power supply
* Fuse
* Isolator
* E-stop


## Example JSON Outputs:

Sample Live data JSON object, sent at 10hz

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

Sample CFG JSON object, mostly static data sent at 1hz to confirm config changes:

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


## Serial Command Protocol (Input)

The controller accepts a standardized, human-readable string protocol via the main USB Serial connection. This allows for direct manual control via a Serial Monitor or seamless integration with custom software (Python, MATLAB, etc.).

**Connection Settings:** `115200 Baud` | **Line Ending:** `Newline (\n)`

### Command Schema

| Command Type | Syntax | Description | Example |
| :--- | :--- | :--- | :--- |
| **Fan Control** | `F:<PWM>` | Sets the EDF thrust using precise 16-bit hardware PWM pulse widths (in microseconds). Bound by the `LAB_SAFE_MAX` firmware governor (1300µs limit). | `F:1200` *(Sets fan to 1200µs)* |
| **Servo Control** | `S:<Degrees>:<Time>` | Commands the LSS control surface to a specific angle over a defined duration (milliseconds). This built-in motion profiling prevents turbulent snapping. | `S:45.0:1000` *(Sweeps to 45° smoothly over 1 second)* |
| **Dynamic Config** | `C:<Type>:<Value>` | Adjusts LSS parameters on the fly without recompiling firmware. Types include `AS` (Angular Stiffness), `AA` (Accel), `AD` (Decel), etc. | `C:AS:2` *(Sets Angular Stiffness to 2)* |
| **Emergency Stop** | `STOP` | **Hardware Kill-Switch.** Instantly bypasses all motion profiles, drops EDF signal to arming state (1100µs), and sets the servo to limp mode. | `STOP` |

### Dynamic Configuration (`C:` Command Reference)

The `C:` command allows you to tune the Lynxmotion Smart Servo (LSS) characteristics in real-time. This is particularly useful for adjusting how the control surface reacts to aerodynamic loads without needing to recompile or restart the flight controller.

**Format:** `C:<Type>:<Value>`
*(Note: Any changes made here will be immediately reflected in the 1Hz `cfg` JSON output loop).*

| Parameter (`<Type>`) | LSS Function | Description | Acceptable Range | Example |
| :--- | :--- | :--- | :--- | :--- |
| **`AS`** | **Angular Stiffness** | Controls how aggressively the servo tries to reach its moving target position. A higher value means a stiffer, punchier response. | `-4` to `4` (Default: `0`) | `C:AS:2` |
| **`AH`** | **Holding Stiffness** | **Critical for aero loads.** Controls how strongly the servo fights to hold its final position against the physical wind/thrust from the fan. | `-4` to `4` (Default: `0`) | `C:AH:3` |
| **`AA`** | **Acceleration** | The rate at which the servo ramps up to its maximum speed (Degrees/sec²). Lower values create a smoother, softer start. | `0` to `10000` | `C:AA:2000` |
| **`AD`** | **Deceleration** | The rate at which the servo ramps down as it approaches the target (Degrees/sec²). Lower values create a cushioned stop. | `0` to `10000` | `C:AD:200` |
| **`SD`** | **Maximum Speed** | Caps the absolute top speed of the servo in degrees per second. A value of `0` removes the cap. | `0` to `1800` | `C:SD:60` |
| **`LED`** | **LED Color** | Changes the physical LED color on the back of the LSS servo. Excellent for creating visual debug states during test routines. | `0` (Off) to `7` (White) | `C:LED:3` *(Blue)* |
| **`GY`** | **Gyre** | Sets the direction of rotation. Useful if you need to quickly invert the control surface logic (e.g., swapping from an elevator to an aileron setup). | `1` (CW) or `-1` (CCW) | `C:GY:-1` |

