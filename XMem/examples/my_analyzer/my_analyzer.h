
#ifndef logic_h
#define logic_h
/*
 * Should we use PORTD or PORTB?  (default is PORTB)
 * PORTD support with triggers seems to work but needs more testing.
 */
//#define USE_PORTD 1

/*
 * Arduino device profile:      ols.profile-agla.cfg
 * Arduino Mega device profile: ols.profile-aglam.cfg
 */
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define CHANPIN PINL
#define CHAN0 49
#define CHAN1 48
#define CHAN2 47
#define CHAN3 46
#define CHAN4 45
#define CHAN5 44
#define CHAN6 43
#define CHAN7 42
/*
#define CHANPIN PINA
#define CHAN0 22
#define CHAN1 23
#define CHAN2 24
#define CHAN3 25
#define CHAN4 26
#define CHAN5 27
#define CHAN6 28
#define CHAN7 29
*/
#else
#if defined(USE_PORTD)
#define CHANPIN PIND
#define CHAN0 2
#define CHAN1 3
#define CHAN2 4
#define CHAN3 5
#define CHAN4 6
#define CHAN5 7
#else
#define CHANPIN PINB
#define CHAN0 8
#define CHAN1 9
#define CHAN2 10
#define CHAN3 11
#define CHAN4 12
/* Comment out CHAN5 if you don't want to use the LED pin for an input */
#define CHAN5 13
#endif /* USE_PORTD */
#endif
#define ledPin 13

/* XON/XOFF are not supported. */
#define SUMP_RESET 0x00
#define SUMP_ARM   0x01
#define SUMP_QUERY 0x02
#define SUMP_XON   0x11
#define SUMP_XOFF  0x13

/* mask & values used, config ignored. only stage0 supported */
#define SUMP_TRIGGER_MASK 0xC0
#define SUMP_TRIGGER_VALUES 0xC1
#define SUMP_TRIGGER_CONFIG 0xC2

/* Most flags (except RLE) are ignored. */
#define SUMP_SET_DIVIDER 0x80
#define SUMP_SET_READ_DELAY_COUNT 0x81
#define SUMP_SET_FLAGS 0x82
#define SUMP_SET_RLE 0x0100

/* extended commands -- self-test unsupported, but metadata is returned. */
#define SUMP_SELF_TEST 0x03
#define SUMP_GET_METADATA 0x04

/* ATmega168:  532 (or lower)
 * ATmega328:  1024 (or lower)
 * ATmega2560: 7168 (or lower)
 */
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define DEBUG_CAPTURE_SIZE 7168
#define CAPTURE_SIZE 7168
#elif defined(__AVR_ATmega328P__)
#define DEBUG_CAPTURE_SIZE 1024
#define CAPTURE_SIZE 1024
#else
#define DEBUG_CAPTURE_SIZE 532
#define CAPTURE_SIZE 532
#endif

#ifdef USE_PORTD
#define DEBUG_ENABLE DDRB = DDRB | B00000001
#define DEBUG_ON PORTB = B00000001
#define DEBUG_OFF PORTB = B00000000
#else
#define DEBUG_ENABLE DDRD = DDRD | B10000000
#define DEBUG_ON PORTD = B10000000
#define DEBUG_OFF PORTD = B00000000
#endif /* USE_PORTD */

#define DEBUG

#ifdef DEBUG
#define MC_SIZE DEBUG_CAPTURE_SIZE
#else
#define MC_SIZE CAPTURE_SIZE
#endif /* DEBUG */

/*
 * SUMP command from host (via serial)
 * SUMP commands are either 1 byte, or for the extended commands, 5 bytes.
 */

#define byte uint8_t
#endif /* logic_h */