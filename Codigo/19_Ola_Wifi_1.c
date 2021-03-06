// Hello World com o modulo ESP-01.
// Envia o comando "AT\r\n", e espera
// a resposta "OK\r\n".
// Sempre que recebe um '\n', confere
// se a string recebida é a resposta
// esperada; se não for, reinicia a string.
// Pisca os dois LEDs indicando quantas
// tentativas foram necessarias para
// receber a resposta esperada.

#include <msp430g2553.h>
#include <legacymsp430.h>

#define BTN  BIT3
#define RX   BIT1
#define TX   BIT2
#define LED1 BIT0
#define LED2 BIT6
#define LEDS (LED1|LED2)

#define BAUD_9600   0
#define BAUD_19200  1
#define BAUD_38400  2
#define BAUD_56000  3
#define BAUD_115200 4
#define BAUD_128000 5
#define BAUD_256000 6
#define NUM_BAUDS   7

void Send_Data(unsigned char c);
void Send_String(char str[]);
void Init_UART(unsigned int baud_rate_choice);
void Atraso(unsigned int ms);
int cmp_str(char *str1, char *str2);
int AT_command_response(char command[], char response[]);
void Wait_Btn(void);
void Pisca(int num_pisca, char led_bits, unsigned int T_2);

int main(void)
{
	int tries;
	WDTCTL = WDTPW + WDTHOLD;
	
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;
	
	P1OUT = BTN;
	P1REN |= BTN;
	P1DIR &= ~BTN;
	P1OUT &= ~LEDS;
	P1DIR |= LEDS;

	Init_UART(BAUD_9600);
	while(1)
	{
		Wait_Btn();
		tries = AT_command_response("AT\r\n","OK\r\n");
		Pisca(tries, LEDS, 250);
	}
	return 0;
}

void Wait_Btn(void)
{
	volatile unsigned int i = 300;
	// Espera o botao ser pressionado
	while(P1IN & BTN);
	// Espera o botao ser solto
	while((P1IN & BTN)==0);
	// Debounce
	while(i)
	{
		i--;
		if((P1IN & BTN)==0)
			i = 300;
	}
}

void Pisca(int num_pisca, char led_bits, unsigned int T_2)
{
	while(num_pisca--)
	{
		P1OUT |= led_bits;
		Atraso(T_2);
		P1OUT &= ~led_bits;
		Atraso(T_2);
	}
}

void Send_Data(unsigned char c)
{
	while((IFG2&UCA0TXIFG)==0);
	UCA0TXBUF = c;
}

void Send_String(char str[])
{
	int i;
	for(i=0; str[i]!= '\0'; i++)
		Send_Data(str[i]);
}

void Init_UART(unsigned int baud_rate_choice)
{
	unsigned char BRs[NUM_BAUDS] = {104, 52, 26, 17, 8, 7, 3};
	unsigned char MCTLs[NUM_BAUDS] = {UCBRF_0+UCBRS_1,
										UCBRF_0+UCBRS_0,
										UCBRF_0+UCBRS_0,
										UCBRF_0+UCBRS_7,
										UCBRF_0+UCBRS_6,
										UCBRF_0+UCBRS_7,
										UCBRF_0+UCBRS_7};
	if(baud_rate_choice<NUM_BAUDS)
	{
		UCA0CTL1 |= UCSWRST;
		// Configura a transmissao serial UART com 8 bits de dados,
		// sem paridade, comecando pelo bit menos significativo,
		// e com um bit de STOP
		UCA0CTL0 = 0;
		// Escolhe o SMCLK como clock para a UART
		UCA0CTL1 |= UCSSEL_2;
		// Define a baud rate
		UCA0BR0 = BRs[baud_rate_choice];
		UCA0BR1 = 0;
		UCA0MCTL = MCTLs[baud_rate_choice];
		// Habilita os pinos para transmissao serial UART
		P1SEL2 = P1SEL = RX+TX;
		UCA0CTL1 &= ~UCSWRST;
	}
}

int AT_command_response(char command[], char response[])
{
	char str[150];
	int i=0, tries=0;
	Send_String(command);
	while(1)
	{
		while((IFG2&UCA0RXIFG)==0);
		str[i] = UCA0RXBUF;
		if(str[i]=='\n')
		{
			tries++;
			str[i+1] = '\0';
			if(cmp_str(str, response))
				return tries;
			str[i=0] = '\0';
		}
		else
			i++;
	}
	return tries;
}

// Atraso de ms
void Atraso(unsigned int ms)
{

	TACCR0 = 1000-1;
	TACTL |= TACLR;
	TACTL = TASSEL_2 + ID_0 + MC_1;
	while(ms--)
	{
		while((TACTL&TAIFG)==0);
		TACTL &= ~TAIFG;
	}
	TACTL = MC_0;
}

int cmp_str(char *str1, char *str2)
{
	for(; ((*str1)!='\0')&&((*str2)!='\0'); str1++, str2++)
	{
		if((*str1)!=(*str2))
			return 0;
	}
	return ((*str1)==(*str2));
}
