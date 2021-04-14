/*
 * Soft_Starter_Control.c
 *
 * Created: 1/19/2021 1:58:24 PM
 * Author : Jose A. Velazquez IV
 */ 

#ifndef F_CPU
#define F_CPU 8000000UL // Setup the clock for the MCU
#endif

#include <avr/io.h>
#include <util/delay.h>
int analogData;        //declare analogData variable

// analog_result = (ADCH << 8)|ADCL; For a 10-bit ADC output


void ADC_setup() {
	ADMUX = 0b00100011;   // Sets Vcc as reference, sets ADC3 as input channel,
	//and left adjusts
	ADCSRA = 0b10000110;  // Turn on ADC, keep ADC single conversion mode,
	//and set division factor-64 for 125kHz ADC clock
}
void analogRead(){
	ADCSRA |= (1 << ADSC);  //start conversion
	analogData = ADCH;      //store data in analogData variable
}

void my_delay_us_ADC(int n) {
	while(n--) {
		_delay_us(1);
		analogRead();
	}
}
int main(void)
{	DDRB &= ~(1 << PB3); // PB4, Pin 3 of the AtTiny microcontroller
	DDRB |= ((1 << PB0) | (1 << PB1) | (1 << PB2));  // Sets PB0, PB1, and PB2 as outputs
	ADC_setup(); // Sets up the ADC
	
	while (1)
	{
		_delay_ms(1000); // Wait one second for program to start
		analogRead(); // Reads the ADC
		
		if(analogData < 50){  // Will not let the soft-stater to start if potentiometer is below .98[V].
			PORTB &= ~(1 << PB0);
			_delay_ms(500);
			PORTB &= ~(1 << PB1);
			_delay_ms(500);
			PORTB &= ~(1 << PB2);
			_delay_ms(500);
			// Sets PB1, PB2, and PB3 low
			// Can turn them all off because a flyback diode is used to mitigate the back EMF (Electromotive Force)
		}
		analogRead();
		if((analogData >= 200)){ // Enables the soft-starter if voltages is greater than 3.9[V] from potentiometer
			PORTB |= (1 << PB0); // Turns on PB0, which will turn on MOSFET
			for(int i=0;i<1000;i++){ // Will delay roughly 1 second
				analogRead();
				if(analogData < 50){ // Breaks loop if analogData is less than .98[V]
					break;
				}
				_delay_us(1000);
			}
			if(analogData < 50)
			continue;
			PORTB |= (1 << PB1); // Turns on PB1, which will turn on MOSFET
			for(int i=0;i<1000;i++){ // Will delay roughly 1 second
				analogRead();
				if(analogData < 50){ // Breaks loop if analogData is less than .98[V]
					break;
				}
				_delay_us(1000);
			}
			if(analogData < 50){ // Continues loop if analogData is less than .98[V]
				continue; // Starts at the beginning of the loop
			}
			PORTB |= (1 << PB2); // Turns on PB2, which will turn on MOSFET
			for(int i=0;i<1000;i++){ // Will delay roughly 1 second
				analogRead();
				if(analogData < 50){ // Breaks loop if analogData is less than .98[V]
					break;
				}
				_delay_us(1000);
			}
		}
	}
}
