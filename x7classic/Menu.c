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
#include <stdbool.h>
#include "Globals.h"
#include "Common.h"
#include "Menu.h"

#define COLOR_RED          0
#define COLOR_GREEN        1
#define COLOR_ORANGE       2
#define COLOR_REDGREEN_50  3
#define COLOR_REDGREEN_100 4

/************************************************************************/
/* CONFIG MENU                                                          */
/************************************************************************/

bool triggerPulled = false;
uint32_t triggerActiveTime = 0;
uint8_t triggerState = 0;

uint32_t triggerHeldTime = 0;
bool triggerWasReleased = 0;

/************************************************************************/
/* If number to blink is greater than 0,                                */
/* then the pauseBetween will take affect                               */
/* 0 - red                                                              */
/* 1 - green                                                            */
/* 2 - orange                                                           */
/* 3 - red green alternating (50ms each)                                */
/* 4 - red green alternating (100ms each)                               */
/************************************************************************/
void indicateNumber(volatile uint32_t *millis, uint8_t numberToBlink, uint8_t color, uint16_t offTime, uint16_t onTime, uint16_t pauseBetween) {

	static uint32_t currentStateActiveTime = 0;
	static bool indicateNumber_ledOn = false;
	static uint8_t indicateNumber_currentBlink = 0;
	static uint32_t indicateNumber_colorActive = 0;
	static uint8_t colorState = 0;

	// Initialize to color 3
	uint8_t greenOnTime = 50;
	uint8_t redOnTime = 50;

	// set for Orange
	if (color == COLOR_ORANGE) {
		greenOnTime = 2;
		redOnTime = 1;
	} else if (color == COLOR_REDGREEN_100) {
		greenOnTime = 100;
		redOnTime = 100;
	}

	// This code will turn the LED on and off to signify the number passed in
	if (!indicateNumber_ledOn 
			&& ((offTime == 0) || ((*millis) - currentStateActiveTime) > offTime)
			&& indicateNumber_currentBlink < numberToBlink) {
		
		switch (color) {
			case COLOR_RED:
				redOn();
			break;
			case COLOR_GREEN:
				greenOn();
			break;
			case COLOR_ORANGE: // Orange
			case COLOR_REDGREEN_50:
			case COLOR_REDGREEN_100:
				switch (colorState) {
					case 0: // Off
						greenOn();
						indicateNumber_colorActive = (*millis);
						colorState = 1;
					break;
					case 1: // Green On
						// If green has been on for 2 ms or more, turn green off and turn red on
						if (((*millis) - indicateNumber_colorActive) >= greenOnTime) {
							greenOff();
							redOn();
							indicateNumber_colorActive = (*millis);
							colorState = 2;
						}
					break;
					case 2:
						// If red has been on for 1 ms or more, turn red off
						if (((*millis) - indicateNumber_colorActive) >= redOnTime) {
							redOff();
							indicateNumber_colorActive = (*millis);
							colorState = 0;
						}
					break;
				}
			break;
		}
					
		indicateNumber_ledOn = true;
		currentStateActiveTime = (*millis);
		indicateNumber_currentBlink++;
	}

	// If the number has exceeded it's onTime, then turn it off
	// If the color is not supposed to turn off, pass in 0 for onTime
	if (indicateNumber_ledOn && ((*millis) - currentStateActiveTime) > onTime) {

		if (onTime > 0) {
			redOff();
			greenOff();
		}			

		indicateNumber_ledOn = false;
		currentStateActiveTime = (*millis);
	}
			
	// Long pause between
	if (!indicateNumber_ledOn 
			&& ((*millis) - currentStateActiveTime) > pauseBetween 
			&& ((indicateNumber_currentBlink >= numberToBlink) || (numberToBlink == 0))) {
				
		indicateNumber_currentBlink = 0;
	}
}

void configMenu_run(volatile uint32_t *millis) {

	static uint32_t currentMenuActiveTime = 0;
	static uint8_t burstSize = 0;
	static uint8_t firingRate = 0;
	static uint8_t ammoLimit = 0;
	static uint8_t safetyShot = 0;

	// 0 - Idle
	// 1 - Pulled
	// 2 - Held
	// 3 - Released
	//triggerState

	// Trigger states should only happen once.
	// Set to idle on the second go around
	triggerState = 0;

	// Trigger Pulled
	if (!triggerPulled
		&& (((PINB & (1 << PINB2)) <= 0) || ((PINA & (1 << PINA6)) <= 0)) // Trigger Held
		&& (((*millis) - triggerActiveTime) >= RELEASE_DEBOUNCE)) { //checkReleaseDebounce(millis)) {

		triggerPulled = true;
		triggerActiveTime = (*millis);
		triggerHeldTime = (*millis);  // How long the trigger was held for

		// Pulled
		triggerState = 1;
	}


	// Trigger Held
	if (triggerPulled
		&& (((PINB & (1 << PINB2)) <= 0) || ((PINA & (1 << PINA6)) <= 0)) // Trigger Held
		&& (((*millis) - triggerActiveTime) >= PULL_DEBOUNCE) // checkPullDebounce(millis)
		&& (((*millis) - triggerActiveTime) >= ROUND_DELAY)) {
		
		// Held
		triggerState = 2;
	}

	// Trigger Release
	if (triggerPulled
		&& (((PINB & (1 << PINB2)) > 0) && ((PINA & (1 << PINA6)) > 0)) // triggerReleased()
		&& (((*millis) - triggerActiveTime) >= PULL_DEBOUNCE)) { //checkPullDebounce(millis)) {

		triggerPulled = false;
		triggerActiveTime = (*millis);
		
		// Released
		triggerState = 3;
	}


	
	// 0  - Select Mode (F or FA)
	// 1  - Select Preset
	// 2  - Main Menu
	// 3  - Firing Mode
	// 4  - Full Auto
	// 5  - Three Round Burst
	// 6  - Auto Response
	// 7  - Semi Auto (Single Shot)
	// 8  - Firing Rate
	// 9  - Select a number between 5 and 40
	// 10 - Burst size
	// 11 - Select a number between 2 and 10
	uint8_t menuState = 0;
	
	switch (menuState) {
		case 0: // Select Mode (F or FA) (orange blink indicating current mode)
		
			// 0 - F
			// 1 - FA
			// currentSelector
			
			indicateNumber(millis, (currentSelector + 1), COLOR_ORANGE, 200, 200, 1000);
			
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				currentSelector = !currentSelector;
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				redOff();
				menuState = 1;
				currentMenuActiveTime = (*millis);
			}
		break;  
		case 1: // Select Preset (green blink indicating current preset)

			indicateNumber(millis, (CURRENT_PRESET[currentSelector] + 1), COLOR_GREEN, 200, 200, 1000);
			
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				if (CURRENT_PRESET[currentSelector] >= (MAX_PRESETS - 1)) {
					CURRENT_PRESET[currentSelector] = 0;
				} else {
					CURRENT_PRESET[currentSelector]++;
				}
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				menuState = 3;
				currentMenuActiveTime = (*millis);
				loadPreset();
			}
		
		break;
		case 2: // Main Menu (??? there is no "Main menu")
			// This code will turn the green LED on and off to signify which preset is active
			menuState = 0;
		break;
		case 3: // Firing Mode (toggle between green and red each on for 100ms)
		
			indicateNumber(millis, 0, COLOR_REDGREEN_100, 0, 0, 0);
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				greenOff();
				redOff();
				menuState = 8;
				currentMenuActiveTime = (*millis);
			}
			
			/*
				0 - Firing Mode
					0 - Full Auto
					1 - Three Round Burst
					2 - Auto Response
					3 - Semi-Auto (Single Shot)
				1 - Firing Rate (Ball Per Second)
					5 - 40
				2 - Burst size
					2 - 10
			*/
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();
				
				// Go to the loaded preset's current firing mode
				switch (FIRING_MODE) {
					default:
					case 0: // Full Auto
						menuState = 4;
					break;
					case 1: // Burst Mode
						menuState = 5;
					break;
					case 2: // Auto Response
						menuState = 6;
					break;
					case 3: // Semi-Auto
						menuState = 7;
					break;
				}
				
				currentMenuActiveTime = (*millis);
			}
		break;
		case 4: // Full Auto (rapid blinking red)
			indicateNumber(millis, 0, COLOR_RED, 50, 50, 0);
			
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				menuState = 5;  // Burst Mode
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();
				
				// Save the firing mode
				eeprom_write_byte(&EEPROM_FIRING_MODE[currentSelector][CURRENT_PRESET[currentSelector]], 0);
				menuState = 12;
				currentMenuActiveTime = (*millis);
			}
		
		break;
		case 5: // Burst Mode (three green blinks, then pause)
			indicateNumber(millis, 3, COLOR_GREEN, 100, 100, 1000);
			
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				menuState = 6;  // Auto-Response
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();
				
				// Save the firing mode
				eeprom_write_byte(&EEPROM_FIRING_MODE[currentSelector][CURRENT_PRESET[currentSelector]], 1);
				menuState = 12;
				currentMenuActiveTime = (*millis);
			}
			
		break;
		case 6: // Auto Response (blink green, blink red, then pause)
		
			indicateNumber(millis, 1, COLOR_REDGREEN_100, 0, 0, 1000);
		
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				menuState = 7;  // Auto-Response
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();
				
				// Save the firing mode
				eeprom_write_byte(&EEPROM_FIRING_MODE[currentSelector][CURRENT_PRESET[currentSelector]], 2);
				menuState = 12;
				currentMenuActiveTime = (*millis);
			}
		break;
		case 7: // Semi Auto (solid green)
		
			greenOn();
			
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				menuState = 4;  // Auto-Response
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();
				
				// Save the firing mode
				eeprom_write_byte(&EEPROM_FIRING_MODE[currentSelector][CURRENT_PRESET[currentSelector]], 3);
				menuState = 12;
				currentMenuActiveTime = (*millis);
			}
		break;
		case 8: // Firing Rate (fast green blink)
		
			indicateNumber(millis, 0, COLOR_GREEN, 0, 0, 1000);
			
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				menuState = 10;  // Burst Size
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				firingRate = 0;
				menuState = 9;
				currentMenuActiveTime = (*millis);
			}
		break;
		case 9: // Select a number between 5 and 40
		
			// Once the user starts entering, don't display the current count
			if (firingRate == 0) {
				indicateNumber(millis, BALLS_PER_SECOND, COLOR_GREEN, 50, 50, 1000);
			}				
			
			// If the trigger was pulled quickly, increment the number
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				firingRate++;
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();
				
				
				// Validate firing rate
				if (firingRate < 5 || firingRate > 40) {
					menuState = 13;
				} else {
					// Save the firing mode
					eeprom_write_byte(&EEPROM_BALLS_PER_SECOND[currentSelector][CURRENT_PRESET[currentSelector]], firingRate);
					menuState = 12;
				}					
				
				currentMenuActiveTime = (*millis);
			}
		break;
		case 10: // Burst Size (three red blinks, then pause)
			indicateNumber(millis, 3, COLOR_RED, 50, 50, 1000);
			
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				menuState = 14;  // Ammo Limit
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				burstSize = 0;
				menuState = 11;
				currentMenuActiveTime = (*millis);
			}
			
		break;
		case 11: // Select a number between 2 and 10
		
			// Once the user starts entering, don't display the current count
			if (burstSize == 0) {
				indicateNumber(millis, BURST_SIZE, COLOR_GREEN, 50, 50, 1000);
			}				
			
			// If the trigger was pulled quickly, increment the number
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				burstSize++;
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();
				
				// Validate burst size
				if (burstSize < 2 || burstSize > 10) {
					menuState = 13;
				} else {
					// Save the firing mode
					eeprom_write_byte(&EEPROM_BURST_SIZE[currentSelector][CURRENT_PRESET[currentSelector]], burstSize);
					menuState = 12;
				}
				
				currentMenuActiveTime = (*millis);
			}
			
		break;
		case 12: // Success blink (solid orange for 500ms)
		
			// Indicate success
			indicateNumber(millis, 0, COLOR_ORANGE, 0, 500, 0);
		
			// Stay on this menu for 500ms then move automatically to the main menu
			if ((*millis) - currentMenuActiveTime > 500) {
				menuState = 0;
			}				
		case 13: // Fail blink (solid red for 500ms)
		
			// Indicate success
			indicateNumber(millis, 0, COLOR_RED, 0, 500, 0);
		
			// Stay on this menu for 500ms then move automatically to the main menu
			if ((*millis) - currentMenuActiveTime > 500) {
				menuState = 0;
			}
		break;
		case 14: // Ammo Limit (Solid red)
			redOn();
			
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				menuState = 16;  // Safety Shot
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();

				ammoLimit = 0;
				menuState = 15;
				currentMenuActiveTime = (*millis);
			}
		
		break;
		case 15: // Select a number between 0 and 250 (Ammo limit)
			// Once the user starts entering, don't display the current count
			if (ammoLimit == 0) {
				indicateNumber(millis, AMMO_LIMIT, COLOR_GREEN, 50, 50, 1000);
			}

			// If the trigger was pulled quickly, increment the number
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				ammoLimit++;
			}

			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();

				// Validate burst size
				if (ammoLimit < 0 || ammoLimit > 250) {
					menuState = 13;
				} else {
					// Save the ammo limit
					eeprom_write_byte(&EEPROM_AMMO_LIMIT[currentSelector][CURRENT_PRESET[currentSelector]], ammoLimit);
					menuState = 12;
				}

				currentMenuActiveTime = (*millis);
			}

		break;
		case 16: // Safety Shot (Solid green)
			greenOn();
			
			indicateNumber(millis, 3, COLOR_RED, 50, 50, 1000);
			
			// If the trigger was pulled quickly, toggle the currentSelector
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				menuState = 3;  // Ammo Limit
			}
			
			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				safetyShot = 0;
				menuState = 11;
				currentMenuActiveTime = (*millis);
			}
			
		break;
		case 17: // Select a number between 0 and 5 (Safety Shot)
			// Once the user starts entering, don't display the current count
			if (safetyShot == 0) {
				indicateNumber(millis, SAFETY_SHOT, COLOR_GREEN, 50, 50, 1000);
			}

			// If the trigger was pulled quickly, increment the number
			if (triggerState == 3 && ((*millis) - triggerHeldTime) < 1000) {
				safetyShot++;
			}

			if (triggerState == 3 && ((*millis) - triggerHeldTime) >= 1000) {
				greenOff();
				redOff();

				// Validate safety shot
				if (safetyShot < 0 || safetyShot > 5) {
					menuState = 13;
				} else {
					// Save the ammo limit
					eeprom_write_byte(&EEPROM_SAFETY_SHOT[currentSelector][CURRENT_PRESET[currentSelector]], safetyShot);
					menuState = 12;
				}

				currentMenuActiveTime = (*millis);
			}
		
		break;
		
	}
	
}	

#define NOT_SELECTED 255

/*

volatile uint8_t currentMenu = 0;
volatile uint8_t menuMax = 0;
volatile uint8_t selectedMenu = NOT_SELECTED;

void getNumberFromUser(uint8_t currentNumber, uint8_t max);

void lightsOff() {
	PORTA &= ~(1 << PINA2); // GREEN
	PORTA &= ~(1 << PINA1); // RED
}

void orangeLed() {
	for (uint8_t x = 0; x < 33; x++) {
		greenOn();
		delay_ms(2);
		redOn();
		
		greenOff();
		delay_ms(1);
		redOff();
	}	
}

void presetMenu() {

	// SELECTOR
	currentSelector = 0;
	menuMax = 1;
	selectedMenu = NOT_SELECTED;
	currentMenu = 0;

	while(selectedMenu == NOT_SELECTED) {
		if (currentMenu == 0) { // Preset 1
			redOn();
			delay_ms(100);
			redOff();
			delay_ms(800);
		} else if (currentMenu == 1) { // Preset 2
			for (uint8_t i = 0; i < 2; i++) {
				redOn();
				delay_ms(100);
				redOff();
				delay_ms(100);
			}

			delay_ms(700);
		}
	}

	currentSelector = selectedMenu;

	////////

	menuMax = 2;
	selectedMenu = NOT_SELECTED;
	currentMenu = 0;
	while(selectedMenu == NOT_SELECTED) {
				
		if (currentMenu == 0) { // Preset 1
			orangeLed();
			delay_ms(800);
		} else if (currentMenu == 1) { // Preset 2
			for (uint8_t i = 0; i < 2; i++) {
				orangeLed();
				delay_ms(100);
			}
			
			delay_ms(800);
		} else if (currentMenu == 2) {  // Preset 3
			for (uint8_t i = 0; i < 3; i++) {
				orangeLed();
				delay_ms(100);
			}
			
			delay_ms(800);
		}
	}
	
	CURRENT_PRESET[currentSelector] = selectedMenu;
}

void mainMenu() {
	menuMax = 4;
	selectedMenu = NOT_SELECTED;
	currentMenu = 0;
	bool state = false;
	while(selectedMenu == NOT_SELECTED) {
		if (currentMenu == 0) { // Firing Mode (toggle green then red)
			state = !state;
			
			redSet(!state);
			greenSet(state);
						
			delay_ms(100);
		} else if (currentMenu == 1) { // Firing Rate (fast green blink)
			state = !state;
			
			redOff();
			greenSet(state);
			
			delay_ms(50);
		} else if (currentMenu == 2) {  // Burst size (three red blinks)
			lightsOff();
			
			// Display as three blinks of red then pause and repeat
			for (uint8_t i = 0; i < 3; i++) {
				redOn();
				delay_ms(100);
				redOff();
				
				if (i == 2) {
					for (uint8_t i = 0; i < 100; i++) {
						delay_ms(10);
						
						if (selectedMenu != NOT_SELECTED) {
							break;
						}
					}					
				} else {
					delay_ms(100);
				}
			}
		} else if (currentMenu == 3) {  // Ammo Limit (Solid Red)
			redOn();
			greenOff();
		} else if (currentMenu == 4) {  // Safety Shot (Solid Green)
			redOff();
			greenOn();
		}			
	}
}

void firingModeMenu() {
	menuMax = 3;
	selectedMenu = NOT_SELECTED;
	currentMenu = FIRING_MODE;
	bool state = LOW;
	while(selectedMenu == NOT_SELECTED) {
		if (currentMenu == 0) {
			state = !state;
			
			redSet(state);
			greenOff();
			
			delay_ms(50);
		} else if (currentMenu == 1) { // Three Round Burst
			lightsOff();
			
			// Display as three blinks of green then pause and repeat
			for (uint8_t i = 0; i < 3; i++) {
				greenOn();
				delay_ms(100);
				greenOff();
				
				if (selectedMenu != NOT_SELECTED) {
					break;
				}
				
				if (i == 2) {
					delay_ms(1000);
				} else {
					delay_ms(100);
				}
			}				
		} else if (currentMenu == 2) { // Auto Response
			lightsOff();
	
			// Display as blink green, blink red, then pause
			greenOn();
			delay_ms(100);
			greenOff();
			delay_ms(100);
			redOn();
			delay_ms(100);
			redOff();

			if (selectedMenu != NOT_SELECTED) {
				break;
			}
		
			delay_ms(1000);
		} else if (currentMenu == 3) { // Auto Response
			redOff();
			greenOn();
		}
	}
	
	if (selectedMenu >= 0 && selectedMenu <= 3) {
		eeprom_write_byte(&EEPROM_FIRING_MODE[currentSelector][CURRENT_PRESET[currentSelector]], selectedMenu);
		successBlink();
	} else {
		failureBlink();
	}
}

void ammoLimitMenu() {
	getNumberFromUser(AMMO_LIMIT, 250);
	
	// Burst size was entered into selectedMenu.  Verify it and save it.
	if (selectedMenu >= 0 && selectedMenu <= 250) {
		eeprom_write_byte(&EEPROM_AMMO_LIMIT[currentSelector][CURRENT_PRESET[currentSelector]], selectedMenu);
		AMMO_LIMIT = selectedMenu;
		successBlink();
	} else {
		failureBlink();
	}
}

void safetyShotMenu() {
	getNumberFromUser(SAFETY_SHOT, 5);
	
	// Burst size was entered into selectedMenu.  Verify it and save it.
	if (selectedMenu >= 0 && selectedMenu <= 250) {
		eeprom_write_byte(&EEPROM_SAFETY_SHOT[currentSelector][CURRENT_PRESET[currentSelector]], selectedMenu);
		SAFETY_SHOT = selectedMenu;
		successBlink();
	} else {
		failureBlink();
	}
}

void successBlink() {
	for (uint8_t i = 0; i < 3; i++) {
		for (uint8_t x = 0; x < 200; x++) {
			greenOn();
			delay_ms(2);
			redOn();
			greenOff();
			delay_ms(1);
			redOff();
		}
	}
}

void failureBlink() {
	delay_ms(200);
	for (uint8_t i = 0; i < 10; i++) {
		redOn();
		delay_ms(50);
		redOff();
		delay_ms(50);
	}
}

void rateOfFireMenu() {
	 getNumberFromUser(BALLS_PER_SECOND, 40);
	
	// Firing rate was entered into selectedMenu.  Verify it and save it.
	if (selectedMenu >= 5 && selectedMenu <= 40) {
		eeprom_write_byte(&EEPROM_BALLS_PER_SECOND[currentSelector][CURRENT_PRESET[currentSelector]], selectedMenu);
		BALLS_PER_SECOND = selectedMenu;
		successBlink();
	} else {
		failureBlink();
	}
}

void burstSizeMenu() {
	getNumberFromUser(BURST_SIZE, 10);
	
	// Burst size was entered into selectedMenu.  Verify it and save it.
	if (selectedMenu >= 2 && selectedMenu <= 10) {
		eeprom_write_byte(&EEPROM_BURST_SIZE[currentSelector][CURRENT_PRESET[currentSelector]], selectedMenu);
		BURST_SIZE = selectedMenu;
		successBlink();
	} else {
		failureBlink();
	}
}

void getNumberFromUser(uint8_t currentNumber, uint8_t max) {
	bool state = false;
	menuMax = max;
	selectedMenu = NOT_SELECTED;
	currentMenu = 0;
	uint8_t currentRate = currentNumber;
	uint8_t displayRate = 0;
	delay_ms(500);
	while(selectedMenu == NOT_SELECTED) {
		if (currentMenu == 0) {
			if (state) {
				displayRate++;
			}
			
			state = !state;
			
			redOff();
			greenSet(state);
			
			delay_ms(200);
			
			if (displayRate >= currentRate) {
				displayRate = 0;
				delay_ms(1000);
			}
		} else {
			lightsOff();
		}
	}
}

void handleConfig() {
	while (1) {
		presetMenu();
		
		loadPreset();
		
		mainMenu();
	
		if (selectedMenu == 0) {
			firingModeMenu();
		} else if (selectedMenu == 1) {
			rateOfFireMenu();
		} else if (selectedMenu == 2) {
			burstSizeMenu();
		} else if (selectedMenu == 3) {
			ammoLimitMenu();
		} else if (selectedMenu == 4) {
			safetyShotMenu();
		}
	}			
}

void configTriggerPulled(uint32_t buttonHeldTime) {
	
	// Select the menu
	if (buttonHeldTime >= 1000) {
		selectedMenu = currentMenu;
	} else {
		// Cycle through the menus
		if (currentMenu == menuMax) {
			currentMenu = 0;
		} else {
			currentMenu++;
		}
		
		greenOn();
		delay_ms(50);
		greenOff();
	}		
}

*/