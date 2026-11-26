# include "pico/led_stimulus.h"

# include "pico/stdlib.h"
# include "hardware/pwm.h"
# include "hardware/clocks.h"
# include <math.h>
# include <stddef.h>

// --- wiring (Pico GPIO numbers, not physical pins)
//     GP2..GP9 are free on the Pico 2 W; GP23/24/25/29 belong to the radio.
static const uint stimPins[LED_STIMULUS_COUNT]     = { 2, 3, 4, 5 };
static const uint feedbackPins[LED_STIMULUS_COUNT] = { 6, 7, 8, 9 };

# define PWM_WRAP              255        // 8-bit brightness, like PWM_RANGE on the Pi
# define PWM_CARRIER_HZ        20000.0f   // well above visible flicker
# define UPDATE_PERIOD_US      1000       // brightness updated at 1 kHz
# define SELECTION_DISPLAY_US  1000000    // feedback LED on-time

static float frequencies[LED_STIMULUS_COUNT];
static uint8_t ledCount = 0;

static uint8_t sineTable[256];                 // one period, 0..PWM_WRAP
static float phase[LED_STIMULUS_COUNT];        // 0..1, one per LED
static uint64_t lastTickUs;

static volatile bool stimulusEnabled = false;
static volatile int selectionIndex = -1;
static volatile uint32_t selectionUntilUs = 0; // 32-bit: read atomically in the ISR

static repeating_timer_t timer;

static bool onTimerTick(repeating_timer_t *t)
{
    (void)t;
    uint64_t now = time_us_64();
    float dt = (float)(now - lastTickUs) * 1e-6f;   // real elapsed time, so jitter can't shift the frequency
    lastTickUs = now;

    bool enabled = stimulusEnabled;
    for (uint8_t i = 0; i < ledCount; i++)
    {
        phase[i] += frequencies[i] * dt;
        if (phase[i] >= 1.0f) phase[i] -= (float)(int)phase[i];

        uint16_t level = enabled ? sineTable[(uint8_t)(phase[i] * 256.0f)] : 0;
        pwm_set_gpio_level(stimPins[i], level);
    }

    bool showing = (int32_t)(selectionUntilUs - (uint32_t)now) > 0;
    int selected = selectionIndex;
    for (uint8_t i = 0; i < ledCount; i++)
        gpio_put(feedbackPins[i], showing && i == selected);

    return true;   // keep repeating
}

void initLedStimulus(const float *pFrequencies, uint8_t count)
{
    ledCount = count < LED_STIMULUS_COUNT ? count : LED_STIMULUS_COUNT;
    for (uint8_t i = 0; i < ledCount; i++)
    {
        frequencies[i] = pFrequencies[i];
        phase[i] = 0.0f;
    }

    for (int i = 0; i < 256; i++)
    {
        float wave = sinf((float)i / 256.0f * 6.28318531f);
        sineTable[i] = (uint8_t)((wave + 1.0f) * 0.5f * PWM_WRAP + 0.5f);
    }

    float clkdiv = (float)clock_get_hz(clk_sys) / (PWM_CARRIER_HZ * (PWM_WRAP + 1));
    for (uint8_t i = 0; i < ledCount; i++)
    {
        gpio_set_function(stimPins[i], GPIO_FUNC_PWM);
        pwm_config config = pwm_get_default_config();
        pwm_config_set_wrap(&config, PWM_WRAP);
        pwm_config_set_clkdiv(&config, clkdiv);
        pwm_init(pwm_gpio_to_slice_num(stimPins[i]), &config, true);  // two pins share a slice: same config, harmless
        pwm_set_gpio_level(stimPins[i], 0);

        gpio_init(feedbackPins[i]);
        gpio_set_dir(feedbackPins[i], GPIO_OUT);
        gpio_put(feedbackPins[i], 0);
    }

    lastTickUs = time_us_64();
    add_repeating_timer_us(-UPDATE_PERIOD_US, onTimerTick, NULL, &timer);  // negative: fixed period between starts
}

void setLedStimulusEnabled(bool enabled) { stimulusEnabled = enabled; }

void showLedSelection(int index)
{
    if (index < 0 || index >= ledCount) return;
    selectionIndex = index;
    selectionUntilUs = time_us_32() + SELECTION_DISPLAY_US;
}

void stopLedStimulus(void)
{
    cancel_repeating_timer(&timer);
    for (uint8_t i = 0; i < ledCount; i++)
    {
        pwm_set_gpio_level(stimPins[i], 0);
        gpio_put(feedbackPins[i], 0);
    }
}