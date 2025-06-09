# DHT11 Driver for GD32VF103

This is a simple driver for the DHT11 temperature and humidity sensor,
written for the GD32VF103 MCU using the GD32VF103 standard peripheral library.

## Features
- Reads temperature and humidity
- Blocking (Timer based) implementation

## Dependencies
- GD32VF103 Standard Peripheral Library
- General Purpose Timer6 


## Note
This driver is specific to the GD32VF103 platform and uses functions like:
```c
gpio_input_bit_get(GPIOA, GPIO_PIN_1);
or
timer_enable(TIMER6);


