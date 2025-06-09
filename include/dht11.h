#ifndef MINFIL_H
#define MINFIL_H

#include <stdint.h>

/*!
 	\brief Initilizes the DHT11 module. Selected GPIOx Configures the data pin. RCU for GPIO and timer6 init
    \param[in]  gpio_periph: GPIOx(x = A,B,C,D,E)
    \param[in]  pin: GPIO pin
 */
void DHT11_init(uint32_t gpio_perpih, uint32_t pin);
/**
 * Polls the sensor and collects the data
 */
void DHT11_readData(void);

/**
 * @brief Returns DTH11 temp in celcius. Call DTH11_readData() first
 * @return Temp float temp val
 */
float DHT11_getTemp(void);

/**
 * @brief Returns DTH11 humidity. Call DTH11_readData() first
 * @return Humid float val
 */
uint8_t DHT11_getHumid(void);

/**
 * @brief Returns DTH11 temp integral. Call DTH11_readData() first
 * @return temp val decimal
 */
uint8_t getTempIntegral(void){
	return data.temp_integral;
}

/**
 * @brief Returns DTH11 temp decimal. Call DTH11_readData() first
 * @return temp val integral
 */
uint8_t getTempDecimal(void){
	return data.temp_decimal;
}

#endif