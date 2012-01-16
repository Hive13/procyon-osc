

// Just don't use this.
void shiftbrite_osc_int_callback(int argNum, int value);

// Expects a blob with an entire image in it.
// Sets dot correction and pushes image when done.
void shiftbrite_osc_blob_callback(int argNum, char * value, int length);

// Call before anything else.
void shiftbrite_osc_init();
