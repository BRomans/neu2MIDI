#pragma once
/**
 * LED SSVEP stimulus for the Pico 2 W.
 *
 * Same behaviour as gpio_presentation.c on the Raspberry Pi: one PWM LED per
 * stimulus frequency (sine-modulated brightness) plus one feedback LED per class
 * that lights up for a moment when an inference selects it.
 *
 * LEDs are driven from a 1 kHz hardware-timer interrupt, so the flicker stays
 * exact no matter how long the main loop blocks on EEG data or the pipeline.
 */
# include <stdbool.h>
# include <stdint.h>

# define LED_STIMULUS_COUNT 4

void initLedStimulus(const float *frequencies, uint8_t count);
void setLedStimulusEnabled(bool enabled);   // false: stimulus LEDs off (breaks, faults)
void showLedSelection(int index);           // light feedback LED `index` for a moment
void stopLedStimulus(void);