
void control_hware(void);
void set_hardware_rx_gain(void);
void set_hardware_rx_frequency(void);
void set_hardware_tx_frequency(void);

extern int hware_flag;
extern char hware_error_flag;
extern double hware_time;
extern int wse_parport_status;
extern int wse_parport_control;
extern int wse_parport_ack;
extern int wse_parport_ack_sign;


#if (LUSERS_DEF == 1 && OSNUM == OSNUM_LINUX)
#include "users_hwaredef.h"
#else
#if (WUSERS_DEF == 1 && OSNUM == OSNUM_WINDOWS)
#include "wusers_hwaredef.h"
#else





// Give each hardware unit its own unique number.
// With few units, make these numbers powers of two to make
// address decoding unnecessary.
#define RX10700 16
#define RX70 32
#define RX144 8
#define RXHFA 2
#define RXHFA_GAIN 1
#define TX10700 4
#define TX70 64
// These two tx units may not be used simultaneously
// with this definition:
#define TX144 128
#define TXHFA 128

// ******************************************************************
// The serial interface to the WSE hardware units uses an acknowledge
// pin so the user will gety a warning in case something goes wrong.
// In case the parallel port is connected to the radio hardware
// before the chassis is properly connected to ground for both the
// computer and the radio hardware, control port inputs may be
// permanently damaged. In such cases, be more careful next time
// and use the below defines to select another input pin.
// 
// There are five control input pins. 
// Make one of them in the below list uncommented:
//

// Pin 10 of the status port has a pull up resistor in the PC.
// In case you use one of the other pins, connect it in parallel
// with pin 10 to make use of the pull up resistor (or add an external
// resistor to +5V)

// These parameters define the frequency control window.
#define FREQ_MHZ_DECIMALS 3
#define FREQ_MHZ_DIGITS 4
#define FREQ_MHZ_ROUNDCORR (0.5*pow(10,-FREQ_MHZ_DECIMALS))
#define FG_HSIZ ((FREQ_MHZ_DECIMALS+FREQ_MHZ_DIGITS+6)*text_width)
#define FG_VSIZ (3*text_height)

// *******************************************
// Radio hardware control is through the parallel port.
// Each hardware unit is controlled by serial data that is clocked
// into a shift register.
// The data in the shift register is transferred to a latch after
// the complete word has been transferred.
// The 8 bits of the output (data) port are used to select
// a hardware unit.
// If all 8 bits are zero, no unit is selected.
// In a small system with maximum 8 units, the 8 data
// pins can be used directly to select one unit each.
// By decoding the 8 bits one can select up to 255 units.
// The number of wires would become impractical and some
// other communication is recommended.
// These are the data pins on the 25 pin d-sub:
//   2  =  bit0
//   3  =  bit1
//   4  =  bit2
//   5  =  bit3
//   6  =  bit4
//   7  =  bit5
//   8  =  bit6
//   9  =  bit7
// These are the control port output pins:
//   1  =  bit0 = Strobe (output)
//  14  =  bit1 = Auto Feed (output)
//  16  =  bit2 = Initialize Paper (output)
//  17  =  bit3 = -Select Input (output)

// These are the status port input pins:
//  10  =  bit7 = Acknowledge (input)
//  11  =  bit6 = Busy (input)
//  12  =  bit5 = P.End (input)
//  13  =  bit4 = IRQ (input)
//  15  =  bit3 = Error (input)

// Two output pins of the control port are used to clock serial 
// data into the selected unit and one output pin is used for
// the transmit/receive switching.
// "Strobe" = pin 1 is clock.
// "Select input" = pin 17 is the serial data.
// "Auto Feed" = pin 14 is the transmit/receive control pin.

#define BIT0 1
#define BIT1 2
#define BIT2 4
#define BIT3 8
#define BIT4 16
#define BIT5 32
#define BIT6 64
#define BIT7 128


#define WSE_PARPORT_CLOCK BIT0
#define WSE_PARPORT_DATA BIT3
#define WSE_PARPORT_PTT BIT1
#define WSE_PARPORT_MORSE_KEY BIT4

#endif
#endif

