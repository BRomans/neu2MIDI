#pragma once

void enableRawKeyboardInput(void);   // keys arrive without Enter, no echo
void restoreKeyboardInput(void);
int  tryReadKey(void);               // -1 if no key waiting, never blocks