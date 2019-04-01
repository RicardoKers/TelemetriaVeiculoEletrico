
#include <avr/io.h>
#include <avr/interrupt.h>
#include "LCD_Ricardo.h"

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
	
	LCD_init(); // inicializa LCD 20x4 (LDC_Ricardo.h)
	
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
		delay(199);
	}
}

