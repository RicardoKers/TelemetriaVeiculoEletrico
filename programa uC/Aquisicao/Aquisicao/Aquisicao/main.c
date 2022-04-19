/*
 * Aquisicao.c
 * Aquisi��o da dados, Telemetria ve�culo El�trico SEM
 * ATMega8
 * Created: 18/04/2022 09:01:59
 * Author : Kerschbaumer
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000 // Clock do microcontrolador necess�rio para serial e delay
#include <util/delay.h>

// defini��es
#define Led_on()  PORTB |= (1<<PB0);
#define Led_off()  PORTB &= ~(1<<PB0);
#define SPI_data_size 14

// vari�veis globais
union u_data
{
	uint16_t wordData[50];
	uint8_t SPI_RX_data[100];
} data;

uint8_t bitData[100];
unsigned char  SPI_TX_data[SPI_data_size]; // dados a serem transmitidos

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//				M o d B u s
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
// configura��o do pino do driver RS485
#define ModBusTxEnablePort PORTD
#define ModBusTxEnablePin PD4
#define ModBusTxEnableDDR DDRD

// configura��o da serial
#define USART_BAUDRATE 19200	// taxa de transmiss�o desejada
#define STOPBITS 2				// n�mero de bits de parada 1 ou 2
#define BAUD_PRESCALE ((F_CPU/ (USART_BAUDRATE * (long)16))-1) //calcula o valor do prescaler da usart
// fim da configura��o da serial

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Inicia transmiss�o
//////////////////////////////////////////////////////////////////////////////////////////////////
void iniciaTransmissao(uint8_t dado) // deve vir antes do #include "ModBusMaster.h"
{
	ModBusTxEnablePort|=(1<<ModBusTxEnablePin); // Habilita a transmiss�o do driver 485 se necess�rio
	UDR = dado;									// transmite o primeiro byte, os seguintes s�o transmitidos na interrup��o da serial
	UCSRB |= (1 << UDRIE);						// habilita a interrup��o da serial
}

#include "ModBusMaster.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Liga o temporizador usado na modBus com o intervalor ajustado para 1ms
//	Ajustar para o clock utilizado
//////////////////////////////////////////////////////////////////////////////////////////////////
void  inicia_timer_1ms()
{
	OCR2=124;			// Ajusta o valor de compara��o do timer 2
	TCNT2=0;			// Zera a contagem do timer 2
	TCCR2=0b00001101;	// habilita o clock do timer 2 com prescaller
	TIMSK|=0b10000000;	// habilita a interrup��o do timer 2
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrup��o de fim de transmiss�o
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(USART_TXC_vect) 
{
	ModBusTxEnablePort&=~(1<<ModBusTxEnablePin); // Habilita a recep��o do driver RS485 se necess�rio
	UCSRB &= ~(1 << TXCIE);						 // desabilita a interrup��o de final de transmiss�o
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrup��o de recep��o de caractere
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(USART_RXC_vect)
{
	if(ModBus.status==aguardandoResposta) // est� aguardado dados
	{
		ModBus.rxbuf[ModBus.rxpt] = UDR; // recebe o byte
		if(ModBus.rxpt==ModBus.rxsize-1) // pacote completo
		{
			
			modBusTimeoutCounterStop();
			ModBus.status=respostaRecebida;
			ModBusProcess(); // processa a resposta
		}
		if(ModBus.rxpt<tam_buff_recep) ModBus.rxpt++; // incrementa o ponteiro de recep��o se o tamanho n�o chegou no limite
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrup��o de caractere transmitido
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(USART_UDRE_vect) // interrup��o de caractere transmitido
{
	if(ModBus.txpt>=ModBus.txsize) // se transmitiu o ultimo caractere do pacote
	{
		ModBus.rxpt=0;
		modBusTimeoutCounterStart();
		ModBus.status=aguardandoResposta;
		UCSRB &= ~(1 << UDRIE); // desabilita a interrup��o e transmiss�o
		UCSRB |= (1 << TXCIE); // habilita a interrup��o de fim de transmiss�o da serial
	}
	else // se ainda n�o � o ultimo byte do pacote
	{
		UDR = ModBus.txbuf[ModBus.txpt]; // transmite o byte
		ModBus.txpt++; // incrementa o ponteiro de transmiss�o
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrup��o do temporizador 2
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(TIMER2_COMP_vect) // interrup��o do temporizador
{
	if(modBusTimeoutCounter==modBusTimeout_ms) // estouro de timeout
	{
		ModBusTimeout(); // chama a fun��o que trata a falta de resposta do escravo
		modBusTimeoutCounter++; // incrementa para parar de contar
	}
	if(modBusTimeoutCounter<modBusTimeout_ms) modBusTimeoutCounter++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Inicializa a comunica��o serial
//////////////////////////////////////////////////////////////////////////////////////////////////
void usart_init() // inicia a comunica��o serial
{
	UCSRB |= (1 << RXEN) | (1 << TXEN); // Turn on the transmission and reception circuitry
	#if STOPBITS == 2
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1) | (1 << USBS);  // Use 8-bit character sizes 2 stop bits
	#else
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);  // Use 8-bit character sizes 1 stop bit
	#endif

	UBRRL = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register
	UBRRH = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register

	UCSRB |= (1 << RXCIE); // Enable the USART Recieve Complete interrupt (USART_RXC)
	ModBusTxEnableDDR |= (1<<ModBusTxEnablePin); // define como sa�da o pino do driver RS485
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//				F I M - M o d B u s
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//				S P I - M A S T E R
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

int SPI_TX_pt=0;

//////////////////////////////////////////////////////////////////////////////////////////////////
//				SPI Master Init
//////////////////////////////////////////////////////////////////////////////////////////////////
void SPI_MasterInit()
{
	// Set MOSI and SCK output, all others input
	DDRB |= (1<<PB2)|(1<<PB3)|(1<<PB5);
	// Enable SPI, Master, set clock /64
	// SPCR -> SPIE SPE DORD MSTR CPOL CPHA SPR1 SPR0
	SPCR = (1<<SPIE)|(1<<SPE)|(1<<MSTR)|(1<<CPHA)|(1<<SPR1)|(1<<SPR0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//				Interrup��o SPI
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR (SPI_STC_vect)
{
	if(SPI_TX_pt<SPI_data_size)
	{
		data.SPI_RX_data[SPI_TX_pt-1]=SPDR;
		_delay_us(20);
		SPDR = SPI_TX_data[SPI_TX_pt];
	}
	if(SPI_TX_pt==SPI_data_size)
	{
		data.SPI_RX_data[SPI_TX_pt-1]=SPDR;
		PORTB|=(1<<PB2); //Desativa /SS
	}
	SPI_TX_pt++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//				SPI Start Transmission
//////////////////////////////////////////////////////////////////////////////////////////////////
void SPI_StartTransmission()
{
	PORTB&=~(1<<PB2); //Ativa /SS
	SPDR = SPI_TX_data[0];
	SPI_TX_pt=1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//				F I M   S P I - M A S T E R
//////////////////////////////////////////////////////////////////////////////////////////////////


void erro()
{
	for(int c=0; c<10; c++)
	{
		Led_on();
		_delay_ms(50);
		Led_off();
		_delay_ms(50);
	}	
}
int main(void)
{
    usart_init();			// inicia a comunica��o serial utilizada na ModBus
    ModBusReset();			// prepara para receber a transmiss�o
    inicia_timer_1ms();		// configura timer 2
	SPI_MasterInit();		// inicia a SPI
    DDRB |= (1<<PB0);		//led Verde
	PORTD |= (1<<PD0);		// habilita pullup RX
	
    sei();
    while (1) 
    {
		// Processamento da ModBus
		if(ModBus.status==inativo)
		{
			SPI_StartTransmission(); // inicia a transa��o na SPI
			_delay_ms(500);
			//modBusReadCoilStatusFC01(24, 1, &bitData[24]);
			//modBusReadHoldingRegistersFC03(10, 22, wordData);
			//modBusForceSingleCoilFC05(1, bitData[0]);
			//modBusPresetSingleRegisterFC06(12,11223);
			/*bitData[0]=1;
			bitData[1]=0;
			bitData[2]=0;
			bitData[3]=1;
			bitData[4]=1;
			bitData[5]=0;
			bitData[6]=0;
			bitData[7]=1;
			bitData[8]=0;
			bitData[9]=1;
			modBusForceMultipleCoilsFC15(0, 10, &bitData[0]);
			data.wordData[0]=10;
			data.wordData[1]=11;
			data.wordData[2]=12;
			data.wordData[3]=13;
			data.wordData[4]=14;*/
			modBusPresetMultipleRegistersFC16(0, 7, &data.wordData[0]);
		}
		if(ModBus.status==respostaRecebida)
		{
			if(ModBus.erro==semErro)
			{
				// processa os dados lidos
				Led_on();
				_delay_ms(250);
				Led_off();
			}
			else
			{
				// trata o erro de leitura
				
			}
			ModBusReset(); // prepara para a pr�xima transmiss�o
		}
		if(ModBus.status==semResposta)
		{
			// trata o erro sem Resposta ou resposta muito curta
			// pode ser um c�digo de erro...
			ModBusReset(); // prepara para a pr�xima transmiss�o
			erro();
		}
		// Fim processamento da ModBus 
    }
}

