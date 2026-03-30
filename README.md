# M5Unit - GESTURE

## Overview

Library for UnitGESTURE using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U127

Unit Gesture is a 3D gesture recognition sensor using the I2C communication interface. It adopts the PAJ7620U2 sensor solution, with the program by default supporting 9 types of gesture recognition. The maximum gesture update frequency can reach 240Hz, and it has a certain level of ambient light interference resistance. It supports custom unit sampling time and can add recognition gesture combinations through program algorithms as needed. The sensor features strong stability, fast recognition speed, high accuracy, and low power consumption (operating current only 2.2mA), making it suitable for various applications such as non-contact remote controls, robot interaction, human-computer interaction games, and gesture light control.

Supports 9 gestures:

Up, Down, Left, Right, Forward, Backward, Clockwise, CounterClockwise, Wave


## Related Link
See also examples using conventional methods here.

- [Unit GESTURE & Datasheet](https://docs.m5stack.com/en/unit/Gesture)

## Required Libraries:

- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-GESTURE - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

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
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)
