// Advent stuff to work on ESP32

extern	const char *revk_version;
#define	VERSION	revk_version
extern uint8_t gameover;

// Compatible functions

char	*readline(const char *);
void	add_history(const char *);
void	sendline(const char *);
#define	TAG "advent"
