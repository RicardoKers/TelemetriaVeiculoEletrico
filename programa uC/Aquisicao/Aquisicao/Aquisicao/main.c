/*
 * Aquisicao.c
 * Aquisição da dados, Telemetria veículo Elétrico SEM
 * Microcontrolador ATMega8
 * Created: 18/04/2022 09:01:59
 * Author : Kerschbaumer
 * Versão 1.0
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000 // Clock do microcontrolador necessário para serial e delay
#include <util/delay.h>

// definições
#define Led_on()  PORTB |= (1<<PB0);
#define Led_off()  PORTB &= ~(1<<PB0);
#define SPI_data_size 14
#define DiamRodaMM 1545

// variáveis globais

uint16_t wordData[20];
uint16_t displayData[4];
unsigned int tempo_velocidade_raw=65000;
unsigned int tempo_velocidade=65000;

union u_data
{
	uint16_t SPI_wordData[7];
	uint8_t SPI_RX_data[14];
} data;

uint8_t bitData[100];
unsigned char  SPI_TX_data[SPI_data_size]; // dados a serem transmitidos

unsigned int ADC_channel;
char ADC_scanFiniched=0;
unsigned int ADC_data[8];

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//				M o d B u s
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
// configuração do pino do driver RS485
#define ModBusTxEnablePort PORTD
#define ModBusTxEnablePin PD4
#define ModBusTxEnableDDR DDRD

// configuração da serial
#define USART_BAUDRATE 19200	// taxa de transmissão desejada
#define STOPBITS 2				// número de bits de parada 1 ou 2
#define BAUD_PRESCALE ((F_CPU/ (USART_BAUDRATE * (long)16))-1) //calcula o valor do prescaler da usart
// fim da configuração da serial

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Inicia transmissão
//////////////////////////////////////////////////////////////////////////////////////////////////
void iniciaTransmissao(uint8_t dado) // deve vir antes do #include "ModBusMaster.h"
{
	ModBusTxEnablePort|=(1<<ModBusTxEnablePin); // Habilita a transmissão do driver 485 se necessário
	UDR = dado;									// transmite o primeiro byte, os seguintes são transmitidos na interrupção da serial
	UCSRB |= (1 << UDRIE);						// habilita a interrupção da serial
}

#include "ModBusMaster.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Liga o temporizador usado na modBus com o intervalor ajustado para 1ms
//	Ajustar para o clock utilizado
//////////////////////////////////////////////////////////////////////////////////////////////////
void  inicia_timer_1ms()
{
	OCR2=124;			// Ajusta o valor de comparação do timer 2
	TCNT2=0;			// Zera a contagem do timer 2
	TCCR2=0b00001101;	// habilita o clock do timer 2 com prescaller
	TIMSK|=0b10000000;	// habilita a interrupção do timer 2
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção de fim de transmissão
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(USART_TXC_vect) 
{
	modBusTimeoutCounterStart();
	ModBus.status=aguardandoResposta;
	ModBusTxEnablePort&=~(1<<ModBusTxEnablePin); // Habilita a recepção do driver RS485
	UCSRB &= ~(1 << TXCIE);						 // desabilita a interrupção de final de transmissão
	UCSRB |= (1 << RXCIE);						 // Habilita a interrupção de recepção}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção de recepção de caractere
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(USART_RXC_vect)
{
	if(ModBus.status==aguardandoResposta)			  // está aguardado dados
	{
		ModBus.rxbuf[ModBus.rxpt] = UDR;			  // recebe o byte
		if(ModBus.rxpt==ModBus.rxsize-1)			  // pacote completo
		{
			modBusTimeoutCounterStop();
			ModBus.status=respostaRecebida;
			UCSRB &= ~(1 << RXCIE);					  // Desabilita a interrupção de recepção
		}
		if(ModBus.rxpt<tam_buff_recep) ModBus.rxpt++; // incrementa o ponteiro de recepção se o tamanho não chegou no limite
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção de caractere transmitido
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(USART_UDRE_vect) // interrupção de caractere transmitido
{
	if(ModBus.txpt>=ModBus.txsize) // se transmitiu o ultimo caractere do pacote
	{
		ModBus.rxpt=0;
		UCSRB &= ~(1 << UDRIE); // desabilita a interrupção e transmissão
		UCSRB |= (1 << TXCIE); // habilita a interrupção de fim de transmissão da serial
	}
	else // se ainda não é o ultimo byte do pacote
	{
		UDR = ModBus.txbuf[ModBus.txpt]; // transmite o byte
		ModBus.txpt++; // incrementa o ponteiro de transmissão
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção do temporizador 2
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(TIMER2_COMP_vect) // interrupção do temporizador
{
	if(modBusTimeoutCounter==modBusTimeout_ms) // estouro de timeout
	{
		ModBusTimeout(); // chama a função que trata a falta de resposta do escravo
		modBusTimeoutCounter++; // incrementa para parar de contar
	}
	if(modBusTimeoutCounter<modBusTimeout_ms) modBusTimeoutCounter++;
	
	// tempo para cálculo da velocidade
	if(tempo_velocidade_raw<5000) tempo_velocidade_raw++;
	else tempo_velocidade=60000; // veículo parado	
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Inicializa a comunicação serial
//////////////////////////////////////////////////////////////////////////////////////////////////
void usart_init() // inicia a comunicação serial
{
	UCSRB |= (1 << RXEN) | (1 << TXEN); // Turn on the transmission and reception circuitry
	#if STOPBITS == 2
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1) | (1 << USBS);  // Use 8-bit character sizes 2 stop bits
	#else
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);  // Use 8-bit character sizes 1 stop bit
	#endif

	UBRRL = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register
	UBRRH = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register

	ModBusTxEnableDDR |= (1<<ModBusTxEnablePin); // define como saída o pino do driver RS485
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
//				Interrupção SPI
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

//////////////////////////////////////////////////////////////////////////////////////////////////
//				A D C    I N I T
//////////////////////////////////////////////////////////////////////////////////////////////////
void ADC_init()
{
	// ADMUX -> REFS1 REFS0 ADLAR – MUX3 MUX2 MUX1 MUX0
	ADMUX = (1<<REFS0); // Reference at AVcc, channel 0
	// ADCSRA -> ADEN ADSC ADFR ADIF ADIE ADPS2 ADPS1 ADPS0
	// ADC = On, interrupt = ON, prescaler /128
	ADCSRA = (1<<ADEN) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//				A D C    S T A R T 
//////////////////////////////////////////////////////////////////////////////////////////////////
void ADC_startConversion()
{
	ADC_channel=0;
	ADMUX = (1<<REFS0); // channel 0
	ADCSRA |= (1<<ADSC);
	ADC_scanFiniched=0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//				A D C    I N T E R R U P T
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR (ADC_vect)
{
	ADC_data[ADC_channel] = ADC_data[ADC_channel]+ADCW;
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
		ADMUX |= 5; 
		ADCSRA |= (1<<ADSC);
		break;
		
		case 5:
		ADC_channel=6;
		ADMUX |= 6;
		ADCSRA |= (1<<ADSC);
		break;

		case 6:
		ADC_channel=7;
		ADMUX |= 7;
		ADCSRA |= (1<<ADSC);
		break;

		case 7:
		ADC_channel=0;
		ADC_scanFiniched=1;
		break;

		default:
		ADC_channel=0;
		ADMUX = (1<<REFS0); // channel 0
	}
}

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

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção externa 1
//////////////////////////////////////////////////////////////////////////////////////////////////

ISR(INT1_vect)
{
	tempo_velocidade=tempo_velocidade_raw;
	tempo_velocidade_raw=0;
}

int main(void)
{
	// variáveis locais
	char sleveSelect=0;		// seleciona se transmite para o volante (1) ou para o box (0)
	uint16_t contadorloops = 0;
	// inicializações
    usart_init();			// inicia a comunicação serial utilizada na ModBus
    ADC_init();				// inicia o conversor A/D
	ModBusReset();			// prepara para receber a transmissão
    inicia_timer_1ms();		// configura timer 2
	SPI_MasterInit();		// inicia a SPI
    DDRB |= (1<<PB0);		// led Verde
	PORTD |= (1<<PD0);		// habilita pullup RX
	
	// interrupção da entrada de velocidade
	MCUCR|=0b00001100; // int1 na subida do sinal
	GICR|=0b10000000;  // ativa int1
		
	sei();
    while (1) 
    {
		// Processamento da ModBus
		if(ModBus.status==inativo)
		{
			if(sleveSelect==0) // transmitindo para o box
			{
				sleveSelect=1;
				SPI_StartTransmission(); // inicia a transação na SPI
				_delay_ms(50);
				ADC_data[0] = 0;
				ADC_data[1] = 0;
				ADC_data[2] = 0;
				ADC_data[3] = 0;
				ADC_data[4] = 0;
				ADC_data[5] = 0;
				ADC_data[6] = 0;
				ADC_data[7] = 0;
				for(int cont=0; cont<49; cont++)
				{
					ADC_startConversion();
					while(ADC_scanFiniched==0)
					{
					
					}
				}
				// dados transmitidos pela modbus
				wordData[0]=data.SPI_wordData[1];						// Vin1
				wordData[1]=data.SPI_wordData[0]-data.SPI_wordData[1];  // Vin2
				wordData[2]=ADC_data[6]-data.SPI_wordData[0];		    // Vin3
				wordData[3]=ADC_data[7]-ADC_data[6];					// Vin4
				wordData[4]=ADC_data[0]-ADC_data[7];					// Vin5
				wordData[5]=ADC_data[1]-ADC_data[0];					// Vin6
				wordData[6]=ADC_data[2]-ADC_data[1];					// Vin7
				wordData[7]=ADC_data[3]-ADC_data[2];					// Vin8
				wordData[8]=ADC_data[4]-ADC_data[3];					// Vin9
				wordData[9]=ADC_data[5]-ADC_data[4];					// Vin10		
				wordData[10]=data.SPI_wordData[5];						// Vin11
				wordData[11]=data.SPI_wordData[6];						// Vin12
				wordData[12]=data.SPI_wordData[4];						// Temperatura
				unsigned int tmpVal = (((long)DiamRodaMM*(long)36)/(long)tempo_velocidade); // Velocidade
				if (tmpVal < 350) wordData[13]=tmpVal;
				
				// ajustar conforme características de cada placa
				if(data.SPI_wordData[2]<=2290) wordData[14]=0;	//placa 1
				else wordData[14]=(data.SPI_wordData[2]-2290)/362; // Iin1
				
				//if(data.SPI_wordData[2]<=2030) wordData[14]=0; // placa2
				//else wordData[14]=(data.SPI_wordData[2]-2030)/355; // Iin1
				
				wordData[15]=data.SPI_wordData[3]; // Temperatura Veículo
				
				if(data.SPI_wordData[6] > 10) tmpVal = data.SPI_wordData[6]; // tensão total bateria
				else tmpVal = ADC_data[5];
				tmpVal = ((long)tmpVal*49)/50;
				wordData[16]=tmpVal;
				
				contadorloops++;
				wordData[19]=contadorloops;
				
				displayData[0]=wordData[16];
				displayData[1]=wordData[12];
				displayData[2]=wordData[13];
				displayData[3]=wordData[14];
													
				ModBus.end_modbus=1; // supervisório
				modBusPresetMultipleRegistersFC16(0, 20, &wordData[0]);
			}
			else
			{
				sleveSelect=0;
				_delay_ms(100);
				ModBus.end_modbus=2; // display do volante
				modBusPresetMultipleRegistersFC16(0, 4, displayData);
			}
		}
		if(ModBus.status==respostaRecebida)
		{
			ModBusProcess(); // processa a resposta
			if(ModBus.erro==semErro)
			{
				// processa os dados lidos				
				Led_on();
			}
			else
			{
				// trata o erro de leitura
				Led_off();
				_delay_ms(100);
				Led_on();
			}
			ModBusReset(); // prepara para a próxima transmissão
		}
		if(ModBus.status==semResposta)
		{
			// trata o erro sem Resposta ou resposta muito curta
			// pode ser um código de erro...
			ModBusReset(); // prepara para a próxima transmissão
			Led_off();
			_delay_ms(100);
			Led_on();
		}
		// Fim processamento da ModBus 
    }
}

/*
Dados transmitidos pela modbus
wordData[0] = Vin1(mV)
wordData[1] = Vin2(mV)
wordData[2] = Vin3(mV)
wordData[3] = Vin4(mV)
wordData[4] = Vin5(mV)
wordData[5] = Vin6(mV)
wordData[6] = Vin7(mV)
wordData[7] = Vin8(mV)
wordData[8] = Vin9(mV)
wordData[9] = Vin10(mV)
wordData[10] = Vin11(mV)
wordData[11] = Vin12(mV)
wordData[12] = Temperatura °C *100 Bateria
wordData[13] = Velocidade Km/h *10
wordData[14] = Iin1 (mA)
wordData[15] = Temperatura °C *100 Veículo
wordData[16] = Tensão da bateria (mV)
wordData[17] =
wordData[18] =
wordData[19] = Contador

MQTT taxa 1s
tópicos
/tve/tempbat °C *100 Bateria
/tve/velocidade Km/h *10 (gráfico)
/tve/corrente (mA) (gráfico)
/tve/tempvei °C *100 Veículo
/tve/tensaobat (mV)

*/

