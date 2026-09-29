#include <signal.h>

# include "cli/program_mode_selection.h"
# include "cli/eeg_source_selection.h"
# include "cli/recording_options.h"
# include "cli/helpers.h"

static volatile sig_atomic_t keepRunning = 1;
static void handleShutdown(int signum) { (void)signum; keepRunning = 0; }

int main(int argc, char *argv[])
{
# if defined(_WIN32) || defined(_WIN64)
# else
    printf("Setting GL Version overrides for Raspberry Pi...\n");
    putenv("MESA_GL_VERSION_OVERRIDE=3.3");
    putenv("MESA_GLSL_VERSION_OVERRIDE=330");
# endif

    ProgramMode mode = promptProgramModeSelection();
    printHorizontalRule();

    if (mode == STANDALONE)
    {
        runRecordingOptionSelection();
        runEEGSourceSelection();
    }

    initializeProgramGPIO(mode);
    signal(SIGINT, handleShutdown);
    signal(SIGTERM, handleShutdown);

    awaitPromptedProgramStart();

    while (keepRunning) updateProgram();

    cleanUpProgram();
    return EXIT_SUCCESS;
}