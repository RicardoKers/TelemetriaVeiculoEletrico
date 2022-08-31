/*
 * microAD.c
 * Leitira das entradas analógicas p/ telemetria do veículo elétrico
 * Created: 18/04/2022 13:52:49
 * Author : Kerschbaumer
 */ 

#include <avr/io.h>
#define F_CPU 8000000
#include <util/delay.h>
#include <avr/interrupt.h>

//dados da SPI
#define SPI_data_size 14

unsigned int ADC_channel;
char ADC_scanFiniched=0;

volatile uint8_t SPI_RX_data[SPI_data_size]; // dados a serem recebidos
int SPI_TX_pt=0;
int SPI_Reception_OK=0;
int SPI_Timeout=0;

union u_data
{
	unsigned int valores[SPI_data_size/2];
	uint8_t SPI_TX_data[SPI_data_size];
} data;


void SPI_SlaveInit()
{
	// Set MISO output, all others input
	DDRB = (1<<PB4);
	// Enable SPI, SLAVE, set clock /128
	// SPCR -> SPIE SPE DORD MSTR CPOL CPHA SPR1 SPR0
	SPCR = (1<<SPIE)|(1<<SPE)|(1<<CPHA);
}

ISR (SPI_STC_vect)
{
	SPDR = data.SPI_TX_data[SPI_TX_pt];
	SPI_Timeout=0; // prepara timeout
	if(SPI_TX_pt<SPI_data_size)
	{
		SPI_RX_data[SPI_TX_pt-1]=SPDR;
	}
	if(SPI_TX_pt==SPI_data_size)
	{
		SPI_RX_data[SPI_TX_pt-1]=SPDR;
		SPI_Reception_OK=1;
		SPI_Timeout=2;
	}
	SPI_TX_pt++;
}

void SPI_StartReception()
{
	SPDR = data.SPI_TX_data[0];
	SPI_TX_pt=1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Liga o temporizador com o intervalor ajustado para 1ms
//	Ajustar para o clock utilizado
//////////////////////////////////////////////////////////////////////////////////////////////////
void  inicia_timer_1ms()
{
	OCR2=123;			// Ajusta o valor de comparação do timer 2
	TCNT2=0;			// Zera a contagem do timer 2
	TCCR2=0b00001010;	// habilita o clock do timer 2 com prescaller
	TIMSK|=0b10000000;	// habilita a interrupção do timer 2
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção de temporizador 2
//////////////////////////////////////////////////////////////////////////////////////////////////

ISR(TIMER2_COMP_vect) // interrupção do temporizador
{
	if(SPI_Timeout<2)
	{
		SPI_Timeout++;
	}
	if(SPI_Timeout==2) // timeout da SPI
	{
		SPI_StartReception(); //reinicia a recepção
	}

}

void ADC_init()
{
	// ADMUX -> REFS1 REFS0 ADLAR – MUX3 MUX2 MUX1 MUX0
	ADMUX = (1<<REFS0); // Reference at AVcc, channel 0
	// ADCSRA -> ADEN ADSC ADFR ADIF ADIE ADPS2 ADPS1 ADPS0
	// ADC = On, interrupt = ON, prescaler /128
	ADCSRA = (1<<ADEN) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

void ADC_startConversion()
{
	ADC_channel=0;
	ADMUX = (1<<REFS0); // channel 0
	ADCSRA |= (1<<ADSC);
	ADC_scanFiniched=0;
}

ISR (ADC_vect)
{
	data.valores[ADC_channel] = data.valores[ADC_channel]+ADCW;
	ADMUX = (1<<REFS0); // reset channel
	switch (ADC_channel)
	{
		case 0:
		ADC_channel=1;
		ADMUX |= 1;
		ADCSRA |= (1<<ADSC);
		break;

		case 1:
		ADC_channel=2;
		ADMUX |= 2;
		ADCSRA |= (1<<ADSC);
		break;
		
		case 2:
		ADC_channel=3;
		ADMUX |= 3;
		ADCSRA |= (1<<ADSC);
		break;
		
		case 3:
		ADC_channel=4;
		ADMUX |= 4;
		ADCSRA |= (1<<ADSC);
		break;

		case 4:
		ADC_channel=5;
		ADMUX |= 6; // pulando canal 5 não usado no hardware
		ADCSRA |= (1<<ADSC);
		break;
		
		case 5:
		ADC_channel=6;
		ADMUX |= 7;
		ADCSRA |= (1<<ADSC);
		break;

		/*case 6:
		ADC_channel=7;
		ADMUX |= 7;
		ADCSRA |= (1<<ADSC);
		break;*/

		case 6:
		ADC_channel=0;
		ADC_scanFiniched=1;
		break;

		default:
		ADC_channel=0;
		ADMUX = (1<<REFS0); // channel 0
	}
}

int main(void)
{

		ADC_init();
    	SPI_SlaveInit();
    	inicia_timer_1ms();
    	sei();
    	SPI_Timeout=0;
    while (1) 
    {
		if(SPI_Reception_OK==1)
		{
			SPI_Reception_OK=0;
			data.valores[0]=0;
			data.valores[1]=0;
			data.valores[2]=0;
			data.valores[3]=0;
			data.valores[4]=0;
			data.valores[5]=0;
			data.valores[6]=0;
			
			for(int cont=0; cont<49; cont++)
			{
				ADC_startConversion();
				while(ADC_scanFiniched==0)
				{
					
				}				
			}			
			SPI_StartReception();
		}
    }
}

