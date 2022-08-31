/*
 * IHM.c
 * ATmega328P
 * Created: 31/05/2022 10:09:39
 * Author : Kerschbaumer
 * Versão 2.0
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 8000000UL
#include <util/delay.h>
#include "ff.h"
#include "diskio.h"
#include "TextLCD.h"
#include "LcdBigNumber.h"
#include "rtc.h"

// definições


// variáveis globais


char telaAtiva=0;

FATFS FileSystemObject;
FIL logFile;
FRESULT R;
DSTATUS driveStatus;
rtc_t rtc; 

unsigned int tempo_10ms;
unsigned int tempo_1s;

char hora[9];
char str[6];
char sdok;

unsigned int Vbat;
unsigned int Tbat;
unsigned int Ibat;
unsigned int velocidade;
unsigned int Tve;

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//					M O D B U S
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

// configuração do pino do driver RS485
#define ModBusTxEnablePort PORTB
#define ModBusTxEnablePin 2
#define ModBusTxEnableDDR DDRB

#define ModBusRxEnable() ModBusTxEnablePort&=~(1<<ModBusTxEnablePin) // Habilita a recepção do driver RS485
#define ModBusTxEnable() ModBusTxEnablePort|=(1<<ModBusTxEnablePin) // Habilita a transmissão do driver RS485

unsigned int ModBusTimerCont = 1;
unsigned int ModBusTimerInterval = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Liga o temporizador usado na modBus com o intervalor ajustado para 1ms
//	Ajustar para o clock utilizado
//////////////////////////////////////////////////////////////////////////////////////////////////
void  inicia_timer_1ms()
{
	OCR2A=193;			// Ajusta o valor de comparação do timer 2
	TCNT2=0;			// Zera a contagem do timer 2
	TCCR2B=0b00000011;	// habilita o clock do timer 2 com prescaller
	TIMSK2|=0b00000010;	// habilita a interrupção do timer 2
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//	Liga o temporizador usado na modBus, deve ficar antes do #include "ModBusSlave.h"
//////////////////////////////////////////////////////////////////////////////////////////////////
extern void  liga_timer_modbus(unsigned char t_ms)
{
	cli();
	ModBusTimerCont=0;
	ModBusTimerInterval=t_ms;
	inicia_timer_1ms();
	sei();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	desliga o temporizador usado na modBus
//////////////////////////////////////////////////////////////////////////////////////////////////
#define desliga_timer_modbus() TCCR2B=0b00000000 // desliga o timer da modbus

//////////////////////////////////////////////////////////////////////////////////////////////////
//	reseta o temporizador usado na modBus
//////////////////////////////////////////////////////////////////////////////////////////////////
#define reset_timer_modbus() TCNT2 = 0 // reseta o timer da modbus

#include "ModBusSlave.h"

// configuração da serial
#define USART_BAUDRATE 19200 // taxa de transmissão desejada
#define BAUD_PRESCALE ((F_CPU/ (USART_BAUDRATE * (long)16))-1) //calcula o valor do prescaler da usart
// fim da configuração da serial

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção de recepção de caractere
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(USART_RX_vect)
{
	cli();
	ModBus.rxbuf[ModBus.rxpt] = UDR0; // recebe o byte
	reset_timer_modbus(); // reseta o timer da modbus
	if(ModBus.status==aguardando && ModBus.rxpt==0) // primeiro byte do pacote
	{
		liga_timer_modbus(1); // liga o timer para detectar pacotes truncados
	}

	if(ModBus.status==aguardando && ModBus.rxpt==6) // recebe o começo do pacote
	{
		if(ModBus.rxbuf[0]==ModBus.end_modbus) //se o endereço confere inicia a recepção
		{
			ModBus.status=recebendo;
			ModBusDefineFunction(ModBus.rxbuf[1]); // seta a função
		}
		else // senão ignora o pacote
		{
			ModBus.status=ignorando;
			ModBus.rxsize = tam_buff_recep; // tamanho máximo
		}
	}
	if(ModBus.status==recebendo && ModBus.rxpt==ModBus.rxsize) // recebeu o restante do pacote
	{
		desliga_timer_modbus(); // desliga o timer da modbus
		ModBus.status = processando;
	}
	if(ModBus.rxpt<tam_buff_recep) ModBus.rxpt++; // incrementa o ponteiro de recepção se o tamanho não chegou no limite
	sei();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção de fim de transmissão
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(USART_TX_vect)
{
	cli();
	UCSR0B &= ~(1 << TXCIE0);	// desabilita a interrupção de final de transmissão
	ModBusReset();				// prepara para receber nova transmissão
	sei();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção de caractere transmitido
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(USART_UDRE_vect) // interrupção de caractere transmitido
{
	cli();
	if(ModBus.txpt==ModBus.txsize-1) // se transmitiu o penultimo caractere do pacote
	{
		UDR0 = ModBus.txbuf[ModBus.txpt]; // transmite o ultimo byte
		UCSR0B &= ~(1 << UDRIE0); // desabilita a interrupção e transmissão
		UCSR0B |= (1 << TXCIE0); // abilita a interrupção de final de transmissão
	}
	else // se ainda não é o ultimo byte do pacote
	{
		UDR0 = ModBus.txbuf[ModBus.txpt]; // transmite o byte
		ModBus.txpt++; // incrementa o ponteiro de transmissão
	}
	sei();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Interrupção do temporizador
//////////////////////////////////////////////////////////////////////////////////////////////////
ISR(TIMER2_COMPA_vect) // interrupção do temporizador
{
	cli();
	if(ModBusTimerCont<ModBusTimerInterval) ModBusTimerCont++;
	if(ModBusTimerCont==ModBusTimerInterval) // intervalo finalizado
	{
		ModBusTimerCont++;
		if(ModBus.status==iniciandoTransmisao)
		{
			ModBusTxEnable(); // Habilita a transmissão do driver 485 se necessário
			ModBus.status = transmitindo; // indica que está transmitindo
			desliga_timer_modbus(); // desliga o timer da modbus
			UDR0 = ModBus.txbuf[ModBus.txpt]; // transmite o primeiro byte, os seguintes são transmitidos na interrupção da serial
			UCSR0B |= (1 << UDRIE0); // habilita a interrupção da serial
			ModBus.txpt++; // incrementa o ponteiro de transmissção
		}
		
		if(ModBus.status==aguardando || ModBus.status==recebendo|| ModBus.status==ignorando) // se o timer disparou na recepção houve erro
		{
			ModBusReset(); // prepara para receber nova transmissão
			desliga_timer_modbus(); // desliga o timer da modbus
		}
	}
	sei();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Inicializa a comunicação serial
//////////////////////////////////////////////////////////////////////////////////////////////////
void usart_init() // inicia a comunicação serial
{
	UCSR0B = (1<<RXEN0)|(1<<TXEN0); // Turn on the transmission and reception circuitry
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);

	UBRR0L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register
	UBRR0H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register

	UCSR0B |= (1 << RXCIE0); // Enable the USART Recieve Complete interrupt (USART_RXC)

	ModBusTxEnableDDR|=(1<<ModBusTxEnablePin); // Habilita TX do driver RS485 como saída
	ModBusRxEnable(); // Habilita a recepção do driver RS485
}
/////////////////////////////////////////////////////////////////////
//		F I M    M O D B U S
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
//		I N T E R R U P Ç Ã O    T I M E R   0
/////////////////////////////////////////////////////////////////////
ISR(TIMER0_OVF_vect )
{	// interrupção configurada para acontecer a cada 1 ms
	cli();
	TCNT0=248; 
	tempo_1s++;
	// temporização da fat32 do cartão SD
	tempo_10ms++;
	if(tempo_10ms>=102)
	{
		tempo_10ms=0;
		disk_timerproc();
	}
	sei();
}

/////////////////////////////////////////////////////////////////////
//		L E    B O T Õ E S
/////////////////////////////////////////////////////////////////////
int le_botoes()
{
	if((PIND&0b00010000)==0)
	{
		_delay_ms(250);
		return 1;
	}
	if((PIND&0b00001000)==0)
	{
		_delay_ms(250);
		return 2;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////
//		I N T   T O   C H A R 3
/////////////////////////////////////////////////////////////////////
void intToChar3(unsigned int data)
{
	int cont=0;
	
	if(data>99)
	{
		str[cont]=(char)(((data%1000)/100)+48);//3
		cont++;
	}
	else
	{
		str[cont]='0';
		cont++;
	}
	if(data>9)
	{
		str[cont]=(char)(((data%100)/10)+48);//2
		cont++;
	}
	else
	{
		str[cont]='0';
		cont++;
	}
	str[cont]=',';
	cont++;
	str[cont]=(char)((data%10)+48);//1
	cont++;
	str[cont]='\t';
	cont++;
	str[cont]=0;
}

/////////////////////////////////////////////////////////////////////
//		T E L A S
/////////////////////////////////////////////////////////////////////

void tela1() // Tela inicial
{
	gotoxy(9,0);
	if(sdok==1)
	{
		putstr("SDOk");
	}
	else
	{
		putstr("SDER");
	}
	
	RTC_GetDateTime(&rtc);
	hora[0]=(((uint16_t)rtc.hour>>4)+48);
	hora[1]=(((uint16_t)rtc.hour&0x000f)+48);
	hora[3]=(((uint16_t)rtc.min>>4)+48);
	hora[4]=(((uint16_t)rtc.min&0x000f)+48);
	hora[6]=(((uint16_t)rtc.sec>>4)+48);
	hora[7]=(((uint16_t)rtc.sec&0x000f)+48);
	gotoxy(0,0);
	putstr(hora);
	gotoxy(14,0);
	putstr("E=");
	putintdec(Vbat);
	LCD_putchar(' ');
	gotoxy(14,1);
	putstr("V=");
	putintdec(velocidade);
	LCD_putchar(' ');
	gotoxy(14,2);
	putstr("T=");
	putintdec(Tbat);
	LCD_putchar(' ');
	BigNumberDecimal(Ibat, 0, 1);
	gotoxy(11,3);
	putstr("A       1");
}

void tela2() // Tela de velocidade
{
	gotoxy(9,0);
	if(sdok==1)
	{
		putstr("SDOk");
	}
	else
	{
		putstr("SDER");
	}
	
	RTC_GetDateTime(&rtc);
	hora[0]=(((uint16_t)rtc.hour>>4)+48);
	hora[1]=(((uint16_t)rtc.hour&0x000f)+48);
	hora[3]=(((uint16_t)rtc.min>>4)+48);
	hora[4]=(((uint16_t)rtc.min&0x000f)+48);
	hora[6]=(((uint16_t)rtc.sec>>4)+48);
	hora[7]=(((uint16_t)rtc.sec&0x000f)+48);
	gotoxy(0,0);
	putstr(hora);
	gotoxy(14,0);
	putstr("E=");
	putintdec(Vbat);
	LCD_putchar(' ');
	gotoxy(14,1);
	putstr("I=");
	putintdec(Ibat);
	LCD_putchar(' ');
	gotoxy(14,2);
	putstr("T=");
	putintdec(Tbat);
	LCD_putchar(' ');
	BigNumberDecimal(velocidade, 0, 1);
	gotoxy(13,3);
	putstr("Km/h  2");
	if(le_botoes()==2) // volta para tela inicial
	{
		telaAtiva=0;
		clr_lcd();
		_delay_ms(50);
	}
}

void tela3() // informações
{
	RTC_GetDateTime(&rtc);
	hora[0]=(((uint16_t)rtc.hour>>4)+48);
	hora[1]=(((uint16_t)rtc.hour&0x000f)+48);
	hora[3]=(((uint16_t)rtc.min>>4)+48);
	hora[4]=(((uint16_t)rtc.min&0x000f)+48);
	hora[6]=(((uint16_t)rtc.sec>>4)+48);
	hora[7]=(((uint16_t)rtc.sec&0x000f)+48);
	gotoxy(6,0);
	putstr(hora);
	gotoxy(0,1);
	putstr("Vel=");
	putintdec(velocidade);
	LCD_putchar(' ');
	gotoxy(0,2);
	if(sdok==1)
	{
		putstr("SD Ok");
	}
	else
	{
		putstr("SD Erro");
	}
	gotoxy(0,3);
	putstr("Ebat=");
	putintdec(Vbat);
	LCD_putchar(' ');
	gotoxy(10,1);
	putstr("Ibat=");
	putintdec(Ibat);
	LCD_putchar(' ');
	gotoxy(10,2);
	putstr("Tbat=");
	putintdec(Tbat);
	LCD_putchar(' ');
	gotoxy(10,3);
	putstr("Tve=");
	putintdec(Tve);
	LCD_putchar(' ');
	gotoxy(19,3);
	putstr("3");
	gotoxy(17,0);
	putuint(Ibat);
	if(le_botoes()==2) // volta para tela inicial
	{
		telaAtiva=0;
		clr_lcd();
		_delay_ms(50);
	}
}

void tela4() // ajuste da hora
{
	char str[3];
	int tmp;
	gotoxy(0,0);
	putstr("   Ajuste da hora   ");
	RTC_GetDateTime(&rtc);
	str[0]=(((uint16_t)rtc.hour>>4)+48);
	str[1]=(((uint16_t)rtc.hour&0x000f)+48);
	str[2]=0;
	gotoxy(9,1);
	putstr(str);
	if(le_botoes()==2) // incrementa
	{
		tmp=(uint16_t)rtc.hour;
		if(tmp<0x0023)
		{
			if((tmp&0x000F)<0x0009)
			{
				tmp++;
			}
			else
			{
				if((tmp&0x000F)==0x0009)
				{
					tmp=(tmp&0x00F0)+0x0010;
				}
			}
			
		}
		else
		{
			tmp=0;
		}
		rtc.hour=tmp;
		RTC_SetDateTime(&rtc);
	}
	//gotoxy(9,2);
	//putuint(tempo_velocidade);
	//putstr("   ");
	gotoxy(19,3);
	putstr("4");
}

void tela5() // ajuste dos minutos
{
	char str[3];
	int tmp;
	gotoxy(0,0);
	putstr(" Ajuste dos minutos ");
	RTC_GetDateTime(&rtc);
	str[0]=(((uint16_t)rtc.min>>4)+48);
	str[1]=(((uint16_t)rtc.min&0x000f)+48);
	str[2]=0;
	gotoxy(9,1);
	putstr(str);
	if(le_botoes()==2) // incrementa
	{
		tmp=(uint16_t)rtc.min;
		if(tmp<0x0059)
		{
			if((tmp&0x000F)<0x0009)
			{
				tmp++;
			}
			else
			{
				if((tmp&0x000F)==0x0009)
				{
					tmp=(tmp&0x00F0)+0x0010;
				}
			}
			
		}
		else
		{
			tmp=0;
		}
		rtc.min=tmp;
		RTC_SetDateTime(&rtc);
	}
	gotoxy(19,3);
	putstr("5");
}

void tela6() // ajuste dos segundos
{
	char str[3];
	int tmp;
	gotoxy(0,0);
	putstr("Ajuste dos segundos ");
	RTC_GetDateTime(&rtc);
	str[0]=(((uint16_t)rtc.sec>>4)+48);
	str[1]=(((uint16_t)rtc.sec&0x000f)+48);
	str[2]=0;
	gotoxy(9,1);
	putstr(str);
	if(le_botoes()==2) // incrementa
	{
		tmp=(uint16_t)rtc.sec;
		if(tmp<0x0059)
		{
			if((tmp&0x000F)<0x0009)
			{
				tmp++;
			}
			else
			{
				if((tmp&0x000F)==0x0009)
				{
					tmp=(tmp&0x00F0)+0x0010;
				}
			}
			
		}
		else
		{
			tmp=0;
		}
		rtc.sec=tmp;
		RTC_SetDateTime(&rtc);
	}
	gotoxy(19,3);
	putstr("6");
}

void inicializa_sd()
{
	unsigned int bytesWritten=0;
	
	if(f_mount(0, &FileSystemObject)!=FR_OK)
	{
		gotoxy(0,1);
		sdok=0;
		putstr("E: 1 ");
		_delay_ms(1000);
	}
	
	driveStatus = disk_initialize(0);
	
	if(driveStatus!=0)
	{
		gotoxy(0,2);
		sdok=0;
		putstr("E: 2>");
		putint(driveStatus);
		putstr("< ");
		_delay_ms(1000);
	}

	if((R=f_open(&logFile, "dados.txt", FA_WRITE))!=FR_OK) // Abre ou cria o arquivo
	{
		LCD_putchar('2');
		_delay_ms(100);
		gotoxy(0,3);
		sdok=0;
		putstr("E: 3>");
		putint(R);
		putstr("< ");
		_delay_ms(1000);
	}
	else
	{
		f_lseek (&logFile,f_size (&logFile)); //aponta para o final do arquivo
		RTC_GetDateTime(&rtc);
		hora[0]=(((uint16_t)rtc.hour>>4)+48);
		hora[1]=(((uint16_t)rtc.hour&0x000f)+48);
		hora[3]=(((uint16_t)rtc.min>>4)+48);
		hora[4]=(((uint16_t)rtc.min&0x000f)+48);
		hora[6]=(((uint16_t)rtc.sec>>4)+48);
		hora[7]=(((uint16_t)rtc.sec&0x000f)+48);
		f_write(&logFile, "Iniciando nova telemetria as ", 29, &bytesWritten);
		f_write(&logFile, hora, 8, &bytesWritten);
		f_write(&logFile, "\n", 1, &bytesWritten);
		f_close(&logFile);
	}
}

void atualiza_tela()
{
	switch(telaAtiva)
	{
		case 0:
		tela1();
		break;
	
		case 1:
		tela2();
		break;
	
		case 2:
		tela3();
		break;
	
		case 3:
		tela4();
		break;
	
		case 4:
		tela5();
		break;
	
		case 5:
		tela6();
		break;
	
		default:
		tela1();
	}
	if(le_botoes()==1)
	{
		if(telaAtiva<5) telaAtiva++;
		else telaAtiva=0;
		clr_lcd();
		_delay_ms(50);
	}
}

int main(void)
{
    int cont=0;
    sdok=1;
	unsigned int bytesWritten=0;
	
    DDRD|=0B01000000; // define o r/w do display como saída com valor 0
    PORTD|=0b00011000; // habilita o pull up dos botões
    DDRC|=0b00001000; // cs como saída
    PORTC|=0b00001000; // cs em 1
	DDRD|=0b00000010; // TXD como saída
    PORTD|=0B00000001; // habilita o pullup no RXD

	hora[2]=(':');
	hora[5]=(':');
	hora[8]=0;
	
	LCD_init();
	BigNumberInit();
	usart_init(); // inicia a comunicação serial utilizada na ModBus
	ModBusReset(); // prepara para receber a transmissão
	
	gotoxy(0,0);
	putstr("  Inicializando...");
	gotoxy(0,1);
	putstr("[");
	
	tempo_1s=0; // para contagem de tempo com o temporizador
	tempo_10ms=0; // para contagem de tempo com o temporizador
	
	// configura o timer 0 para aproximadamente 1 ms (1/1024 s)
	TCCR0B=0b00000101; // Habilita o timer 0 com prescaler 1024
	TCNT0=248; // inicia a contagem em 158
	TIMSK0 |=1; // Habilita a interrupção do timer 0
	
	sei(); // ativa todas as interrupções
	
	for(cont=0; cont<9; cont++)
	{
		LCD_putchar(1);
		_delay_ms(100);
	}
	//inicializa o RTC
	RTC_Init();

	// inicializa cartão SD
	inicializa_sd();
	
	gotoxy(9,1);
	for(cont=9; cont<18; cont++)
	{
		LCD_putchar(1);
		_delay_ms(100);
	}
	LCD_putchar(']');
	clr_lcd();
    ///////////////////////////////////////////////////
	// L O O P
	///////////////////////////////////////////////////
	while (1) 
    {
		if(ModBus.status == processando)
		{
			ModBusProcess(); // inicia o processamento do pacote
		}
	
		atualiza_tela();
	
		if(tempo_1s>1000 && sdok==1)
		{
			tempo_1s=0;
			// armazena os dados no SD de 1 em 1 segundos
			f_open(&logFile, "/dados.txt", FA_WRITE);
			f_lseek (&logFile,f_size (&logFile)); //aponta para o final do arquivo
	    
			RTC_GetDateTime(&rtc);
			hora[0]=(((uint16_t)rtc.hour>>4)+48);
			hora[1]=(((uint16_t)rtc.hour&0x000f)+48);
			hora[3]=(((uint16_t)rtc.min>>4)+48);
			hora[4]=(((uint16_t)rtc.min&0x000f)+48);
			hora[6]=(((uint16_t)rtc.sec>>4)+48);
			hora[7]=(((uint16_t)rtc.sec&0x000f)+48);
	    
			f_write(&logFile, hora, 8, &bytesWritten);
			intToChar3(Tve);
			f_write(&logFile,"\t", 1, &bytesWritten);
			f_write(&logFile,str, 5, &bytesWritten);
			intToChar3(Tbat);
			f_write(&logFile,str, 5, &bytesWritten);
			intToChar3(Ibat);
			f_write(&logFile,str, 5, &bytesWritten);
			intToChar3(Vbat);
			f_write(&logFile,str, 5, &bytesWritten);
			intToChar3(velocidade);
			f_write(&logFile,str, 5, &bytesWritten);
			f_write(&logFile,"\n", 1, &bytesWritten);
			f_close(&logFile);
		}
		// conversão das entradas
		//Tve=321;
		// sinais do display
		Tbat=ModBus.data_reg[1]/10; //Temperatura da bateria LM35
		Ibat=ModBus.data_reg[3]; // corrente da bateria
		Vbat=ModBus.data_reg[0]/100; // tensão da bateria
		velocidade=ModBus.data_reg[2]; // velocidade
    }
}
