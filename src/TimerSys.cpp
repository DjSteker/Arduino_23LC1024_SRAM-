/*
 * TimerSys.cpp
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 *
 * Configuracion: Timer1 (16 bits) en modo CTC.
 * Prescaler 8 -> 1 tick = 1 us (a 8 MHz) o 0.5 us (a 16 MHz).
 * OCR1A = 999 -> Interrumpe cada 1,000 ticks.
 */

#include "TimerSys.h"
#include <avr/interrupt.h>
#include <avr/io.h>

static volatile uint32_t g_micros_counter = 0;

ISR(TIMER1_COMPA_vect) {
  g_micros_counter += 1000;
}

void SystemTimer::init() {
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TCCR1B = (1 << WGM12) | (1 << CS11);
  OCR1A = 999;
  TIMSK1 = (1 << OCIE1A);
  sei();
}

uint32_t SystemTimer::getMicros() {
  uint32_t base;
  uint16_t tcnt;
  uint8_t oldSREG = SREG;

  cli();
  base = g_micros_counter;
  tcnt = TCNT1;

  if ((TIFR1 & (1 << OCF1A)) && (tcnt < (OCR1A / 2))) {
    base += (OCR1A + 1);
  }
  SREG = oldSREG;

  return base + (uint32_t)tcnt;
}

uint32_t SystemTimer::getMillis() {
  return getMicros() / 1000;
}

//------------------------------------------------------------


//------------------------------------------------------------
// // STOP Timer1 (TCCR1B)
// TCCR1B &= ~((1<<CS12)|(1<<CS11)|(1<<CS10));
//
// // Prescaler 1 Timer1 (TCCR1B)
// TCCR1B &= ~((1<<CS12)|(1<<CS11)|(1<<CS10));
// TCCR1B |= (1<<CS10);
//
// // Prescaler 8 Timer1 (TCCR1B)
// TCCR1B &= ~((1<<CS12)|(1<<CS11)|(1<<CS10));
// TCCR1B |= (1<<CS11);
//
// // Prescaler 64 Timer1 (TCCR1B)
// TCCR1B &= ~((1<<CS12)|(1<<CS11)|(1<<CS10));
// TCCR1B |= (1<<CS11)|(1<<CS10);
//
// // Prescaler 256 Timer1 (TCCR1B)
// TCCR1B &= ~((1<<CS12)|(1<<CS11)|(1<<CS10));
// TCCR1B |= (1<<CS12);
//
// // Prescaler 1024 Timer1 (TCCR1B)
// TCCR1B &= ~((1<<CS12)|(1<<CS11)|(1<<CS10));
// TCCR1B |= (1<<CS12)|(1<<CS10);
//
// STOP Timer2 (TCCR2B)
// TCCR2B &= ~((1<<CS22)|(1<<CS21)|(1<<CS20));
//
// // Prescaler 1 Timer2 (TCCR2B)
// TCCR2B &= ~((1<<CS22)|(1<<CS21)|(1<<CS20));
// TCCR2B |= (1<<CS20);
//
// // Prescaler 8 Timer2 (TCCR2B)
// TCCR2B &= ~((1<<CS22)|(1<<CS21)|(1<<CS20));
// TCCR2B |= (1<<CS21);
//
// // Prescaler 32 Timer2 (TCCR2B)
// TCCR2B &= ~((1<<CS22)|(1<<CS21)|(1<<CS20));
// TCCR2B |= (1<<CS21)|(1<<CS20);
//
// // Prescaler 64 Timer2 (TCCR2B)
// TCCR2B &= ~((1<<CS22)|(1<<CS21)|(1<<CS20));
// TCCR2B |= (1<<CS22);
//
// // Prescaler 128 Timer2 (TCCR2B)
// TCCR2B &= ~((1<<CS22)|(1<<CS21)|(1<<CS20));
// TCCR2B |= (1<<CS22)|(1<<CS20);
//
// // Prescaler 256 Timer2 (TCCR2B)
// TCCR2B &= ~((1<<CS22)|(1<<CS21)|(1<<CS20));
// TCCR2B |= (1<<CS22)|(1<<CS21);
//
// // Prescaler 1024 Timer2 (TCCR2B)
// TCCR2B &= ~((1<<CS22)|(1<<CS21)|(1<<CS20));
// TCCR2B |= (1<<CS22)|(1<<CS21)|(1<<CS20);
//
// STOP Timer0 (TCCR0B)
// TCCR0B &= ~((1<<CS02)|(1<<CS01)|(1<<CS00));
//
// // Prescaler 1 Timer0 (TCCR0B)
// TCCR0B &= ~((1<<CS02)|(1<<CS01)|(1<<CS00));
// TCCR0B |= (1<<CS00);
//
// // Prescaler 8 Timer0 (TCCR0B)
// TCCR0B &= ~((1<<CS02)|(1<<CS01)|(1<<CS00));
// TCCR0B |= (1<<CS01);
//
// // Prescaler 64  Timer0 (TCCR0B)
// TCCR0B &= ~((1<<CS02)|(1<<CS01)|(1<<CS00));
// TCCR0B |= (1<<CS01)|(1<<CS00);
//
// // Prescaler 256 Timer0 (TCCR0B)
// TCCR0B &= ~((1<<CS02)|(1<<CS01)|(1<<CS00));
// TCCR0B |= (1<<CS02);
//
// // Prescaler 1024 Timer0 (TCCR0B)
// TCCR0B &= ~((1<<CS02)|(1<<CS01)|(1<<CS00));
// TCCR0B |= (1<<CS02)|(1<<CS00);
//
// Timer0: 8 bits (Valor máximo: 255)
// Timer1: 16 bits (Valor máximo: 65,535)
// Timer2: 8 bits (Valor máximo: 255)
// ADC: 10 bits (Valor máximo: 1,023)
//
// Timer2 (TCCR2B) 8 bits
// Prescaler 8:    TCCR2B |= (1<<CS21);
// Prescaler 32:   TCCR2B |= (1<<CS22) | (1<<CS20);
// Prescaler 64:   TCCR2B |= (1<<CS22) | (1<<CS21);
// Prescaler 128:  TCCR2B |= (1<<CS22) | (1<<CS21) | (1<<CS20);
// Prescaler 256:  TCCR2B |= (1<<CS22);
// Prescaler 1024: TCCR2B |= (1<<CS22) | (1<<CS21);Timer1 (TCCR1B)Prescaler 1:    TCCR1B |= (1<<CS10);
// Timer1 (TCCR1B) 16 bits
// Prescaler 8:    TCCR1B |= (1<<CS11);
// Prescaler 64:   TCCR1B |= (1<<CS11) | (1<<CS10);
// Prescaler 256:  TCCR1B |= (1<<CS12);
// Prescaler 1024: TCCR1B |= (1<<CS12) | (1<<CS10);Timer0 (TCCR0B)Prescaler 1:    TCCR0B |= (1<<CS00);
// Timer0 (TCCR0B) 8 bits
// Prescaler 8:    TCCR0B |= (1<<CS01);
// Prescaler 64:   TCCR0B |= (1<<CS01) | (1<<CS00);
// Prescaler 256:  TCCR0B |= (1<<CS02);
// Prescaler 1024: TCCR0B |= (1<<CS02) | (1<<CS00);ADC (ADCSRA)División 2:   ADCSRA |= (1<<ADPS0);
// ADC (ADCSRA) 10 bits
// División 4:   ADCSRA |= (1<<ADPS1);
// División 8:   ADCSRA |= (1<<ADPS1) | (1<<ADPS0);
// División 16:  ADCSRA |= (1<<ADPS2);
// División 32:  ADCSRA |= (1<<ADPS2) | (1<<ADPS0);
// División 64:  ADCSRA |= (1<<ADPS2) | (1<<ADPS1);
// División 128: ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);

//------------------------------------------------------------
//inputString.reserve(200);
// bitWrite(ADCSRA, ADPS2, 1);
// bitWrite(ADCSRA, ADPS1, 0);
// bitWrite(ADCSRA, ADPS0, 1);
//ADPS2 - ADPS1 - ADPS0 - Division Factor
//0        - 0       - 0        ->2
//0        - 0       - 1        ->2
//0        - 1       - 0        ->4
//0        - 1       - 1        ->8
//1        - 0       - 0        ->16
//1        - 0       - 1        ->32
//1        - 1       - 0        ->64
//1        - 1       - 1        ->128
// También habilitar el ADC y la interrupción si es necesario
// ADCSRA |= (1 << ADEN); // Enable ADC
// ADCSRA |= (1 << ADSC); // Start conversion
//------------------------------------------------------------
