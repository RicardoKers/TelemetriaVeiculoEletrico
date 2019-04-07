
#include <avr/io.h>
#include <avr/interrupt.h>
#include "TextLCD.h"

int contador;

ISR(INT0_vect) {
	//if((PIND&0b00010000)!=0)
	contador++;
	//else
	//contador--;
}

ISR(INT1_vect) {
	//if((PIND&0b00010000)!=0)
	//contador++;
	//else
	contador--;
}

int main(void)
{
	unsigned int y=0;
	
	contador=10;
	
	LCD_init(); // inicializa LCD 20x4 (TextLCD.h)
	
	especial_char(0,0b01001010,0b10010000,0b00000001,0b10010000,0b00000001,0b10010000,0b10101010,0b11111111);

	
	EICRA = 0b00000101;
	EIMSK = 0b00000011;
	sei(); // ativa todas as interrupç?es
	
	while(1)
	{
		gotoxy(2,0);
		putstr("Contador = ");
		putuint(contador);
		putstr("      ");
		
		gotoxy(2,1);
		putstr("y = ");
		putuint(y);
		putstr("      ");
		y++;
		if(y==100)
		{
			clr_lcd();
			LCD_delay(65000);
			LCD_putchar(0);
			LCD_delay(65000);
		}
		LCD_delay(199);
	}
}

