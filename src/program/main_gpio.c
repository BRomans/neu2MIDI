# include <signal.h>

# include "cli/program_mode_selection.h"   // only for the ProgramMode type
# include "cli/eeg_source_selection.h"
# include "cli/recording_options.h"
# include "cli/helpers.h"

static volatile sig_atomic_t keepRunning = 1;
static void handleShutdown(int signum) { (void)signum; keepRunning = 0; }

int main(void)
{
    runRecordingOptionSelection();
    runEEGSourceSelection();

    initializeProgramGPIO(STANDALONE);
    signal(SIGINT, handleShutdown);
    signal(SIGTERM, handleShutdown);

    awaitPromptedProgramStart();

    while (keepRunning) updateProgram();

    cleanUpProgram();
    return EXIT_SUCCESS;
}