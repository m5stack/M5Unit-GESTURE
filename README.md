# M5Unit - GESTURE

## Overview

Library for UnitGESTURE using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U127

Unit Gesture is a 3D gesture recognition sensor using the I2C communication interface. It adopts the PAJ7620U2 sensor solution, with the program by default supporting 9 types of gesture recognition. The maximum gesture update frequency can reach 240Hz, and it has a certain level of ambient light interference resistance. It supports custom unit sampling time and can add recognition gesture combinations through program algorithms as needed. The sensor features strong stability, fast recognition speed, high accuracy, and low power consumption (operating current only 2.2mA), making it suitable for various applications such as non-contact remote controls, robot interaction, human-computer interaction games, and gesture light control.

Supports 9 gestures:

Up, Down, Left, Right, Forward, Backward, Clockwise, CounterClockwise, Wave


## Related Link

- [Unit GESTURE & Datasheet](https://docs.m5stack.com/en/unit/Gesture)

## Required Libraries:

- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-GESTURE - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

### For ESP-IDF settings

> **NOTE:** The ESP-IDF native build (`idf.py`) targets ESP-IDF **5.1 or later** (5.x and 6.x).

This library covers a single unit, so the examples need no Kconfig choice. Build them directly:

```sh
cd examples/UnitUnified/PlotToSerial
idf.py set-target esp32s3               # or esp32 / esp32c5 / esp32c6 / esp32h2 / esp32p4 / ...
idf.py build flash monitor
```

`BLEKeyboard` depends on an Arduino-only BLE keyboard library, so it is available for Arduino only.

## Support via [PaHub](https://docs.m5stack.com/en/unit/Unit-PaHub%20v2.1)

|Unit|Support|Note|
|---|---|---|
|UnitGesture|OK|See the `ViaPaHub` example|

See also [M5Unit-HUB](https://github.com/m5stack/M5Unit-HUB)

## Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-GESTURE/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxygen](https://www.doxygen.nl/)
- [Git](https://git-scm.com/) (Output commit hash to html)
