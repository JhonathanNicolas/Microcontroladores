// https://circuitdigest.com/microcontroller-projects/sending-email-using-msp430-and-esp8266

#include <msp430g2553.h>
#include <legacymsp430.h>

#define WIFI_NAME    "GVT-60C0"//"Diogo"
#define WIFI_PSWD    "3703000202"//"khqw6023"
#define EMAIL_DST    "srcaetano@gmail.com"
#define EMAIL_SRC    "srcaetano@gmail.com"
#define SMTP2GO_NAME "c3JjYWV0YW5vQGdtYWlsLmNvbQ=="
#define SMTP2GO_PSWD "NGN5ZXlVSElzQVpp"
#define SMTP_SERVER  "mail.smtp2go.com"
#define SMTP_PORT    "2525"

#define OK "OK\r\n"

#define BTN  BIT3
#define RX   BIT1
#define TX   BIT2
#define LED1 BIT0
#define LED2 BIT6
#define LEDS (LED1|LED2)
#define BLINK_T 150

#define BAUD_9600   0
#define BAUD_19200  1
#define BAUD_38400  2
#define BAUD_56000  3
#define BAUD_115200 4
#define BAUD_128000 5
#define BAUD_256000 6
#define NUM_BAUDS   7

void Send_Data(unsigned char c);
void Int_to_String(int n, char *str);
void Send_String(char str[]);
void Init_UART(unsigned int baud_rate_choice);
void Atraso(unsigned int ms);
int cmp_str(char *str1, char *str2);
int AT_command_response(char command[], char response[]);
void Wait_Btn(void);
void Pisca(int num_pisca, char led_bits, unsigned int T_2);
int len_str(char *str);
void cat_str(char *str_dest, char *str_src);
void connect(char *wifi_name, char *wifi_pswd);
void send_email(char *url, char *port, char *req);
void AT_CIPSEND(char *str);

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;
	
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;
	
	P1OUT = BTN;
	P1REN |= BTN;
	P1DIR &= ~BTN;
	P1OUT &= ~LEDS;
	P1DIR |= LEDS;

	Atraso(500);
	Init_UART(BAUD_9600);
	connect(WIFI_NAME, WIFI_PSWD);
	while(1)
	{
		P1OUT |= LED1;
		Wait_Btn();
		P1OUT &= ~LED1;
		send_email(SMTP_SERVER, SMTP_PORT,
			"MSP430+ESP-01 send email.\r\n");
	}
	return 0;
}

void connect(char *wifi_name, char *wifi_pswd)
{
	char req[100];
	req[0] = '\0';
	cat_str(req, "AT+CWJAP_CUR=\"");
	cat_str(req, wifi_name);
	cat_str(req, "\",\"");
	cat_str(req, wifi_pswd);
	cat_str(req, "\"\r\n");
	AT_command_response("AT\r\n", OK);
	AT_command_response("AT+CWMODE=3\r\n", OK);
	AT_command_response("AT+CWQAP\r\n", OK);
	AT_command_response(req, OK);
	AT_command_response("AT+CIPMUX=1\r\n",OK);
}

#define CONN_ID "0"

void send_email(char *url, char *port, char *req)
{
	char cur_req[150];
	cur_req[0] = '\0';
	AT_command_response("AT+CIPSERVER=1,80\r\n",OK);
	cat_str(cur_req, "AT+CIPSTART=" CONN_ID ",\"TCP\",\"");
	cat_str(cur_req, url);
	cat_str(cur_req, "\",");
	cat_str(cur_req, port);
	cat_str(cur_req, "\r\n");
	AT_command_response(cur_req, OK);
	AT_CIPSEND("HELO www.site.com\r\n");
	AT_CIPSEND("AUTH LOGIN\r\n");
	AT_CIPSEND(SMTP2GO_NAME "\r\n");
	AT_CIPSEND(SMTP2GO_PSWD "\r\n");
	AT_CIPSEND("MAIL FROM:<" EMAIL_SRC ">\r\n");
	AT_CIPSEND("RCPT TO:<" EMAIL_DST ">\r\n");
	AT_CIPSEND("DATA\r\n");
	AT_CIPSEND(req);
	AT_CIPSEND(".\r\n");
	AT_CIPSEND("QUIT\r\n");
	AT_command_response("AT+CIPCLOSE=" CONN_ID "\r\n",OK);
	AT_command_response("AT+CIPSERVER=0\r\n",OK);
}

void AT_CIPSEND(char *str)
{
	char req[150];
	req[0] = '\0';
	cat_str(req,"AT+CIPSEND=" CONN_ID ",");
	Int_to_String(len_str(str), req+len_str(req));
	cat_str(req, "\r\n");
	cat_str(req, str);
	AT_command_response(req, OK);
}

int len_str(char *str)
{
	int i = 0;
	while(str[i]!='\0')
		i++;
	return i;
}

void cat_str(char *str_dest, char *str_src)
{
	int i = 0, j = len_str(str_dest);
	while(1)
	{
		if((str_dest[j] = str_src[i])=='\0')
			return;
		i++;
		j++;
	}
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

void Int_to_String(int n, char *str)
{
	int casa, dig, i=0;
	if(n==0)
	{
		str[i++] = '0';
		str[i] = '\0';
		return;
	}
	if(n<0)
	{
		str[i++] = '-';
		n = -n;
	}
	for(casa = 1; casa<=n; casa *= 10);
	casa /= 10;
	while(casa>0)
	{
		dig = (n/casa);
		str[i++] = dig+'0';
		n -= dig*casa;
		casa /= 10;
	}
	str[i] = '\0';
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
			{
				Pisca(1, LEDS, BLINK_T);
				return tries;
			}
			str[i=0] = '\0';
		}
		else
			i++;
	}
	Pisca(1, LEDS, BLINK_T);
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
