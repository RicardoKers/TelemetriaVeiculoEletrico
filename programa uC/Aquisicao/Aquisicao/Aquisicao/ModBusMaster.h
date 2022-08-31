/*
 * ModBusMaster.h
 *
 *		Vers�o: 03-2022
 *      Author: Kerschbaumer
 *
 *  Fun��es implementadas:
 *
 *  	Read Coil Status (FC=01)
 *  	Read Holding Registers (FC=03)
 *  	Force Single Coil (FC=05)
 *  	Preset Single Register (FC=06)
 *  	Force Multiple Coils (FC=15)
 *  	Preset Multiple Registers (FC=16)
 */

// Configura��o ModBus
#define tam_buff_recep 255
#define tam_buff_trans 255
#define modBusTimeout_ms 1000
// final Configura��o ModBus

struct mb
{
	enum e_status {inativo, transmitindo, aguardandoResposta, respostaRecebida, semResposta} status;
	enum e_erro {semErro, timeout, respostaInvalida} erro;
	uint8_t end_modbus; // armazena o endere�o na modbus
	uint16_t rxsize; // tamanho do pacote na recep��o
	uint16_t txsize; // tamanho do pacote na transmiss�o
	uint8_t rxbuf[tam_buff_recep]; // buffer de recep��o
	uint8_t txbuf[tam_buff_trans]; // buffer de transmiss�o
	uint8_t funcao;
	uint16_t *data_reg_addr; // ponteiro para os dados de word recebidos pela modbus
	uint8_t *data_bit_addr; // ponteiro para os dados de bit recebidos pela modbus
	uint16_t numRegs; // armazena o n�mero de registradores da opera��o
	uint16_t rxpt; // ponteiro para o buffer de recep�ao, necessario em algumas arquiteturas
	uint16_t txpt; // ponteiro para o buffer de transmiss�o, necessario em algumas arquiteturas
} ModBus;

// defini��es
#define modBusTimeoutCounterStart() modBusTimeoutCounter=0
#define modBusTimeoutCounterStop() modBusTimeoutCounter=modBusTimeout_ms+1

// vari�veis globais
unsigned int modBusTimeoutCounter=modBusTimeout_ms+1;

//////////////////////////////////////////////////////////////////////////////////////////////////
//                            P R O T O T I P O S
//////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t CRC16Table256(uint16_t i) // gera tabela de crc
{
	uint16_t crc,c,j;
	crc = 0;
	c   = (uint16_t) i;
	for (j=0; j<8; j++)
	{
		if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ 0xA001;
		else crc =   crc >> 1;
		c = c >> 1;
    }
	return crc;
}


uint16_t update_crc_16(uint16_t crc, uint8_t c) // atualiza o valor do crc
{
	uint16_t tmp, short_c;
	short_c = 0x00ff & (uint16_t) c;
	tmp =  crc       ^ short_c;
	crc = (crc >> 8) ^ CRC16Table256((uint16_t) tmp & 0xff);
	return crc;
}

uint16_t CRC16(uint8_t *ptr,uint16_t npts) // calcula o crc de um vetor
{
	uint16_t crc;
	uint16_t i;
	crc=0xffff;// valor inicial para CRC16modbus
	for(i=0;i<npts;i++)
	{
		crc=update_crc_16(crc,(uint8_t) *(ptr+i));
    }
	return crc;
}

void ModBusReset() // prepara para a pr�xima transmiss�o
{
	ModBus.status=inativo;
	ModBus.erro=semErro;	
}

void modBusReadCoilStatusFC01(uint16_t endInicial, uint16_t numRegistradores, uint8_t *dataAddr)
{
	uint16_t crc;
	if(ModBus.status==inativo) // est� pronto para transmitir
	{
		ModBus.txbuf[0]=ModBus.end_modbus; // inicia o pacote com o endere�o do slave
		ModBus.txbuf[1]=1; // indica a fun��o 1
		ModBus.txbuf[2]=(uint8_t)(endInicial>>8);
		ModBus.txbuf[3]=(uint8_t)(endInicial);
		ModBus.txbuf[4]=(uint8_t)(numRegistradores>>8);
		ModBus.txbuf[5]=(uint8_t)(numRegistradores);
		crc=CRC16(ModBus.txbuf,(uint16_t)6); // calcula o crc
		ModBus.txbuf[6]=(uint8_t)(crc);
		ModBus.txbuf[7]=(uint8_t)(crc>>8);
		ModBus.txsize=8;
		ModBus.status=transmitindo;
		ModBus.funcao=1; // para ajustar a recep��o
		ModBus.data_bit_addr=dataAddr; // armazena o ponteiro para a recep��o
		ModBus.numRegs=numRegistradores; // armazena o n�mero de registradores para a recep��o
		ModBus.rxsize=5+(numRegistradores/8); // determina o n�mero de bytes na resposta para preparar a recep��o
		if((numRegistradores%8)!=0) ModBus.rxsize++; // soma um se n�o completou 8 bits de recep��o
		ModBus.txpt=1; // atualiza o ponteiro de transmiss��o
		iniciaTransmissao(ModBus.txbuf[0]);
	}
}

void modBusReadHoldingRegistersFC03(uint16_t endInicial, uint16_t numRegistradores, uint16_t *dataAddr)
{
	uint16_t crc;
	if(ModBus.status==inativo) // est� pronto para transmitir
	{
		ModBus.txbuf[0]=ModBus.end_modbus; // inicia o pacote com o endere�o do slave
		ModBus.txbuf[1]=3; // indica a fun��o 3
		ModBus.txbuf[2]=(uint8_t)(endInicial>>8);
		ModBus.txbuf[3]=(uint8_t)(endInicial);
		ModBus.txbuf[4]=(uint8_t)(numRegistradores>>8);
		ModBus.txbuf[5]=(uint8_t)(numRegistradores);
		crc=CRC16(ModBus.txbuf,(uint16_t)6); // calcula o crc
		ModBus.txbuf[6]=(uint8_t)(crc);
		ModBus.txbuf[7]=(uint8_t)(crc>>8);
		ModBus.txsize=8;
		ModBus.status=transmitindo;
		ModBus.funcao=3; // para ajustar a recep��o
		ModBus.data_reg_addr=dataAddr; // armazena o ponteiro para a recep��o
		ModBus.numRegs=numRegistradores; // armazena o n�mero de registradores para a recep��o
		ModBus.rxsize=5+(numRegistradores*2); // determina o n�mero de bytes na resposta para preparar a recep��o
		ModBus.txpt=1; // atualiza o ponteiro de transmiss��o
		iniciaTransmissao(ModBus.txbuf[0]);
	}
}

void modBusForceSingleCoilFC05(uint16_t end, uint8_t valor)
{
	uint16_t crc;
	if(ModBus.status==inativo) // est� pronto para transmitir
	{
		ModBus.txbuf[0]=ModBus.end_modbus; // inicia o pacote com o endere�o do slave
		ModBus.txbuf[1]=5; // indica a fun��o 5
		ModBus.txbuf[2]=(uint8_t)(end>>8);
		ModBus.txbuf[3]=(uint8_t)(end);
		if(valor==1)
		{
			ModBus.txbuf[4]=0xFF;
			ModBus.txbuf[5]=0;
		}
		else
		{
			ModBus.txbuf[4]=0;
			ModBus.txbuf[5]=0;
		}
		crc=CRC16(ModBus.txbuf,(uint16_t)6); // calcula o crc
		ModBus.txbuf[6]=(uint8_t)(crc);
		ModBus.txbuf[7]=(uint8_t)(crc>>8);
		ModBus.txsize=8;
		ModBus.status=transmitindo;
		ModBus.funcao=5; // para ajustar a recep��o
		ModBus.rxsize=8; // determina o n�mero de bytes na resposta para preparar a recep��o
		ModBus.txpt=1; // atualiza o ponteiro de transmiss��o
		iniciaTransmissao(ModBus.txbuf[0]);
	}
}

void modBusPresetSingleRegisterFC06(uint16_t end, uint16_t valor)
{
	uint16_t crc;
	if(ModBus.status==inativo) // est� pronto para transmitir
	{
		ModBus.txbuf[0]=ModBus.end_modbus; // inicia o pacote com o endere�o do slave
		ModBus.txbuf[1]=6; // indica a fun��o 6
		ModBus.txbuf[2]=(uint8_t)(end>>8);
		ModBus.txbuf[3]=(uint8_t)(end);
		ModBus.txbuf[4]=(uint8_t)(valor>>8);
		ModBus.txbuf[5]=(uint8_t)(valor);
		crc=CRC16(ModBus.txbuf,(uint16_t)6); // calcula o crc
		ModBus.txbuf[6]=(uint8_t)(crc);
		ModBus.txbuf[7]=(uint8_t)(crc>>8);
		ModBus.txsize=8;
		ModBus.status=transmitindo;
		ModBus.funcao=6; // para ajustar a recep��o
		ModBus.rxsize=8; // determina o n�mero de bytes na resposta para preparar a recep��o
		ModBus.txpt=1; // atualiza o ponteiro de transmiss��o
		iniciaTransmissao(ModBus.txbuf[0]);
	}
}

void modBusForceMultipleCoilsFC15(uint16_t endInicial, uint16_t numRegistradores, uint8_t *dataAddr)
{
	uint16_t temp; // vari�vel para valores tempor�rios
	uint16_t cont_bit; // conta os bits
	uint16_t cont;
	uint16_t crc;
	if(ModBus.status==inativo) // est� pronto para transmitir
	{
		ModBus.txbuf[0]=ModBus.end_modbus; // inicia o pacote com o endere�o do slave
		ModBus.txbuf[1]=15; // indica a fun��o 15
		ModBus.txbuf[2]=(uint8_t)(endInicial>>8);
		ModBus.txbuf[3]=(uint8_t)(endInicial);
		ModBus.txbuf[4]=(uint8_t)(numRegistradores>>8);
		ModBus.txbuf[5]=(uint8_t)(numRegistradores);
		ModBus.txbuf[6]=(numRegistradores/8); // determina o n�mero de bytes na resposta para preparar a recep��o
		if((numRegistradores%8)!=0) ModBus.txbuf[6]++; // soma um se n�o completou 8 bits de recep��o
		temp=7;// primeiro byte de dados do pacote
		cont_bit=1; //determina a posi��o do bit no byte
		ModBus.txbuf[7]=0; //zera o byte para depois inserir os uns
		for(cont=0; cont<numRegistradores; cont++) // conta os bits recebidos
		{
			if(dataAddr[cont]==1) // bit a ser enviado � 1
			{
				ModBus.txbuf[temp]=ModBus.txbuf[temp]|cont_bit; // liga o bit
			}
			cont_bit=cont_bit<<1;
			if(cont_bit==256) //fim do byte
			{
				temp++; // aponta o proximo byte de dados do pacote
				cont_bit=1; // recome�a no primeiro bit
				ModBus.txbuf[temp]=0;
			}
		}
		crc=CRC16(ModBus.txbuf,(uint16_t)temp+1); // calcula o crc
		temp++;
		ModBus.txbuf[temp]=(uint8_t)(crc);
		temp++;
		ModBus.txbuf[temp]=(uint8_t)(crc>>8);
		ModBus.txsize=temp+1;
		ModBus.status=transmitindo;
		ModBus.funcao=15; // para ajustar a recep��o
		ModBus.rxsize=8; // determina o n�mero de bytes na resposta para preparar a recep��o
		ModBus.txpt=1; // atualiza o ponteiro de transmiss��o
		iniciaTransmissao(ModBus.txbuf[0]);
	}
}

void modBusPresetMultipleRegistersFC16(uint16_t endInicial, uint16_t numRegistradores, uint16_t *dataAddr)
{
	uint16_t crc;
	uint16_t temp; // vari�vel para valores tempor�rios
	uint16_t cont;
	if(ModBus.status==inativo) // est� pronto para transmitir
	{
		ModBus.txbuf[0]=ModBus.end_modbus; // inicia o pacote com o endere�o do slave
		ModBus.txbuf[1]=16; // indica a fun��o 16
		ModBus.txbuf[2]=(uint8_t)(endInicial>>8);
		ModBus.txbuf[3]=(uint8_t)(endInicial);
		ModBus.txbuf[4]=(uint8_t)(numRegistradores>>8);
		ModBus.txbuf[5]=(uint8_t)(numRegistradores);
		ModBus.txbuf[6]=(uint8_t)(numRegistradores*2);
		temp=7;
		for(cont=0; cont<numRegistradores; cont++)
		{
			ModBus.txbuf[temp]=(uint8_t)(dataAddr[cont]>>8);
			temp++;
			ModBus.txbuf[temp]=(uint8_t)(dataAddr[cont]);
			temp++;
		}
		crc=CRC16(ModBus.txbuf,(uint16_t)temp); // calcula o crc
		ModBus.txbuf[temp]=(uint8_t)(crc);
		temp++;
		ModBus.txbuf[temp]=(uint8_t)(crc>>8);
		ModBus.txsize=temp+1;
		ModBus.status=transmitindo;
		ModBus.funcao=16; // para ajustar a recep��o
		ModBus.rxsize=8; // determina o n�mero de bytes na resposta para preparar a recep��o
		ModBus.txpt=1; // atualiza o ponteiro de transmiss��o
		iniciaTransmissao(ModBus.txbuf[0]);
	}
}

void ModBusProcess()
{
	uint16_t crc; // armazena o valor do crc do pacote
	uint16_t temp; // vari�vel para valores tempor�rios
	uint16_t cont_bit; // conta o bit nas recep��es de bits
	uint16_t cont; // vari�vel para contar os registradores recebidos

	ModBus.erro=respostaInvalida;
	crc=CRC16(ModBus.rxbuf,ModBus.rxsize-2); // calcula o crc do pacote
	if((uint8_t)(crc&0x00ff)==ModBus.rxbuf[ModBus.rxsize-2]&&(uint8_t)(crc>>8)==ModBus.rxbuf[ModBus.rxsize-1]) // testa se o crc � v�lido
	{
		if(ModBus.funcao==1) // se for a fun��o 1
		{
			if(ModBus.rxbuf[0]!=ModBus.end_modbus)// erro: endere�o inv�lido
			{
				return;
			}
			if(ModBus.rxbuf[1]!=1)// erro: fun��o inv�lida
			{
				return;
			}
			if(ModBus.rxbuf[2]!=(ModBus.rxsize-5))// erro: contador de bytes incorreto
			{
				return;
			}
			// tudo certo, armazenando dados lidos
			temp=3;// primeiro byte de dados do pacote
			cont_bit=1; //determina a posi��o do bit no byte
			for(cont=0; cont<ModBus.numRegs; cont++) // conta os bits recebidos
			{
				if((ModBus.rxbuf[temp]&cont_bit)!=0) // bit recebido � 1
				{
					ModBus.data_bit_addr[cont]=1;
				}
				else // bit recebido � 1
				{
					ModBus.data_bit_addr[cont]=0;
				}
				cont_bit=cont_bit<<1;
				if(cont_bit==256) //fim do byte
				{
					temp++; // aponta o proximo byte de dados do pacote
					cont_bit=1; // recome�a no primeiro bit
				}
			}
			ModBus.erro=semErro;
			return;
		}
		if(ModBus.funcao==3) // se for a fun��o 3
		{
			if(ModBus.rxbuf[0]!=ModBus.end_modbus)// erro: endere�o inv�lido
			{
				return;
			}
			if(ModBus.rxbuf[1]!=3)// erro: fun��o inv�lida
			{
				return;
			}
			if(ModBus.rxbuf[2]!=(ModBus.numRegs*2))// erro: contador de bytes incorreto
			{
				return;
			}
			// tudo certo, armazenando dados lidos
			temp=3;// primeiro byte de dados do pacote
			for(cont=0; cont<ModBus.numRegs; cont++) // conta os bits recebidos
			{
				ModBus.data_reg_addr[cont]=(ModBus.rxbuf[temp+1]|(ModBus.rxbuf[temp]<<8));
				temp=temp+2; // aponta o proximo dado no pacote
			}
			ModBus.erro=semErro;
			return;
		}
		if(ModBus.funcao==5) // se for a fun��o 5
		{
			if(ModBus.rxbuf[0]!=ModBus.end_modbus)// erro: endere�o inv�lido
			{
				return;
			}
			if(ModBus.rxbuf[1]!=5)// erro: fun��o inv�lida
			{
				return;
			}
			if(ModBus.rxbuf[2]!=ModBus.txbuf[2]||ModBus.rxbuf[3]!=ModBus.txbuf[3])// erro: endere�o incorreto
			{
				return;
			}
			if(ModBus.rxbuf[4]!=ModBus.txbuf[4]||ModBus.rxbuf[5]!=ModBus.txbuf[5])// erro: dado incorreto
			{
				return;
			}
			// tudo certo
			ModBus.erro=semErro;
			return;
		}
		if(ModBus.funcao==6) // se for a fun��o 6
		{
			if(ModBus.rxbuf[0]!=ModBus.end_modbus)// erro: endere�o inv�lido
			{
				return;
			}
			if(ModBus.rxbuf[1]!=6)// erro: fun��o inv�lida
			{
				return;
			}
			if(ModBus.rxbuf[2]!=ModBus.txbuf[2]||ModBus.rxbuf[3]!=ModBus.txbuf[3])// erro: endere�o incorreto
			{
				return;
			}
			if(ModBus.rxbuf[4]!=ModBus.txbuf[4]||ModBus.rxbuf[5]!=ModBus.txbuf[5])// erro: dado incorreto
			{
				return;
			}
			// tudo certo
			ModBus.erro=semErro;
			return;
		}
		if(ModBus.funcao==15) // se for a fun��o 15
		{
			if(ModBus.rxbuf[0]!=ModBus.end_modbus)// erro: endere�o inv�lido
			{
				return;
			}
			if(ModBus.rxbuf[1]!=15)// erro: fun��o inv�lida
			{
				return;
			}
			if(ModBus.rxbuf[2]!=ModBus.txbuf[2]||ModBus.rxbuf[3]!=ModBus.txbuf[3])// erro: endere�o incorreto
			{
				return;
			}
			if(ModBus.rxbuf[4]!=ModBus.txbuf[4]||ModBus.rxbuf[5]!=ModBus.txbuf[5])// erro: dado incorreto
			{
				return;
			}
			// tudo certo
			ModBus.erro=semErro;
			return;
		}
		if(ModBus.funcao==16) // se for a fun��o 16
		{
			if(ModBus.rxbuf[0]!=ModBus.end_modbus)// erro: endere�o inv�lido
			{
				return;
			}
			if(ModBus.rxbuf[1]!=16)// erro: fun��o inv�lida
			{
				return;
			}
			if(ModBus.rxbuf[2]!=ModBus.txbuf[2]||ModBus.rxbuf[3]!=ModBus.txbuf[3])// erro: endere�o incorreto
			{
				return;
			}
			if(ModBus.rxbuf[4]!=ModBus.txbuf[4]||ModBus.rxbuf[5]!=ModBus.txbuf[5])// erro: dado incorreto
			{
				return;
			}
			// tudo certo
			ModBus.erro=semErro;
			return;
		}
	}
}

void ModBusTimeout() // atingiu o tempo m�ximo para responder
{
	ModBus.status=semResposta;
	ModBus.erro=timeout;
}