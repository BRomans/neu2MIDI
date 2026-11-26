# b2MIDI
This is a fork of tinyBCI Presenter.
b2MIDI is a brain-controlled application that maps neural outputs
to a customizable midi-controller.

It runs a TinyBCI SSVEP pipeline on EEG data and
can present the stimuli on a screen (raylib), on LEDs driven by a Raspberry Pi's
GPIO, or leave presentation to a separate app and run headless.

## Targets

| Target | What it is | Runs on |
|---|---|---|
| `b2MIDI` | Standalone app with on-screen stimuli (raylib) | macOS, Linux, Windows, Raspberry Pi |
| `gpio_b2MIDI` | Standalone app with LED stimuli via GPIO (pigpio) | Raspberry Pi Zero / 4 (mock on desktop) |
| `headless_b2MIDI` | BCI engine only, no stimuli; talks to a presenter over LSL | everywhere |
| `ssvep_box_b2MIDI` | Console experiment for an external stimulus box, with LSL markers | everywhere |
| `headless_b2MIDI_pico` | Headless engine firmware for the Raspberry Pi Pico 2 W | Pico 2 W |

All desktop executables are written to `bin/`.

## Build

To build the project as desktop software you need CMake ≥ 3.26 and a C/C++ compiler
(Xcode command line tools on macOS, GCC on Linux, Visual Studio or MinGW on Windows).
LSL, raylib and TinyBCI are downloaded automatically on the first configure.

Run the command
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To build a single target:
```sh
cmake --build build --target gpio_b2MIDI
```

On Linux (including Raspberry Pi OS), raylib needs the X11/OpenGL development packages:
```sh
sudo apt install libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### Build options

| Option | Default | Effect |
|---|---|---|
| `TBCI_PICO` | `OFF` | Build the Pico 2 W firmware instead of the desktop targets |
| `TBCI_MEASURE` | `OFF` | Add tick logging and GPIO timing markers to `headless_b2MIDI` (not on Windows) |
| `TBCI_USE_PIGPIO` | auto | Use the real pigpio for `gpio_b2MIDI`. `ON` when pigpio is found, otherwise the mock is used |

Pass them at configure time, e.g. `cmake -B build -DTBCI_MEASURE=ON`.

### Desktop Target
`b2MIDI` assumes a desktop environment (a display and OpenGL 3.3) and builds raylib
from source. At start it asks for a program mode:

- **Standalone**: connects to the EEG headset and runs the pipeline itself.
- **Presentation Only**: shows the stimuli and connects to a `headless_b2MIDI` engine over LSL.
- **Presentation Only With Serial**: shows the stimuli and connects to a TinyBCI engine
  (e.g. the Pico) over USB serial.

Press **Space** to start the trials.

### GPIO Target
`gpio_b2MIDI` drives the stimuli on LEDs instead of a screen, always in standalone mode.
It uses BCM pin numbers:

| LED | Pins |
|---|---|
| Stimulus (PWM) | 6, 27, 22, 23 |
| Selection feedback | 5, 4, 24, 25 |

On a Raspberry Pi install pigpio before configuring, and run the program as root
(pigpio needs direct access to the GPIO hardware):
```sh
sudo apt install libpigpio-dev
sudo systemctl stop pigpiod     # if the daemon is running, it holds the GPIO
sudo ./bin/gpio_b2MIDI
```

On macOS, Windows or a Linux PC without pigpio, the target builds against a mock
(`src/mock/`) that simulates the GPIO calls, so you can run and debug the full program
without hardware. Set `PIGPIO_MOCK_LOG=1` to print every pin change.

Stimulus frequencies and endless mode are set per target in `CMakeLists.txt`:
```cmake
target_compile_definitions(gpio_b2MIDI PRIVATE
        N_FREQS=4
        FREQUENCIES=4.5f,5.0f,5.5f,6.0f
        ENDLESS_MODE=1
)
```
Do not put braces around the `FREQUENCIES` list. On macOS the build shell expands
`{a,b,c}` into separate definitions and the build fails with "macro redefined".
The code adds the braces itself.

Press **Space** in the terminal to start. Keys are read from the terminal, so when
running from CLion enable *Run → Edit Configurations → Emulate terminal in the output console*,
otherwise every key needs Enter.

### Headless Target
The headless target automatically searches for an LSL stream from the presenter.
Start `headless_b2MIDI` on the machine connected to the headset, then start `b2MIDI`
in *Presentation Only* mode on the machine showing the stimuli. Triggers flow from the
presenter to the engine, inferences flow back.

### SSVEP Box Target
`ssvep_box_b2MIDI` runs a fixed experiment for an external stimulus box: one 30-second
trial per frequency in random order. Press **S** to start each trial and **Q** to quit.
Trial events are published on the LSL stream `Tiny_BCI_Experiment_Markers`.

## Build for Pico
To build the project for Raspberry Pico, you first need to copy the `pico-sdk` inside
the root folder of the project, with its submodules (btstack, cyw43-driver, tinyusb):
```sh
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk && git submodule update --init && cd ..
```
You also need the ARM toolchain (`arm-none-eabi-gcc`), e.g. `brew install --cask gcc-arm-embedded`
on macOS or `sudo apt install gcc-arm-none-eabi` on Linux.

The headset name is set in `src/program/main_headless_pico.c` (`UNICORN_NAME`).
Change it to match your Unicorn before building.

Then run:
```sh
cmake -B build-pico -DTBCI_PICO=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-pico --target headless_b2MIDI_pico
```

To flash, hold **BOOTSEL** while plugging in the Pico and copy
`headless_b2MIDI_pico.uf2` onto the drive that appears.

Use a separate build folder for the Pico (`build-pico`). The Pico SDK switches the
whole project to the ARM toolchain, so it cannot share a build folder with the
desktop targets.