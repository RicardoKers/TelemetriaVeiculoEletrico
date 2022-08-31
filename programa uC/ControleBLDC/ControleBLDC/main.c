/*
 * ControleBLDC.c
 * ATmega8
 * Created: 20/07/2022 19:11:07
 * Author : Kerschbaumer

 VERSÃO 0.6

Sinais BLDC

Chaves
Q1 Q3 Q5
Q2 Q4 Q8

states   0  1  2  3  4  5
Hall 1	"""""""""_________
Hall 2	______"""""""""___
Hall 3	"""_________""""""

Q1(PWM) MMMMMM____________
Q2		_________""""""___
Q3(PWM)	______MMMMMM______
Q4		"""____________"""
Q5(PWM)	____________MMMMMM
Q6		___""""""_________
  */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 8000000
#include <util/delay.h>

#define placaAntiga
//#define placaNova


#if defined(placaAntiga)
	#define Hall_1 (PIND & (1<<PD0))
	#define Hall_2 (PIND & (1<<PD1))
	#define Hall_3 (PIND & (1<<PD2))

	#define Q1_H (PORTB |= (1<<PB1))
	#define Q1_L (PORTB &= ~(1<<PB1))

	#define Q2_H (PORTD |= (1<<PD5))
	#define Q2_L (PORTD &= ~(1<<PD5))

	#define Q3_H (PORTB |= (1<<PB2))
	#define Q3_L (PORTB &= ~(1<<PB2))

	#define Q4_H (PORTD |= (1<<PD6))
	#define Q4_L (PORTD &= ~(1<<PD6))

	#define Q5_H (PORTB |= (1<<PB3))
	#define Q5_L (PORTB &= ~(1<<PB3))

	#define Q6_H (PORTD |= (1<<PD7))
	#define Q6_L (PORTD &= ~(1<<PD7))
#else
	#define Hall_1 (PINC & (1<<PC0))
	#define Hall_2 (PINC & (1<<PC1))
	#define Hall_3 (PINC & (1<<PC2))

	#define Q1_H (PORTB |= (1<<PB1))
	#define Q1_L (PORTB &= ~(1<<PB1))

	#define Q2_H (PORTB |= (1<<PB0))
	#define Q2_L (PORTB &= ~(1<<PB0))

	#define Q3_H (PORTB |= (1<<PB2))
	#define Q3_L (PORTB &= ~(1<<PB2))

	#define Q4_H (PORTB |= (1<<PB4))
	#define Q4_L (PORTB &= ~(1<<PB4))

	#define Q5_H (PORTB |= (1<<PB3))
	#define Q5_L (PORTB &= ~(1<<PB3))

	#define Q6_H (PORTB |= (1<<PB5))
	#define Q6_L (PORTB &= ~(1<<PB5))
#endif

enum states {E0, E1, E2, E3, E4, E5};
enum states state;
enum states oldState;

int entAna0;

void getHallState()
{
	char hallState=0;
	if(Hall_1 != 0) hallState += 1;
	if(Hall_2 != 0) hallState += 2;
	if(Hall_3 != 0) hallState += 4;
	
	switch (hallState)
	{
		case 1:
			state = E1;
		break;
		case 2:
			state = E3;
		break;
		case 3:
			state = E2;
		break;
		case 4:
			state = E5;
		break;
		case 5:
			state = E0;
		break;
		case 6:
			state = E4;
		break;
	}
}

ISR(TIMER1_COMPA_vect)
{
	Q1_L;
	Q5_L;
	Q3_L;
}

ISR(TIMER1_OVF_vect)
{
	OCR1A=entAna0;
	TCNT1=0;
	if(entAna0>3)
	{
		switch (state)
		{
			case E0:
			Q1_H;
			break;
			case E1:
			Q1_H;
			break;
			case E2:
			Q3_H;
			break;
			case E3:
			Q3_H;
			break;
			case E4:
			Q5_H;
			break;
			case E5:
			Q5_H;
			break;
		}
	}
}

ISR(ADC_vect)
{
	entAna0=ADCW/2;
	if(entAna0>501) entAna0=501;
	ADCSRA|=0b01000000; // inicia nova conversão
}

int main(void)
{
	ICR1 = 499; // Divide o clock por 1000 (ICR1+1) para um PWM de 1 KHz
	OCR1A=0; // Ajusta o PWM inicial para 0%
	TCCR1A = (1 << WGM11); // Ajusta para o modo PWM Rápido
	TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS10); // Ajusta o prescaler para 1
	TIMSK  = (1 << OCIE1A) | (1 << TOIE1); // habilita as interrupções
	
	#if defined(placaAntiga)
		DDRB = 0b00001110;
		DDRD = 0b11100000;
		ADMUX=0b01000000; // escolhe a entrada analógica 0, com a referência no pino AVCC
	#else
		DDRB = 0b00111111;
		DDRD = 0b00000000;
		ADMUX=0b01000011; // escolhe a entrada analógica 0, com a referência no pino AVCC
	#endif  
	 
	ADCSRA=0b11001111; // habilita o A/D com prescaler de 128, habilitando sua interrupção.
	
	sei();
	
    while (1) 
    {
		cli();
		getHallState();	
		if(state!=oldState)
		{
			oldState=state;
			Q1_L;
			Q5_L;
			Q3_L;
			switch (state)
			{
				case E0:
				Q4_H;
				Q2_L;
				Q6_L;
				break;
				case E1:
				Q6_H;
				Q4_L;
				Q2_L;
				break;
				case E2:
				Q6_H;
				Q4_L;
				Q2_L;
				break;
				case E3:
				Q2_H;
				Q4_L;
				Q6_L;
				break;
				case E4:
				Q2_H;
				Q4_L;
				Q6_L;
				break;
				case E5:
				Q4_H;
				Q2_L;
				Q6_L;
				break;
			}
			TCNT1=0;
		}		
		sei();
    }
}

