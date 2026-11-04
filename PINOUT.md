# ESP32 DEVKITV1 Pinout - Steve

## SD Card Module (SPI)

| SD Module | Board Label | GPIO |
|-----------|-------------|------|
| CS        | D5          | 5    |
| MOSI      | D23         | 23   |
| MISO      | D19         | 19   |
| SCK       | D18         | 18   |
| VCC       | 3V3         | -    |
| GND       | GND         | -    |

> Hardware VSPI defaults. These pins are fixed by the ESP32 SPI peripheral.

## Audio Output (Internal DAC -> LM386 Amplifier)

| Signal     | Board Label | GPIO |
|------------|-------------|------|
| DAC output | D25         | 25   |

| LM386 Pin | Connection                          |
|-----------|-------------------------------------|
| IN        | D25 via 100R + 0.1uF RC filter      |
| VCC       | VIN (5V)                            |
| GND       | GND                                 |
| OUT       | Speaker                             |

## HC-SR04 Ultrasonic Sensor

| HC-SR04 | Board Label | GPIO |
|---------|-------------|------|
| VCC     | VIN (5V)    | -    |
| GND     | GND         | -    |
| Trig    | D27         | 27   |
| Echo    | D33         | 33   |

## Power

| Rail | Board Pin |
|------|-----------|
| 3.3V | 3V3      |
| 5V   | VIN      |
