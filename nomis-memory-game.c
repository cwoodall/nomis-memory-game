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
#include <avr/interrupt.h>

#define MAX_PERIOD 32768 // 2^15
#define MULTIPLIER 513   // 2^9 + 1 (A-1 is divisible by all prime factors of M)
#define C          1     // We know 1 is relatively prime with M
#define MAX_MOVES  100   // Maximum number of moves

#define clear_display() PORTB &= 0xF0;
#define set_display(state) PORTB |= led_display(state);

/**
 * enum STATE gamestates: IDLE, CPU, PLAYER, LOSE
 *  
 * \breif Define state names for the finite state machine that runs the game flow.
 *         
 * IDLE: The MCU waits for an input from the user to play the game. In this state
 *       the MCU cascades the LEDS, signifying that it is on and ready to go. While
 *       this is happening the MCU is incrementing the random variable to
 *       continuously change the seed. Exits into the CPU state if a button is
 *       pressed.
 *
 * CPU: The CPU's turn. The CPU (well MCU) generates a random number and,
 *        adds that as a move to the moves[] array. Then the CPU returns the game
 *        to the PLAYER state. Ignores input from player.
 * 
 * PLAYER: The player's turn, in this state the MCU will wait for a string of 
 *           inputs from the player. Once the Player presses a button that is not
 *           in the array moves[], the player looses the game and the game goes to
 *           the LOSE state. Otherwise the game goes to the CPU state and the 
 *           computer adds another move to the moves array.
 *
 * LOSE: Cleans up the game and returns to the IDLE state. Just for house keeping.
 */
enum STATE {
    IDLE,
    CPU,
    PLAYER,
    LOSE,
} gamestates;

/** Function Headers */
uint16_t read_adc();
uint8_t led_display(uint8_t state);
uint16_t rand_lcg(uint16_t lcg_previous, uint16_t m, uint16_t a, uint16_t c);
void cascade_leds();
void blink_leds();
uint8_t get_player_move();

enum STATE gamestate = IDLE;

int main (void)
{
    /**
     * \var  uint8_t   moves  tracks the computers moves which the player 
     *                          must match.
     *
     * \var  uint8_t   player_moves  tracks the players moves, gets reset after 
     *                                 every turn.
     *
     * \var  uint16_t  i  predefine looping variable for future loops
     * 
     * \var  uint16_t  counter  Generic counting variable, used to count the number
     *                            of moves the computer has made, and the player 
     *                            has made.
     *
     * \var  uint16_t  random  Seed random with whatever junk is in the eeprom 
     *                           region at 46. We store future random generations
     *                           there to be used as future seeds, on future 
     *                           bootup or resets.
     *
     * \var  STATE  gamestate  Keep track of where the game is. 4 possible states,
     *                           IDLE (0), CPU (1), PLAYER (2), or LOSE (4).
     */
    uint8_t moves[MAX_MOVES]; 
    uint16_t i; 
    uint16_t cpu_counter = 0; 
    uint16_t player_counter = 0;
    uint16_t random = eeprom_read_word((uint16_t *) 46);
    uint16_t player_move;
    //    enum STATE gamestate = CPU;
    // Set up PortB pins 0, 1, and 2 to be outputs.
    DDRB = 0x07;
    // Set pull down resistors and all pins off.
    PORTB = 0x00;
   
    // Setup the ADC

    // Select ADC2
    ADMUX = 0b00000010; 
    // ADCSRA[7]: Set ADEN on.
    // ADCSRA[2:0]: Set to 011 for a divide by 8 clock division. 
    //                (125 kHz ADC clock)
    ADCSRA = 0b10000011;

    // And now the games begin!
    while (1) {
        if (gamestate == CPU) {
            // Get a new random number from the lcg
            random = rand_lcg(random, MAX_PERIOD, MULTIPLIER, C ); 
            eeprom_write_word((uint16_t *)46, random); // Store last random value in the EEPROM for next seed, if reset occurs
            // Store this move into memory. First shift rand over and only take the
            // two most significant bits.
            moves[cpu_counter] = 0x01 << (random >> 13);
            
            for (i = 0; i <= cpu_counter; i++) {
                // Translate the move into something that we can send to the 
                // charlieplexed LEDs
                PORTB |= led_display(moves[i]); 
                _delay_ms(500);
                PORTB &= 0xF0;
                _delay_ms(100);
            }
            cpu_counter += 1;
            gamestate = PLAYER;
            _delay_ms(10);
        } else if (gamestate == PLAYER) {
            player_move = get_player_move(); 
            if (player_move == 0) {
                clear_display();
            } else {
                set_display(player_move);               
                _delay_ms(50);
                clear_display();
                _delay_ms(50);
                set_display(player_move);
                _delay_ms(50);
                clear_display();
                if (player_move == moves[player_counter]) {
                    if (player_counter == (cpu_counter-1)) {
                        player_counter = 0;
                        _delay_ms(1000);
                        gamestate = CPU;
                    } else {   
                        player_counter += 1;
                    }
                } else {
                    player_counter = 0;
                    cpu_counter = 0;
                    
                    gamestate = IDLE;
                    blink_leds();
                    _delay_ms(100);
                    blink_leds();
                    _delay_ms(500);

                }
            }
        } else if (gamestate == IDLE) {
            // When the game is IDLE (not being played), increment the seed.
            // Once random is done being incremented store that value at location
            // 46 in the EEPROM so it can be accessed later.
            random += 0x0001;
            eeprom_write_word((uint16_t *)46, random);
            cascade_leds();
            if (read_adc() > 200) {
                gamestate = CPU;
                blink_leds();
                _delay_ms(100);
                blink_leds();
                _delay_ms(500);
            }
        } else {
            // Flash LED 1 and 4 if there is an error
            PORTB |= led_display(0x01);
            _delay_ms(100);
            PORTB &= 0xF0;
            _delay_ms(100);
            PORTB |= led_display(0x08);
            _delay_ms(100);
            PORTB &= 0xF0;
            _delay_ms(100);
        }
    }
    return 0;
}


/**
 * led_display()
 * \param   uint8_t  state  The 4-bit one hot encoding of the current game move.
 * \return  uint8_t  encoded_state  The encodign for our charlieplexed LED display.
 *
 * \breif Converts a 4-but one hot encoded gamestate into an encoding appropriate for 
 *        the charlieplexed LED display on PORTB pins 0, 1 and 2.
 *
 * LEDS, MUST be on pins 0, 1 and 2 on any PORT. However, this can be changed by
 * changing the encodings.
 */
uint8_t led_display(uint8_t state)
{
    switch (state) {
    case 0x01:
        return 0x03;
        break;
    case 0x02:
        return 0x04;
        break;
    case 0x04:
        return 0x06;
        break;
    case 0x08:
        return 0x01;
        break;
    default:
        return 0x00;
        break;
    }
}

/**
 * cascade_leds()
 * 
 *
 * \brief Cascade the output LEDs on and off using POV.
 *        Used for the idle gamestate.
 */
void cascade_leds()
{
    /// Predefinition of the for loop variable... Saves memory
    static uint16_t i = 0;
    static uint8_t up = 1;
   
    PORTB |= led_display(0x01 << i);
    _delay_ms(100);
    PORTB &= 0xF0;
    _delay_ms(50);

    if (i == 3)
        up = 0;
    else if (i == 0)
        up = 1;

    if (up)
        i += 1;
    else
        i -= 1;
}

void blink_leds() {
    uint16_t j;
    uint16_t i;
    for( i = 0; i < 100; i++) {
        for (j = 0; j <= 3; j++) {
            set_display(0x01 << j);
            _delay_us(100);
            clear_display();
            _delay_us(10);
        }
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

uint16_t read_adc() {
    // TODO: Make more general and allow channel selection

    // ADCSRA[6]: start single conversion
    ADCSRA |= 0b01000000;
    
    // Loop until ADIF interrupt occurs
    while(!(ADCSRA & (1 << ADIF)));
    
    // Clear ADIF by writing 1 to it
    ADCSRA |= (1<<ADIF);

    // Return the ADC data
    return ADC;
}


uint8_t get_player_move() {
    uint16_t raw_move = read_adc();
    uint8_t move;
    static uint8_t prev_move = 0;
    if ((raw_move >= 500) & (raw_move <= 520)) {
        move = 0x01;
    } else if ((raw_move >= 600) & (raw_move <= 620)) {
        move = 0x02;
    } else if ((raw_move >= 660) & (raw_move <= 680)) {
        move = 0x04;
    } else if ((raw_move >= 710) & (raw_move <= 730)) {
        move = 0x08;
    } else {
        move = 0x00;
    }
    
    // Make the reading edge sensitive
    if (move == prev_move)
        move = 0;
    else
        prev_move = move;
    
    // Try to prevent bouncing
    _delay_us(1000);
    return move;
}
