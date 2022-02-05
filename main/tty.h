// UART low level

#define port_mask(p) ((p)&0x3F) // Get GPIO from io()
#define port_inv(p) (((p)&0x40)?1:0)    // Get if inverted

void tty_setup(void);
void tty_flush(void);
int tty_rx_ready(void);
void tty_tx(uint8_t b);
void tty_break(void);
uint8_t tty_rx(void);
int tty_tx_space(void);
int tty_tx_waiting(void);
