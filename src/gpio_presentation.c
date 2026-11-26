# include "presentation.h"
# include <pigpio.h>
# include <math.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#include <_time.h>

# define STIM_LED_COUNT 4
# define FEEDBACK_LED_COUNT 4
# define PWM_RANGE 255
# define PWM_FREQUENCY_HZ 1000

static const int stimPins[STIM_LED_COUNT]         = {6, 27, 22, 23};
static const int feedbackPins[FEEDBACK_LED_COUNT] = { 5,  4, 24, 25};

static float *frequencies;
static uint16_t frequencyCount;

static uint16_t targetIndex;
static bool hasTarget = false;

static uint16_t selectionIndex;
static double selectionTime = -SELECTION_DISPLAY_TIME;

static bool stimulusEnabled = true;

static double nowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void initializePresentation(const float *pFrequencies, uint16_t pFrequencyCount)
{
    if (pFrequencyCount != STIM_LED_COUNT)
    {
        fprintf(stderr, "Expected %d frequencies for %d stim LEDs, got %u\n",
                STIM_LED_COUNT, STIM_LED_COUNT, pFrequencyCount);
        exit(EXIT_FAILURE);
    }

    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "pigpio init failed - run as root (or with the right permissions)\n");
        exit(EXIT_FAILURE);
    }

    frequencyCount = pFrequencyCount;
    frequencies = malloc(frequencyCount * sizeof(float));
    memcpy(frequencies, pFrequencies, frequencyCount * sizeof(float));

    for (int i = 0; i < STIM_LED_COUNT; i++)
    {
        gpioSetMode(stimPins[i], PI_OUTPUT);
        gpioSetPWMrange(stimPins[i], PWM_RANGE);
        gpioSetPWMfrequency(stimPins[i], PWM_FREQUENCY_HZ);
        gpioPWM(stimPins[i], 0);
    }
    for (int i = 0; i < FEEDBACK_LED_COUNT; i++)
    {
        gpioSetMode(feedbackPins[i], PI_OUTPUT);
        gpioWrite(feedbackPins[i], 0);
    }
}

// ---

void setPresentationTarget(uint16_t index) { targetIndex = index; hasTarget = true; }
void clearPresentationTarget(void)         { hasTarget = false; }
// no visual target indicator on the LED rig - these exist only so callers compile unchanged

// ---

void displaySelection(uint16_t index)
{
    selectionIndex = index;
    selectionTime = nowSeconds();
}

static void updateSelectionFeedback(void)
{
    bool showing = (nowSeconds() <= selectionTime + SELECTION_DISPLAY_TIME);
    for (int i = 0; i < FEEDBACK_LED_COUNT; i++)
        gpioWrite(feedbackPins[i], (showing && i == selectionIndex) ? 1 : 0);
}

// ---

void drawMessageScreen(const char *message)
{
    printf("%s\n", message);
}

void drawPreparationScreen(const char *message)
{
    printf("%s\n", message);
}

void drawStimulusScreen(void)
{
    double t = nowSeconds();
    for (int i = 0; i < STIM_LED_COUNT; i++)
    {
        if (!stimulusEnabled) { gpioPWM(stimPins[i], 0); continue; }
        double wave = sin(frequencies[i] * TAU * t);
        int duty = (int)((wave + 1.0) / 2.0 * PWM_RANGE);
        gpioPWM(stimPins[i], duty);
    }
    updateSelectionFeedback();
}

void pauseStimulus(void)  { stimulusEnabled = false; }
void resumeStimulus(void) { stimulusEnabled = true;  }

// ---

void stopPresentation(void)
{
    for (int i = 0; i < STIM_LED_COUNT; i++) gpioPWM(stimPins[i], 0);
    for (int i = 0; i < FEEDBACK_LED_COUNT; i++) gpioWrite(feedbackPins[i], 0);
    free(frequencies);
    gpioTerminate();
}