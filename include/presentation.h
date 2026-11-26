# pragma once

# define SELECTION_DISPLAY_TIME 0.5f

# define TAU 6.28318530717958647692528676655900576839433879875021

void initializePresentation(const float *pFrequencies, uint16_t pFrequencyCount);
void drawPreparationScreen(const char *message);
void drawMessageScreen(const char *message);
void drawStimulusScreen(void);
void stopPresentation(void);

void pauseStimulus(void);
void resumeStimulus(void);

void disableTextureStimulus(void);
void enableTextureStimulus(void);

void setPresentationTarget(uint16_t index);
void clearPresentationTarget(void);
void displaySelection(uint16_t index);

bool isStartKeyPressed(void);
bool shouldClosePresentation(void);