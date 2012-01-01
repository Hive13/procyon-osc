
// Send a command to the ShiftBrite.
// Only the bottom 2 bits of 'cmd' are used, and only valid values are 0 for
// dot control registers and 1 for sending RGB data.
// Toggling the latch (e.g. via shiftbrite_latch), and waiting the appropriate
// time to do so (as p9 of the A6281 datasheet says with t_su) are still your
// responsibility.
void shiftbrite_command(int cmd, int red, int green, int blue);

// Toggle the latch - setting it high for about 1 usec, and then setting it
// back low.
void shiftbrite_latch(void);

