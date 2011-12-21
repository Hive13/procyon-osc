
void osc_parse(char * data, int length);
void osc_appcall(char * data, int length);
void osc_dispatch_callbacks(char * typetag, int argOffset, int maxLength);

// Example callbacks
void intEcho(int argnum, int value);
void floatEcho(int argnum, float value);
void stringEcho(int argnum, char * value, int length);
void blobEcho(int argNum, char * value, int length);

struct osc_state {
    char * addressPattern;
    void (*intCallback) (int argNum, int value);
    void (*floatCallback) (int argNum, float value);
    void (*stringCallback) (int argNum, char * value, int length);
    void (*blobCallback) (int argNum, char * value, int length);
};

struct osc_state g_osc_state;

float bigEndianFloat(char * ptr);
long bigEndianLong(char * ptr);
double bigEndianDouble(char * ptr);
void printHexDump(char * t, int bytes);
void reverseBuffer(char * src, char * dest, int size);

