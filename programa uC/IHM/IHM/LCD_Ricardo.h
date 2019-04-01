// Versão 2019-1

// data pin 4
#define LCD_DATA_PORT4_H PORTB|=0b00000100
#define LCD_DATA_PORT4_L PORTB&=~0b00000100
#define LCD_DATA_DIRECTION4 DDRB|=0b00000100
// data pin 5
#define LCD_DATA_PORT5_H PORTB|=0b00001000
#define LCD_DATA_PORT5_L PORTB&=~0b00001000
#define LCD_DATA_DIRECTION5 DDRB|=0b00001000
// data pin 6
#define LCD_DATA_PORT6_H PORTB|=0b00010000
#define LCD_DATA_PORT6_L PORTB&=~0b00010000
#define LCD_DATA_DIRECTION6 DDRB|=0b00010000
// data pin 7
#define LCD_DATA_PORT7_H PORTB|=0b00100000
#define LCD_DATA_PORT7_L PORTB&=~0b00100000
#define LCD_DATA_DIRECTION7 DDRB|=0b00100000

// E
#define LCD_E_H PORTB|=0b00000010
#define LCD_E_L PORTB&=~0b00000010
#define LCD_E_DIRECTION DDRB|=0b00000010

// RS
#define LCD_RS_H PORTB|=0b00000001
#define LCD_RS_L PORTB&=~0b00000001
#define LCD_RS_DIRECTION DDRB|=0b00000001

//////////////////////////////////////////////////////////////////////////////////////////////////
//                            D E L A Y
//////////////////////////////////////////////////////////////////////////////////////////////////
static void delay(unsigned long del)
{
while(del>0)
	{
		del--;
		DDRB&=0b11111111; // para forçar o compilador a implementar 
						  // o delay com as otimizações ativadas
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                              L C D   G R A V A   I N S T
//////////////////////////////////////////////////////////////////////////////////////////////////
static void LCD_grava_inst(unsigned char cmd)
{
	delay(55);
	LCD_RS_L;
	// bit 4
	if((cmd&0b00010000)!=0) LCD_DATA_PORT4_H;
	else LCD_DATA_PORT4_L;
	// bit 5
	if((cmd&0b00100000)!=0) LCD_DATA_PORT5_H;
	else LCD_DATA_PORT5_L;
	// bit 6
	if((cmd&0b01000000)!=0) LCD_DATA_PORT6_H;
	else LCD_DATA_PORT6_L;
	// bit 7
	if((cmd&0b10000000)!=0) LCD_DATA_PORT7_H;
	else LCD_DATA_PORT7_L;
	LCD_E_H;
	LCD_E_L;
	// bit 0
	if((cmd&0b00000001)!=0) LCD_DATA_PORT4_H;
	else LCD_DATA_PORT4_L;
	// bit 1
	if((cmd&0b00000010)!=0) LCD_DATA_PORT5_H;
	else LCD_DATA_PORT5_L;
	// bit 2
	if((cmd&0b00000100)!=0) LCD_DATA_PORT6_H;
	else LCD_DATA_PORT6_L;
	// bit 3
	if((cmd&0b00001000)!=0) LCD_DATA_PORT7_H;
	else LCD_DATA_PORT7_L;
	LCD_E_H;
	LCD_E_L;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                               L C D   P U T C H A R
//////////////////////////////////////////////////////////////////////////////////////////////////
void LCD_putchar(char c)
{
	delay(55);
	LCD_RS_H;
	// bit 4
	if((c&0b00010000)!=0) LCD_DATA_PORT4_H;
	else LCD_DATA_PORT4_L;
	// bit 5
	if((c&0b00100000)!=0) LCD_DATA_PORT5_H;
	else LCD_DATA_PORT5_L;
	// bit 6
	if((c&0b01000000)!=0) LCD_DATA_PORT6_H;
	else LCD_DATA_PORT6_L;
	// bit 7
	if((c&0b10000000)!=0) LCD_DATA_PORT7_H;
	else LCD_DATA_PORT7_L;
	LCD_E_H;
	LCD_E_L;
	// bit 0
	if((c&0b00000001)!=0) LCD_DATA_PORT4_H;
	else LCD_DATA_PORT4_L;
	// bit 1
	if((c&0b00000010)!=0) LCD_DATA_PORT5_H;
	else LCD_DATA_PORT5_L;
	// bit 2
	if((c&0b00000100)!=0) LCD_DATA_PORT6_H;
	else LCD_DATA_PORT6_L;
	// bit 3
	if((c&0b00001000)!=0) LCD_DATA_PORT7_H;
	else LCD_DATA_PORT7_L;
	LCD_E_H;
	LCD_E_L;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//                                    C L R  L C D
//////////////////////////////////////////////////////////////////////////////////////////////////
void clr_lcd()
{
	LCD_grava_inst(0x01);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                                  G O T O X Y
//////////////////////////////////////////////////////////////////////////////////////////////////
void gotoxy(char x,char y)
{
	if(y==0)
	{
		LCD_grava_inst(0x80+x);
	}
	if(y==1)
	{
		LCD_grava_inst(0xC0+x);
	}
	if(y==2)
	{
		LCD_grava_inst(0x80+20+x);
	}
	if(y==3)
	{
		LCD_grava_inst(0xC0+20+x);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                               E S P E C I A L  C H A R
//////////////////////////////////////////////////////////////////////////////////////////////////
/*static void especial_char(char end, char L1,char L2,char L3,char L4,char L5,char L6,char L7,char L8)
{
	end=end*8+0x40;
	grava_inst(end);
	LCD_putchar(L1);
	LCD_putchar(L2);
	LCD_putchar(L3);
	LCD_putchar(L4);
	LCD_putchar(L5);
	LCD_putchar(L6);
	LCD_putchar(L7);
	LCD_putchar(L8);
}
*/

//////////////////////////////////////////////////////////////////////////////////////////////////
//                               P U T S T R
////////////////////////////////////////////////////////////////////////////////////////
void putstr(char *str)
{
	int cont=0;
	while(str[cont]!=0&&cont<1000)
	{
		LCD_putchar(str[cont]);
		cont++;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                               P U T U I N T
////////////////////////////////////////////////////////////////////////////////////////
void putuint(unsigned int dado)
{
	char str[11];
	int cont=0;

	if(dado>999999999)
	{
		str[cont]=(char)((dado/1000000000)+48);//10
		cont++;
	}
	if(dado>99999999)
	{
		str[cont]=(char)((dado%1000000000/100000000)+48);//9
		cont++;
	}
	if(dado>9999999)
	{
		str[cont]=(char)((dado%100000000/10000000)+48);//8
		cont++;
	}
	if(dado>999999)
	{
		str[cont]=(char)((dado%10000000/1000000)+48);//7
		cont++;
	}
	if(dado>99999)
	{
		str[cont]=(char)((dado%1000000/100000)+48);//6
		cont++;
	}
	if(dado>9999)
	{
		str[cont]=(char)((dado%100000/10000)+48);//5
		cont++;
	}
	if(dado>999)
	{
		str[cont]=(char)(((dado%10000)/1000)+48);//4
		cont++;
	}
	if(dado>99)
	{
		str[cont]=(char)(((dado%1000)/100)+48);//3
		cont++;
	}
	if(dado>9)
	{
		str[cont]=(char)(((dado%100)/10)+48);//2
		cont++;
	}
	str[cont]=(char)((dado%10)+48);//1
	cont++;
	str[cont]=0;
	putstr(str);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                               P U T I N T
////////////////////////////////////////////////////////////////////////////////////////
void putint(int dado)
{
	char str[12];
	int cont=0;
	if(dado<0)
	{
		str[cont]='-';
		cont++;
		dado=dado*(-1); // não funciona para -2147483648
	}
	if(dado>999999999)
	{
		str[cont]=(char)((dado/1000000000)+48);//10
		cont++;
	}
	if(dado>99999999)
	{
		str[cont]=(char)((dado%1000000000/100000000)+48);//9
		cont++;
	}
	if(dado>9999999)
	{
		str[cont]=(char)((dado%100000000/10000000)+48);//8
		cont++;
	}
	if(dado>999999)
	{
		str[cont]=(char)((dado%10000000/1000000)+48);//7
		cont++;
	}
	if(dado>99999)
	{
		str[cont]=(char)((dado%1000000/100000)+48);//6
		cont++;
	}
	if(dado>9999)
	{
		str[cont]=(char)((dado%100000/10000)+48);//5
		cont++;
	}
	if(dado>999)
	{
		str[cont]=(char)(((dado%10000)/1000)+48);//4
		cont++;
	}
	if(dado>99)
	{
		str[cont]=(char)(((dado%1000)/100)+48);//3
		cont++;
	}
	if(dado>9)
	{
		str[cont]=(char)(((dado%100)/10)+48);//2
		cont++;
	}
	str[cont]=(char)((dado%10)+48);//1
	cont++;
	str[cont]=0;
	putstr(str);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                               L C D   i n i t
//////////////////////////////////////////////////////////////////////////////////////////////////
void LCD_init()
{
	unsigned int i;
	char initdata[] = {0x33, 0x32, 0x28, 0x0C, 0x01, 0x06, 0x80};

	delay(200);	

	LCD_DATA_DIRECTION4;
	LCD_DATA_DIRECTION5;
	LCD_DATA_DIRECTION6;
	LCD_DATA_DIRECTION7;
	LCD_E_DIRECTION;
	LCD_RS_DIRECTION;
	LCD_E_H;
	LCD_RS_L;   
	LCD_E_L;
	
	for (i = 0; i < sizeof initdata; ++i)
	{
		LCD_grava_inst(initdata[i]);
		delay(500);
	}
}