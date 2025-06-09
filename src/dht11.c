/* 
Replace this file with your code. Put your source files in this directory and any libraries in the lib folder. 
If your main program should be assembly-language replace this file with main.S instead.

Libraries (other than vendor SDK and gcc libraries) must have .h-files in /lib/[library name]/include/ and .c-files in /lib/[library name]/src/ to be included automatically.
*/

#include "gd32vf103.h"
#include "include\dht11.h"

typedef struct{
	uint8_t humid_integral;
	uint8_t humid_decimal;
	uint8_t temp_integral;
	uint8_t temp_decimal;
}DHT11_data;

static DHT11_data data;
static uint32_t sel_gpio_pin;
static uint32_t sel_gpio_perpih;
static timer_parameter_struct timer_initparam;


/*!
 	\brief Initilizes the DHT11 module. Selected GPIOx Configures the data pin. RCU for GPIO and timer6 init
    \param[in]  gpio_periph: GPIOx(x = A,B,C,D,E)
    \param[in]  pin: GPIO pin
 */
void DHT11_init(uint32_t gpio_perpih, uint32_t pin){

	sel_gpio_perpih = gpio_perpih;
	sel_gpio_pin = pin;

	switch(sel_gpio_perpih){
		case GPIOA: rcu_periph_clock_enable(RCU_GPIOA); break;
		case GPIOB: rcu_periph_clock_enable(RCU_GPIOB); break;
		case GPIOC: rcu_periph_clock_enable(RCU_GPIOC); break;
		case GPIOD: rcu_periph_clock_enable(RCU_GPIOD); break;
		case GPIOE: rcu_periph_clock_enable(RCU_GPIOE); break;
	}
	//gpio_init(gpio_perpih, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, pin);
	gpio_init(sel_gpio_perpih, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, sel_gpio_pin);
	//for(volatile int i = 0; i < 90000; i++);
	gpio_bit_set(sel_gpio_perpih, sel_gpio_pin);
	//gpio_bit_write(gpio_perpih, pin, RESET);

	rcu_periph_clock_enable(RCU_TIMER6);

	timer_parameter_struct timer_initparam;

	timer_initparam.prescaler = 32;
	timer_initparam.alignedmode = TIMER_COUNTER_EDGE;
	timer_initparam.counterdirection = TIMER_COUNTER_UP;
	timer_initparam.period = 64125;
	timer_initparam.clockdivision = TIMER_CKDIV_DIV1;
	timer_initparam.repetitioncounter = 1;
	timer_init(TIMER6, &timer_initparam);

	data.humid_decimal = 0;
	data.humid_integral = 0;
	data.temp_decimal = 0;
	data.temp_integral = 0;
}

/**
 * Polls the sensor and collects the data
 */
void DHT11_readData(void){

	//DBG: Config GPIOB7 to output binary pattern
	gpio_init(GPIOB, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
	
	timer_initparam.prescaler = 32;
	timer_initparam.alignedmode = TIMER_COUNTER_EDGE;
	timer_initparam.counterdirection = TIMER_COUNTER_UP;
	timer_initparam.period = 64125;
	timer_initparam.clockdivision = TIMER_CKDIV_DIV1;
	timer_initparam.repetitioncounter = 1;
	timer_init(TIMER6, &timer_initparam);

	//DBG!!!!
	gpio_bit_reset(GPIOB, GPIO_PIN_9);

	gpio_bit_reset(sel_gpio_perpih, sel_gpio_pin);			 	//Set GPIO LOW
	timer_flag_clear(TIMER6, TIMER_FLAG_UP);					//Clear update flag
	timer_enable(TIMER6);										//Enable TIMER6
	while( timer_flag_get(TIMER6, TIMER_FLAG_UP) == RESET );	//...Timer6 exp (19 ms)
	timer_flag_clear(TIMER6, TIMER_FLAG_UP);					//Clear update flag
	timer_disable(TIMER6);										//Turn of TIMER6 for now

	gpio_bit_set(sel_gpio_perpih, sel_gpio_pin);				//Set GPIO HIGH

	//Set Selected GPIO as input to read DHT11 response
	gpio_init(sel_gpio_perpih, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, sel_gpio_pin);

	while( gpio_input_bit_get(sel_gpio_perpih, sel_gpio_pin) == SET);
	while( gpio_input_bit_get(sel_gpio_perpih, sel_gpio_pin) == RESET);
	while( gpio_input_bit_get(sel_gpio_perpih, sel_gpio_pin) == SET);

	//DBG!!!!
	gpio_bit_set(GPIOB, GPIO_PIN_9);

	/**
	 * Reconfig Timer6 to count 1 microseconds
	 */
	timer_prescaler_config(TIMER6, 1, TIMER_PSC_RELOAD_NOW);
	timer_autoreload_value_config(TIMER6, 108);
	timer_counter_value_config(TIMER6, 0);


	/**
	 * param holds the total number of bits that has been gathered
	 */
	uint8_t bits = 0;
	
	/**
	 * param is the sequence of bits. Between [0 -> 4] where 0 is first humid and 4 is checksum
	 */
	uint8_t seq = 0;

	/**
	 * param is the weight used to shift collected bits. Range: [7-0]. Reset to 7 when reached 0
	 */
	int8_t weight = 7;

	/**
	 * Array holds collected data
	 * Byte 1: Integral Humid
	 * Byte 2: Decimal Humid
	 * Byte 3: Integral Temp
	 * Byte 4: Decimal Temp
	 * Byte 5: Checksum
	 */
	uint8_t gath_data[5] = {0,0,0,0,0};

	/**
	 * Following reads the total amount of 40 bits
	 */
	while( bits < 40 ){

		uint8_t us = 0;
		timer_counter_value_config(TIMER6, 0);								 //Reset counter to 0
		while( gpio_input_bit_get(sel_gpio_perpih, sel_gpio_pin) == RESET); //While DHT11 has line LOW
		
		timer_enable(TIMER6);
		while( gpio_input_bit_get(sel_gpio_perpih, sel_gpio_pin) == SET){	//While DHT11 has line HIGH
			if( timer_flag_get(TIMER6, TIMER_FLAG_UP) == SET ){				//If TIMER6 has expired...
				us++;
				timer_flag_clear(TIMER6, TIMER_FLAG_UP);					//Clear update flag
			}
		}
		timer_disable(TIMER6);
		
		//Check if bit 1 or 0
		uint8_t bit = (us < 30 ? 0U : 1U);
		seq = bits++ / 8;
		gath_data[seq] |= ( bit << weight );
		if( (weight -= 1) < 0 ) weight = 7;
	}

	
	if( (gath_data[0] + gath_data[1] + gath_data[2] + gath_data[3]) == gath_data[4] ){
		data.humid_integral = gath_data[0];
		data.humid_decimal = gath_data[1];
		data.temp_integral = gath_data[2];
		data.temp_decimal = gath_data[3];

		//DBG
		gpio_bit_reset(GPIOB, GPIO_PIN_9);
	}
		

	
	/**
	 * Reconfigure Selected GPIO as output
	 */
	gpio_init(sel_gpio_perpih, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, sel_gpio_pin);
	gpio_bit_set(sel_gpio_perpih, sel_gpio_pin);
}

/**
 * @brief Returns DTH11 temp in celcius. Call DTH11_readData() first
 * @return Temp val
 */
float DHT11_getTemp(void){
	return (float)(data.temp_integral + data.temp_decimal / 100);
}

/**
 * @brief Returns DTH11 humidity. Call DTH11_readData() first
 * @return Humid val
 */
uint8_t DHT11_getHumid(void){
	return data.humid_integral;
}

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
