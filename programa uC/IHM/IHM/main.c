#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 8000000UL  // 8 MHz (for delay.h)
#include <util/delay.h>
//#include "ff.h"
//#include "diskio.h"
#include "TextLCD.h"
#include "rtc.h"

rtc_t rtc;

// parâmetros 
#define numero_parametros 10
int parametros[numero_parametros];
char texto_parametros[numero_parametros][21]={"Parametro 0         \0",
											  "Parametro 1         \0",
											  "Parametro 2         \0",
											  "Parametro 3         \0",
											  "Parametro 4         \0",
											  "Parametro 5         \0",
											  "Parametro 6         \0",
											  "Parametro 7         \0",
											  "Parametro 8         \0",
											  "Parametro 9         \0"};

// variáveis para manipulação do encoder
char encoder_incrementa;
char encoder_decrementa;
char encoder_click;

// variáveis para controle das telas
char tela_ativa;
char tela_anterior;
char espacos[]={"                    "};

unsigned int contador;
unsigned int tempo_1s;
unsigned int tempo_10ms;
unsigned int y;

ISR(INT0_vect)
{
	if((PINB&0b00000001)==0)
	{
		encoder_incrementa=1;
	}
	else
	{
		encoder_decrementa=1;
	}
}

ISR(INT1_vect)
{
		encoder_click=1;
}

ISR(TIMER0_OVF_vect )
{	// interrupção configurada para acontecer a cada 1 ms
	TCNT0=248; // reinicia a contagem em 158
	tempo_1s++;
	if(tempo_1s>=1024)
	{
		y++;
		tempo_1s=0;
	}
	
	// temporização da fat32 do cartão SD
	tempo_10ms++;
	if(tempo_10ms>=102)
	{
		tempo_10ms=0;
		//disk_timerproc();
	}
}void tela0(){	char hora[9];
	hora[2]=(':');
	hora[5]=(':');
	hora[8]=0;		RTC_GetDateTime(&rtc);		
	hora[0]=(((uint16_t)rtc.hour>>4)+48);
	hora[1]=(((uint16_t)rtc.hour&0x000f)+48);
	hora[3]=(((uint16_t)rtc.min>>4)+48);
	hora[4]=(((uint16_t)rtc.min&0x000f)+48);
	hora[6]=(((uint16_t)rtc.sec>>4)+48);
	hora[7]=(((uint16_t)rtc.sec&0x000f)+48);
		
	gotoxy(0,0);
	putstr("Tela 0 ");
	putstr(hora);	putstr("     ");	gotoxy(0,1);
	putstr("                    ");	gotoxy(0,2);
	putstr("                    ");	gotoxy(0,3);
	putstr("                   0");
	if(encoder_incrementa!=0)
	{
		encoder_incrementa=0;
		tela_ativa=1;
		tela_anterior=0;
	}
	if(encoder_decrementa!=0)
	{
		encoder_decrementa=0;
	}	if(encoder_click!=0)
	{
		encoder_click=0;
	}}void tela1(){	if(tela_ativa!=tela_anterior) //entrando nesta tela	{			}
	gotoxy(0,0);
	putstr("Ajuste de Parametros");
	gotoxy(0,1);
	putstr(texto_parametros[0]);
	gotoxy(0,2);
	putstr(espacos);
	gotoxy(0,3);
	putstr("                   1");	if(encoder_incrementa!=0)
	{
		encoder_incrementa=0;
		tela_ativa=2;
		tela_anterior=1;
	}
	if(encoder_decrementa!=0)
	{
		encoder_decrementa=0;
		tela_ativa=0;
		tela_anterior=1;
	}	if(encoder_click!=0)
	{
		encoder_click=0;
	}}void tela2(){	if(tela_ativa!=tela_anterior) //entrando nesta tela	{			}	RTC_GetDateTime(&rtc);
	gotoxy(0,0);
	putstr("      tela 2       ");
	gotoxy(0,1);
	putstr("    Minuto atual    ");
	gotoxy(0,2);
	putstr("         ");
	LCD_putchar((((uint16_t)rtc.min>>4)+48));	LCD_putchar(((uint16_t)rtc.min&0x000f)+48);	putstr("         ");	gotoxy(0,3);
	putstr("                   2");	if(encoder_incrementa!=0)
	{
		encoder_incrementa=0;
		tela_ativa=3;
		tela_anterior=2;
	}
	if(encoder_decrementa!=0)
	{
		encoder_decrementa=0;
		tela_ativa=1;
		tela_anterior=2;
	}	if(encoder_click!=0)
	{
		encoder_click=0;
	}}void tela3(){	if(tela_ativa!=tela_anterior) //entrando nesta tela	{			}	RTC_GetDateTime(&rtc);
	gotoxy(0,0);
	putstr("      tela 3       ");
	gotoxy(0,1);
	putstr("   Segundo atual    ");
	gotoxy(0,2);
	putstr("         ");
	LCD_putchar((((uint16_t)rtc.sec>>4)+48));	LCD_putchar(((uint16_t)rtc.sec&0x000f)+48);	putstr("         ");	gotoxy(0,3);
	putstr("                   3");	if(encoder_incrementa!=0)
	{
		encoder_incrementa=0;
		//tela_ativa=2;
		tela_anterior=3;
	}
	if(encoder_decrementa!=0)
	{
		encoder_decrementa=0;
		tela_ativa=2;
		tela_anterior=3;
	}	if(encoder_click!=0)
	{
		encoder_click=0;
	}}void atualiza_tela(){	switch (tela_ativa)
	{
		case 0:
		tela0();
		break;

		case 1:
		tela1();
		break;
		
		case 2:
		tela2();
		break;
		
		case 3:
		tela3();
		break;

		default:
		tela0();
	}}

int main(void)
{

	// iniciaçiza variáveis globais
	encoder_incrementa=0;
	encoder_decrementa=0;
	encoder_click=0;
	
	tela_ativa=0;
	tela_anterior=0;
	
	y=0;	
	contador=10;
	
	LCD_init(); // inicializa LCD 20x4 (TextLCD.h)
	
	gotoxy(0,0);
	putstr("  Inicializando...");
	
	DDRC=0b00110000;
	
	EICRA = 0b00001111;
	EIMSK = 0b00000011;
	
	// configura o timer 0 para aproximadamente 1 ms (1/1024 s)
	TCCR0B=0b00000101; // Habilita o timer 0 com prescaler 1024
	TCNT0=248; // inicia a contagem em 158
	TIMSK0 |=1; // Habilita a interrupção do timer 0
	//////////////
	
	sei(); // ativa todas as interrupções
	
	tempo_1s=0;
	tempo_10ms=0;
	//inicializa o RTC
	RTC_Init();
	/*
	    rtc.hour = 0x14; //  10:40:20 am
	    rtc.min =  0x52;
	    rtc.sec =  0x00;

	    rtc.date = 0x16; //1st Jan 2016
	    rtc.month = 0x05;
	    rtc.year = 0x19;
	    rtc.weekDay = 5; // Friday: 5th day of week considering monday as first day.
	
		RTC_SetDateTime(&rtc);
	*/
	/*
	// inicializa cartão SD
	FATFS FileSystemObject;
	
	if(f_mount(0, &FileSystemObject)!=FR_OK)
	{
		gotoxy(0,1);
		putstr("E: 1 ");
	}

	DSTATUS driveStatus = disk_initialize(0);
	if(driveStatus!=0)
	{
		gotoxy(0,2);
		putstr("E: 2>");
		putint(driveStatus);
		putstr("< ");
	}	
	FIL logFile;
	// testa SD
	FRESULT R;
	if((R=f_open(&logFile, "/teste.txt", FA_READ))!=FR_OK)
	{
		gotoxy(0,3);
		putstr("E: 3>");
		putint(R);
		putstr("< ");
	}
	unsigned int bytesWritten=0;
	//f_lseek (&logFile,f_size (&logFile)); //aponta para o final do arquivo
	//f_write(&logFile, "New log opened!\n", 16, &bytesWritten);
	char str[100];
	f_read (&logFile,str,7,&bytesWritten);
	//Flush the write buffer with f_sync(&logFile);
	gotoxy(8,3);
	str[7]=0;
	putstr(str);
	//Close and unmount.
	f_close(&logFile);
	f_mount(0,0);
	// fim testa sd
	*/
	gotoxy(0,0);
	putstr("                    ");
	while(1)
	{
		atualiza_tela();
		/*
		gotoxy(0,0);
		putstr("C=");
		putuint(contador);
		putstr("  ");
		
		gotoxy(5,0);
		putstr("y=");
		putuint(y);
		putstr("  ");
		encoder_click_ativo=1;
		if(encoder_click!=0)
		{
			encoder_click=0;
			while(encoder_click==0)
			{
				gotoxy(0,1);
				putstr("Aj->c=");
				putuint(contador);
				putstr("  ");
				encoder_giro_ativo=1;
				if(encoder_incrementa!=0)
				{
					encoder_incrementa=0;
					contador++;
				}
				if(encoder_decrementa!=0)
				{
					encoder_decrementa=0;
					contador--;
				}
			}
			encoder_giro_ativo=0;
			encoder_click_ativo=0;
			encoder_click=0;
			gotoxy(0,1);
			putstr("                    ");
		}*/
	}
}

