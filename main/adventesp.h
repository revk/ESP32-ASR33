// Advent stuff to work on ESP32

extern const char *revk_version;
#define	VERSION	revk_version
extern uint8_t gameover;

// Compatible functions

char *readline (const char *);
void sendline (const char *, int);
void cheese (void);
#define	TAG "advent"
