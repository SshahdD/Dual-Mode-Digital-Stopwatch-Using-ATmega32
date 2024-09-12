/*
 * Project2.c
 *
 *  Created on: ٠٩‏/٠٩‏/٢٠٢٤
 *      Author: shahd
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define BUTTON_INC_HOURS (1 << PB1)
#define BUTTON_DEC_HOURS (1 << PB0)
#define BUTTON_INC_MINUTES (1 << PB4)
#define BUTTON_DEC_MINUTES (1 << PB3)
#define BUTTON_INC_SECONDS (1 << PB6)
#define BUTTON_DEC_SECONDS (1 << PB5)

unsigned char HOURS=0;
unsigned char MINUTES=0;
unsigned char SECONDS=0;
unsigned char countdown_mode=0;
unsigned char Flag_inc_dec=0;


void display_digit(unsigned char DIGIT, unsigned char POSITION) {
    // Turn off all digit positions (PORTA controls which digit is active)
	PORTA = (PORTA & 0xC0) | (1<<POSITION);


    // Set the 7-segment data (by lower 4 bits at PORTC)
    PORTC = (PORTC & 0xF0) | (DIGIT & 0x0F);
}

void display_time(void) {
    // Display seconds
    display_digit(SECONDS % 10, 5);  // Ones place of seconds
    _delay_ms(1);
    display_digit(SECONDS / 10, 4);  // Tens place of seconds
    _delay_ms(1);

    // Display minutes
    display_digit(MINUTES % 10, 3);  // Ones place of minutes
    _delay_ms(1);
    display_digit(MINUTES / 10, 2);  // Tens place of minutes
    _delay_ms(1);

    // Display hours
    display_digit(HOURS % 10, 1);  // Ones place of hours
    _delay_ms(1);
    display_digit(HOURS / 10, 0);  // Tens place of hours
    _delay_ms(1);
}
//void handle_buttons(void) {}
/* Interrupt Service Routine for timer1 compare mode */
ISR(TIMER1_COMPA_vect) {
	if (countdown_mode) {
	        if (SECONDS == 0) {
	            if (MINUTES == 0) {
	                if (HOURS == 0) {
	                    PORTD|=(1<<PD0);
	                	// Countdown finished
	                    return;
	                }
	                HOURS--;
	                MINUTES = 59;
	                SECONDS = 59;
	            } else {
	                MINUTES--;
	                SECONDS = 59;
	            }
	        } else {
	            SECONDS--;
	        }
	    } else {PORTD &= ~(1 << PD0);  // Turn off BUZZER
	        SECONDS++;
	        if (SECONDS >= 60) {
	            SECONDS = 0;
	            MINUTES++;
	            if (MINUTES >= 60) {
	                MINUTES = 0;
	                HOURS++;
	                if (HOURS >= 24) {
	                    HOURS = 0;  // Reset after 24 hours
	                }
	            }
	        }
	    }
}

void Timer1_CTC_Init(void) {
    TCNT1 = 0;        // Set Timer1 initial count to zero
    OCR1A = 15624;      // Set compare match value for 1-second delay

    TIMSK |= (1 << OCIE1A); // Enable Timer1 Compare A Interrupt

    // Configure Timer1
    TCCR1A = (1 << FOC1A) | (1 << FOC1B);
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC Mode, Prescaler = 1024
}

ISR(INT0_vect) {
    TCNT1 = 0;
    SECONDS = 0;
    MINUTES = 0;
    HOURS = 0;
}

void INT0_Init_FallingEdge(void) {
    MCUCR |= (1 << ISC01);   // Falling edge
    GICR |= (1 << INT0);     // Enable INT0
    PORTD |= (1 << PD2);     // Enable pull-up resistor for PD2
}

ISR(INT1_vect) {
    TCCR1B &= 0xF8; // Stop Timer1
    Flag_inc_dec=1;
}

void INT1_Init_RisingEdge(void) {
    MCUCR |= (1 << ISC10) | (1 << ISC11); // Rising edge
    GICR |= (1 << INT1);  // Enable INT1
    TCNT1 = 0;

}

ISR(INT2_vect) {
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // Restart Timer1
	PORTD&=~(1<<PD0);
	Flag_inc_dec=0;
}

void INT2_Init_FallingEdge(void) {
    MCUCSR &= ~(1 << ISC2); // Falling edge
    GICR |= (1 << INT2);    // Enable INT2
}


int main(void) {
    DDRD &= ~((1 << PD2) | (1 << PD3)); // PD2, PD3 as input
    DDRB = 0x00; // PORTB as input
    PORTB = 0xFF; // Enable pull-ups on PORTB
    DDRD |= (1 << PD0) | (1 << PD4) | (1 << PD5); // PD0, PD4, PD5 as output
    DDRC |= 0x0F; // PC0-PC3 as output
    PORTC &= 0xF0; // Initialize PORTC to zero
    DDRA |= 0x3F; // PORTA as output

    SREG |= (1 << 7); // Enable global interrupts
    PORTD |= (1 << PD4); // Enable normal increment mode (red LED on)
    PORTD &= ~(1 << PD5);

    Timer1_CTC_Init();
    INT0_Init_FallingEdge();
    INT1_Init_RisingEdge();
    INT2_Init_FallingEdge();

    while (1) {
    	display_time();
    	while (Flag_inc_dec==1) {

    	         	if (!(PINB & (1 << PB7))) {  // Enter countdown mode if PB7 is pressed
    		    	    countdown_mode ^=1;
    		    	    PORTD ^= (1 << PD5);  // Set countdown mode LED
    		    	    PORTD ^= (1 << PD4);  // Turn off normal mode LED
    		             while(!(PINB & (1 << PB7))){display_time();};

    		    	    // Stay in countdown mode while button PB7 is pressed
    		         	}
    	    	            display_time();  // Continuously update the display
    	    	            PORTD &= ~(1 << PD0);  // Turn off BUZZER
    	    	        // Hours increment
    	    	        if (!(PINB & BUTTON_INC_HOURS)) {
    	    	            HOURS = (HOURS + 1) % 24;
    	    	            display_time();  // Update display
    	    	        }

    	    	        // Hours decrement
    	    	        if (!(PINB & BUTTON_DEC_HOURS)) {
    	    	            HOURS = (HOURS == 0) ? 23 : HOURS - 1;
    	    	            display_time();  // Update display
    	    	        }

    	    	        // Minutes increment
    	    	        if (!(PINB & BUTTON_INC_MINUTES)) {
    	    	            MINUTES = (MINUTES + 1) % 60;
    	    	            display_time();  // Update display
    	    	        }

    	    	        // Minutes decrement
    	    	        if (!(PINB & BUTTON_DEC_MINUTES)) {
    	    	            MINUTES = (MINUTES == 0) ? 59 : MINUTES - 1;
    	    	            display_time();  // Update display
    	    	        }

    	    	        // Seconds increment
    	    	        if (!(PINB & BUTTON_INC_SECONDS)) {
    	    	            SECONDS = (SECONDS + 1) % 60;
    	    	            display_time();  // Update display
    	    	        }

    	    	        // Seconds decrement
    	    	        if (!(PINB & BUTTON_DEC_SECONDS)) {
    	    	            SECONDS = (SECONDS == 0) ? 59 : SECONDS - 1;
    	    	            display_time();  // Update display
    	    	        }

    	    	        display_time();  // Continuously update the display

    	    	    }
    	display_time();  // Update the display


    }
}
