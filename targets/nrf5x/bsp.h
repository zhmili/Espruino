#include "nrf_gpio.h"

#define LEDS_ON(x) nrf_gpio_pin_write(17, 0);
#define LEDS_OFF(x) nrf_gpio_pin_write(17, 1);
#define LEDS_CONFIGURE(x) nrf_gpio_cfg_output(17);

#define TX_PIN_NUMBER 6
#define RX_PIN_NUMBER 8
#define RTS_PIN_NUMBER 5
#define CTS_PIN_NUMBER 7
