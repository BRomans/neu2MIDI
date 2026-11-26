#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS   /* getenv() would trip /WX on MSVC */
#endif

#include "../../include/pigpio.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <windows.h>
#else
  #include <time.h>
  #include <errno.h>
#endif

#define NGPIO (PI_MAX_GPIO + 1)

static int             s_init, s_log;
static unsigned        s_mode[NGPIO];
static volatile int    s_level[NGPIO];
static unsigned        s_pwm_duty[NGPIO], s_pwm_range[NGPIO], s_pwm_freq[NGPIO];
static gpioAlertFunc_t s_alert[NGPIO];

#define LOG(...) do { if (s_log) { fprintf(stderr, "[pigpio-mock] " __VA_ARGS__); fputc('\n', stderr); } } while (0)
#define CHECK_GPIO(g) do { if ((g) > PI_MAX_GPIO) return PI_BAD_GPIO; } while (0)

/* ---- clocks ---------------------------------------------------------- */

static uint64_t now_us(void)
{
#if defined(_WIN32)
    static LARGE_INTEGER f;
    LARGE_INTEGER c;
    if (!f.QuadPart) QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (uint64_t)(c.QuadPart / f.QuadPart) * 1000000u
         + (uint64_t)(c.QuadPart % f.QuadPart) * 1000000u / (uint64_t)f.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
#endif
}

static void sleep_us(uint64_t us)
{
#if defined(_WIN32)
    /* Sleep() is ~1-15 ms coarse: sleep most of it, spin the tail. */
    uint64_t end = now_us() + us;
    if (us > 2000) Sleep((DWORD)((us - 2000) / 1000));
    while (now_us() < end) { /* spin */ }
#else
    struct timespec ts = { (time_t)(us / 1000000u), (long)(us % 1000000u) * 1000L };
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) { }
#endif
}

static void set_level(unsigned g, int lvl)
{
    if (s_level[g] != lvl)
    {
        s_level[g] = lvl;
        if (s_alert[g]) s_alert[g]((int)g, lvl, gpioTick());
    }
}

/* ---- lifecycle ------------------------------------------------------- */

int gpioCfgClock(unsigned micros, unsigned peripheral, unsigned source)
{
    (void)micros; (void)peripheral; (void)source;
    return 0;
}

int gpioInitialise(void)
{
    if (!s_init)
    {
        const char *e = getenv("PIGPIO_MOCK_LOG");
        s_log = e && *e && *e != '0';
        for (int i = 0; i < NGPIO; ++i) { s_pwm_range[i] = 255; s_pwm_freq[i] = 800; }
        s_init = 1;
        fprintf(stderr, "[pigpio-mock] GPIO is simulated (PIGPIO_MOCK_LOG=1 to trace calls)\n");
    }
    return 79; /* real pigpio returns its version */
}

void gpioTerminate(void) { s_init = 0; LOG("terminate"); }

/* ---- basic IO -------------------------------------------------------- */

int gpioSetMode(unsigned g, unsigned m)
{
    CHECK_GPIO(g);
    if (m > 7) return PI_BAD_MODE;
    s_mode[g] = m;
    LOG("mode    gpio%u = %u", g, m);
    return 0;
}

int gpioGetMode(unsigned g) { CHECK_GPIO(g); return (int)s_mode[g]; }

int gpioSetPullUpDown(unsigned g, unsigned pud)
{
    CHECK_GPIO(g);
    if (pud > PI_PUD_UP) return PI_BAD_PUD;
    if (s_mode[g] == PI_INPUT) set_level(g, pud == PI_PUD_UP);
    return 0;
}

int gpioRead(unsigned g) { CHECK_GPIO(g); return s_level[g]; }

int gpioWrite(unsigned g, unsigned l)
{
    CHECK_GPIO(g);
    if (l > 1) return PI_BAD_LEVEL;
    s_mode[g] = PI_OUTPUT;
    s_pwm_duty[g] = 0;
    if (s_level[g] != (int)l) LOG("write   gpio%u = %u", g, l);   /* log changes only */
    set_level(g, (int)l);
    return 0;
}

/* ---- PWM ------------------------------------------------------------- */

int gpioPWM(unsigned g, unsigned duty)
{
    CHECK_GPIO(g);
    if (duty > s_pwm_range[g]) return PI_BAD_DUTYCYCLE;
    s_mode[g] = PI_OUTPUT;
    s_pwm_duty[g] = duty;
    set_level(g, duty > 0);
    return 0;   /* not logged: called every frame by the stimulus loop */
}

int gpioGetPWMdutycycle(unsigned g) { CHECK_GPIO(g); return (int)s_pwm_duty[g]; }

int gpioSetPWMrange(unsigned g, unsigned r)
{
    CHECK_GPIO(g);
    s_pwm_range[g] = r;
    LOG("pwmrange gpio%u = %u", g, r);
    return (int)r;
}

int gpioSetPWMfrequency(unsigned g, unsigned f)
{
    CHECK_GPIO(g);
    s_pwm_freq[g] = f;
    LOG("pwmfreq gpio%u = %u Hz", g, f);
    return (int)f;
}

int gpioHardwarePWM(unsigned g, unsigned freq, unsigned duty)
{
    CHECK_GPIO(g);
    if (duty > 1000000u) return PI_BAD_DUTYCYCLE;
    s_pwm_freq[g] = freq;
    s_pwm_duty[g] = duty;
    set_level(g, duty > 0);
    LOG("hwpwm   gpio%u = %u Hz, duty %u/1e6", g, freq, duty);
    return 0;
}

/* ---- timing ---------------------------------------------------------- */

uint32_t gpioTick(void) { return (uint32_t)now_us(); }

uint32_t gpioDelay(uint32_t micros)
{
    uint32_t t0 = gpioTick();
    sleep_us(micros);
    return gpioTick() - t0;
}

int gpioSleep(unsigned timetype, int seconds, int micros)
{
    (void)timetype; /* absolute treated as relative in the mock */
    sleep_us((uint64_t)seconds * 1000000u + (uint64_t)micros);
    return 0;
}

double time_time(void)        { return (double)now_us() / 1e6; }
void   time_sleep(double s)   { if (s > 0) sleep_us((uint64_t)(s * 1e6)); }

/* ---- callbacks ------------------------------------------------------- */

int gpioSetAlertFunc(unsigned g, gpioAlertFunc_t f) { CHECK_GPIO(g); s_alert[g] = f; return 0; }
int gpioSetSignalFunc(unsigned signum, gpioSignalFunc_t f) { (void)signum; (void)f; return 0; }

/* ---- mock-only ------------------------------------------------------- */

int pigpioMockGetLevel(unsigned g) { return g > PI_MAX_GPIO ? 0 : s_level[g]; }
int pigpioMockGetPWM(unsigned g)   { return g > PI_MAX_GPIO ? 0 : (int)s_pwm_duty[g]; }