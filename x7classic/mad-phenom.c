/*
This file is part of mad-phenom.

mad-phenom is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

mad-phenom is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with mad-phenom.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <stdbool.h>

#include "Globals.h"
#include "Common.h"
#include "Menu.h"
#include "Trigger.h"
#include "PushButton.h"

uint8_t counter = 0;

// This interrupt should occur approx. 3906 times per second
// divide by 4 to get an approx millisecond
ISR(TIM0_COMPA_vect) {
	TCNT0 = 0;
	counter++;
	if (counter == 4) {
		millis++;
		counter = 0;
	}
}

int main(void) {

	TCCR0B |= (1 << CS01);  // Enable timer with 1/8th prescale
	TIMSK0 |= 1 << OCIE0A; // Configure Timer0 for Compare Match
	OCR0A = 255; // Match at 200
	
	sei();  // Enable global interrupts
	
	initialize();
	
	DDRA |= (1 << PINA1); // Pin 12 - Red LED
	DDRA |= (1 << PINA2); // Pin 11 - Green LED
	DDRA |= (1 << PINA7); // Pin 6  - Solenoid
	
	DDRB &= ~(1 << PINB1); // Pin 3 - Push button
	DDRB &= ~(1 << PINB2); // Pin 5 - Trigger Pin 1
	DDRA &= ~(1 << PINA6); // Pin 7 - Trigger Pin 2

	// Other unknown pins
	//DDRA |= (1 << PINA0); // Pin 13
	//PORTA |= (1 << PINA0); // 13 - HIGH

	DDRA |= (1 << PINA3); // Pin 10 - Power pin
	PORTA |= (1 << PINA3); // 10 - HIGH
	//PORTA &= ~(1 << PINA4);	// 9 - LOW
	//PORTA &= ~(1 << PINA5);	// 8 - LOW
	
	// Set Triggers HIGH
	PORTB |= (1 << PINB2);
	PORTA |= (1 << PINA6);
	
	// Set Pushbutton HIGH
	PORTB |= (1 << PINB1);

	// Selector switch on pin 2
	DDRB &= ~(1 << PINB0); // Pin 2
	PORTB |= (1 << PINB0); // Pin 2 set HIGH
	
	// If the button is held during startup, enter config mode.
	uint16_t buttonHeldTime = 0;
	bool configMode = false;
	while ((PINB & (1 << PINB1)) <= 0) {
		delay_ms(1);
		
		buttonHeldTime++;
		if (buttonHeldTime > 3000) {
			buttonHeldTime = 3000;
		}
	}
	
	if (buttonHeldTime >= 1000) {
		configMode = true;
	}
	
	if (configMode) {
		// Enter the configuration menu
		currentSelector = 0;
		for (;;) {
			configMenu_run(&millis);
		}			
	} else { // Normal run mode
		for (;;) {
			// This prevents time from changing within an iteration
			trigger_run(&millis);
			pushbutton_run(&millis);
		}
	}		
}

/*
ISR(PCINT1_vect) {

	if (!triggerPulled
		&& (((PINB & (1 << PINB2)) <= 0) || ((PINA & (1 << PINA6)) <= 0))) { // Trigger Held
		triggerPulled = true;

		uint16_t buttonHeldTime = 0;
		delay_ms(PULL_DEBOUNCE);
		while ((((PINB & (1 << PINB2)) <= 0) || ((PINA & (1 << PINA6)) <= 0))) { // Trigger Held
			delay_ms(1);
			buttonHeldTime += 1;
			
			if (buttonHeldTime > 3000) {
				buttonHeldTime = 3000;
			}
		}
		configTriggerPulled(buttonHeldTime);
	}

	if (triggerPulled
	    && (((PINB & (1 << PINB2)) > 0) && ((PINA & (1 << PINA6)) > 0))) { // && triggerReleased()) {
		delay_ms(RELEASE_DEBOUNCE);
		triggerPulled = false;
	}
}
*/