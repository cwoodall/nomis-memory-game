/**
 * Project: Memory Game
 * Version: 01
 * Creator(s): Christopher Woodall
 * License: MIT License
 * 
 * Memory Game is a Simon (tm) clone based around an ATTiny85 microprocessor.
 * The game generates a "random" string of moves for the player to copy.
 * If the player is unable to remember the string and presses a wrong button,
 * then the game is over and the player looses.
 *
 * The random numbers are generated using a Linear Congruential Generator (LCG),
 * which functions based on the formula X_next = (a*X_n + c) mod m. The 
 * selection of a, c and m are very important for generating a series of
 * psuedo-random numbers with the maximum period of m. Selection is outlined
 * inside of the rand_lcg() function
 */
#define F_CPU 1000000 /* 1MHz Internal Oscillator */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#define MAX_PERIOD 32768 // 2^15
#define MULTIPLIER 513   // 2^9 + 1 (A-1 is divisible by all prime factors of M)
#define C          1     // We know 1 is relatively prime with M
#define MAX_MOVES  100   // Maximum number of moves

uint8_t led_display(uint8_t state);
uint16_t rand_lcg(uint16_t lcg_previous, uint16_t m, uint16_t a, uint16_t c);

int main (void)
{
    // Current game state... Create an enum for better state names
    //  uint8_t state = 0x00;
    
    // Setup random number generator
    // Need a better way to obtain a seed (maybe noise off a zener diode?
    uint16_t random = eeprom_read_word((uint16_t *) 46); // seed by placing whatevr junk is already in random into random 

    uint8_t moves[MAX_MOVES]; // Previous move the game makes
    uint8_t player_moves[MAX_MOVES]; // Array where use moves are written
    DDRB = 0x07; // Set up pins 0, 1 and 2 to be outputs
    PORTB = 0x00; // Set all pins off by default
    
    uint16_t counter = 0; // main counter
    uint16_t i; // internal counter
    while (1) {
        // Get a new random number from the lcg
        random = rand_lcg(random, MAX_PERIOD, MULTIPLIER, C ); 
        eeprom_write_word((uint16_t *)46, random); // Store last random value in the EEPROM for next seed, if reset occurs
        // Store this move into memory. First shift rand over and only take the
        // two most significant bits.
        moves[counter] = 0x01 << (random >> 13);
        
        
        for(i = 0; i <= counter; i++) {
            PORTB = led_display(moves[i]); // Translate the state encoding to something 
            // our charlieplexed LED system understands
            _delay_ms(500);
            PORTB = 0x00;
            _delay_ms(100);
        }
        counter += 1;
        _delay_ms(1000);
    }
    return 0;
}


/**
 * led_display()
 * \param   uint8_t  state  The 4-bit one hot encoding of the current game move.
 * \return  uint8_t  encoded_state  The encodign for our charlieplexed LED display.
 *
 * \breif Converts a 4-but one hot encoded state into an encoding appropriate for the
 *        charlieplexed LED display on PORTB pins 0, 1 and 2.
 *
 * LEDS, MUST be on pins 0, 1 and 2 on any PORT. However, this can be changed by
 * changing the encodings.
 */
uint8_t led_display(uint8_t state)
{
    switch (state) {
    case 0x01:
        return 0x02;
        break;
    case 0x02:
        return 0x05;
        break;
    case 0x04:
        return 0x04;
        break;
    case 0x08:
        return 0x03;
        break;
    default:
        return 0x00;
        break;
    }
}

// rand_lcg generates a random number from some set of parameters, where the result
// is constantly fedback into the function when a new random number is desired. 
// Needs some initial seed value.
//
// m, a and c need to be chosen carefully
uint16_t rand_lcg(uint16_t lcg_previous, uint16_t m, uint16_t a, uint16_t c)
{
    return (lcg_previous*a + c) % m;
}
