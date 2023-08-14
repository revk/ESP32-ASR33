// UART low level

#define port_mask(p) ((p)&0x3FFF) // Get GPIO from io()
#define port_inv(p) (((p)&0x4000)?1:0)    // Get if inverted

void tty_setup(void);
void tty_flush(void);
int tty_rx_ready(void);
void tty_tx(uint8_t b);
void tty_break(uint8_t chars);
uint8_t tty_rx(void);
int tty_tx_space(void);
int tty_tx_waiting(void);
void tty_xoff(void);
void tty_xon(void);
void tty_stats(softuart_stats_t *);
