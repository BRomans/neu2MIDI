#ifndef TBCI_PIGPIO_MOCK_H
#define TBCI_PIGPIO_MOCK_H
/* Desktop stand-in for pigpio: same API subset, no hardware.
 * Only used when TBCI_USE_PIGPIO=OFF (see CMakeLists.txt). */
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define PI_INPUT   0
#define PI_OUTPUT  1
#define PI_ALT0    4
#define PI_ALT1    5
#define PI_ALT2    6
#define PI_ALT3    7
#define PI_ALT4    3
#define PI_ALT5    2

#define PI_OFF  0
#define PI_ON   1
#define PI_LOW  0
#define PI_HIGH 1

#define PI_PUD_OFF  0
#define PI_PUD_DOWN 1
#define PI_PUD_UP   2

#define PI_TIME_RELATIVE 0
#define PI_TIME_ABSOLUTE 1

#define PI_MAX_GPIO 53

#define PI_INIT_FAILED     -1
#define PI_BAD_GPIO        -3
#define PI_BAD_MODE        -4
#define PI_BAD_LEVEL       -5
#define PI_BAD_PUD         -6
#define PI_BAD_DUTYCYCLE   -8
#define PI_NOT_INITIALISED -31

typedef void (*gpioAlertFunc_t)(int gpio, int level, uint32_t tick);
typedef void (*gpioSignalFunc_t)(int signum);

int      gpioCfgClock(unsigned micros, unsigned peripheral, unsigned source);
int      gpioInitialise(void);
void     gpioTerminate(void);

int      gpioSetMode(unsigned gpio, unsigned mode);
int      gpioGetMode(unsigned gpio);
int      gpioSetPullUpDown(unsigned gpio, unsigned pud);
int      gpioRead(unsigned gpio);
int      gpioWrite(unsigned gpio, unsigned level);

int      gpioPWM(unsigned gpio, unsigned dutycycle);
int      gpioGetPWMdutycycle(unsigned gpio);
int      gpioSetPWMrange(unsigned gpio, unsigned range);
int      gpioSetPWMfrequency(unsigned gpio, unsigned frequency);
int      gpioHardwarePWM(unsigned gpio, unsigned freq, unsigned duty);

uint32_t gpioTick(void);
uint32_t gpioDelay(uint32_t micros);
int      gpioSleep(unsigned timetype, int seconds, int micros);
double   time_time(void);
void     time_sleep(double seconds);

int      gpioSetAlertFunc(unsigned gpio, gpioAlertFunc_t f);
int      gpioSetSignalFunc(unsigned signum, gpioSignalFunc_t f);

/* Mock-only: inspect pin state */
int      pigpioMockGetLevel(unsigned gpio);
int      pigpioMockGetPWM(unsigned gpio);

#ifdef __cplusplus
}
#endif
#endif