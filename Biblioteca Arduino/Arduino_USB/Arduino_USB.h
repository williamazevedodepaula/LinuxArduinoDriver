/*******************************************************************************
*                              Arduino_USB                                                                             
*
* Nomes: Jonathan F. Bispo e William Azevedo de Paula
* 
*------------------------------------------------------------------------------
* 
* Biblioteca para transfer�ncia de dados entre a placa "Arduino" e o Computador
* Pessoal (PC), atrav�s do USB, sem utilizar o driver do conversor Serial-USB
* do arduino (o microcontrolador Atmega328, utilizado no arduino, n�o possui 
* controlador USB, sendo a comunica��o realizada atrav�s de protocolo serial
* entre o mesmo e o conversor - FTDI FT232R -, sendo a comunica��o USB apenas 
* entre o FTDI e o computador). Ao utilizar o driver do FTDI FT232R, 
* � gerada uma porta serial virtual no computador (exemplo: COM1), com a qual
* os processos se comunicam tamb�m e forma serial. O driver � quem converte os
* dados para envio via USB, para depois o FTDI converter para serial novamente.
* Esta biblioteca permite a comunica��o direta entre a aplica��o e o FTDI, sem
* a necessidade de utilizar porta serial virtual e nem mesmo os drivers oficiais
* do FTDI (Apenas o driver "Arduino_Driver", desenvolvido juntamente com este 
* trabalho), diminuindo o overhead, uma vez que a comunica��o utilizando porta
* serial virtual mant�m o "baud rate" de uma porta f�sica, o qul tora a 
* transfer�ncia mais lenta do que utilizando neste trabalho.  
*
********************************************************************************/


//---------Inclus�es-------------

#ifndef _AVR_IO_H_
	#include	<avr/io.h>
#endif

#ifndef WProgram_h
	#include	"WProgram.h"
#endif
#ifndef HardwareSerial_h
	#include	"HardwareSerial.h"
#endif


#ifdef ACTIVE_DEBUG
  #include <LiquidCrystal.h>
  LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#endif

//---------------------------------




//----------------------------Atrasos gerados com instrucoes NOP, em Assembly------------------------------------------------------
#define  asm_delay     __asm__("nop\n""nop\n"); 
#define  asm_delay_s   __asm__("nop\n""nop\n""nop\n""nop\n""nop\n""nop\n""nop\n""nop\n""nop\n"); 
#define  asm_delay_a   __asm__("nop\n""nop\n"); 
#define  asm_delay_b   __asm__("nop\n""nop\n""nop\n""nop\n"); //<---Acrescentei 1 nop a mais aki....verificar se nao da erro
#define  asm_delay_c   __asm__("nop\n""nop\n""nop\n""nop\n");
#define  asm_delay_d   __asm__("nop\n""nop\n""nop\n""nop\n""nop\n"); 
#define  asm_delay_e   __asm__("nop\n""nop\n""nop\n""nop\n"); 
#define  asm_delay_f   __asm__("nop\n""nop\n""nop\n""nop\n"); 
#define  asm_delay_g   __asm__("nop\n""nop\n""nop\n""nop\n""nop\n"); 
#define  asm_delay_m   __asm__("nop\n""nop\n""nop\n""nop\n"); 
//----------------------------------------------------------------------------------------------------------------------------------


int w = -1;
int pin_receptor,pin_transmissor;

#define BUFFER_SIZE 64



/*--------Inicializa os dados necessarios para o funcionamento da biblioteca-----
 
  Este m�todo deve ser chamado antes de realizar a primeira transfer�ncia
  de dados, para garantir que todos os dados neces�rios estejam corretamente 
  inicializados
  
--------------------------------------------------------------------------------*/

void Usb_Setup(){
   
  #ifdef ACTIVE_DEBUG 
     lcd.begin(16, 2);
  #endif
  
  
   pin_receptor  = 0;                         //Pino de recep��o serial   (rx)                                                                                                                                                     
   pin_transmissor = 1;                       //Pino de transmiss�o serial(tx)
   
   pinMode(pin_transmissor,OUTPUT);   
   digitalWrite(pin_transmissor, LOW);        //Pino de transmiss�o (tx) deve ficar sempre em zero 
  

}


/*-------------------------LER BYTE DA PORTA USB---------------------------------
 
  Este m�todo fica em espera, at� que um byte chegue da porta USB (do FTDI).
  Ap�s a execu��o, retorna o byte lido.
  
  A leitura � feita atrav�s de uma modifica��o no padr�o serial:
            
  a) O m�todo aguarda at� que apare�a um zero no pino de recep���o (rx).
  b) O m�todo aguarda at� que apare�a um segundo zero no pino de recep���o,
     para evitar que a leitura seja disparada por causa de ru�dos, por exemplo.
  c) Ap�s a leitura do segundo zero, inicia-se o processo de leitura dos 8 bits
     (0-7), come�ando do LSB. Uma leitura � feita com um per�odo de em m�dia
     4 a 5 ciclos de clock do Atmeg328. Esse tempo de espera � gerado
     utilizando instru��es nop, pois os m�todos de gera��o de atraso dispon�veis
     nas bibliotecas do arduino n�o s�o t�o recisos.
     
  Ordem dos bits:
        
  
  VALOR     0  1  1  1  1  1  1  0     b0   b1  b2    b3  b4    b5   b6  b7
            | ------------------ |     |----|----|----|----|----|----|----|
          Prepara    Espera    Inicia  ----------8 bits a serem lidos------
          
          
          
      Uma poss�vel melhoria futura � aproveitar os bits desperdi�ados durante a 
      espera para cria��o de um c�digo de verifica��o de erros, ou acrescentar
      um bit de Paridade.
      
      OBS: A obten��o dos valores dos pinos � obtida utilizando o valor do 
           registrador PIND, onde a posi��o 0 corresponde ao pino Rx e aposi��o 
           1 ao pino Tx. Esta estrat�gia deve ser utilizada no lugar do m�todo 
           "digitalRead()", pois este apresenta overhead muito grande, n�o sendo
           poss�vel verificar todos os bits em tempo.
  
--------------------------------------------------------------------------------*/
int Read_Byte_from_USB(){
  int val = 0;

  
  // Um byte enviado serialmente pelo FTDI
  
  while (val = (PIND & B00000001)<<0);    //Preara 
  asm_delay_s                             //Delay em assembly, para manter precis�o
  while (val = (PIND & B00000001)<<0);    //Espera at� que val=0, marcando o in�cio
  asm_delay_s                                      
  asm_delay_g

  val = val | (PIND & B00000001)<<0;      //Leitura do valor do bino Rx (LSB de PIND)
  asm_delay_a
  
  val = val | (PIND & B00000001)<<1;
  asm_delay_b
  
  val = val | (PIND & B00000001)<<2;
  asm_delay_c

  val = val | (PIND & B00000001)<<3;
  asm_delay_d

  val = val | (PIND & B00000001)<<4;
  asm_delay_e

  val = val | (PIND & B00000001)<<5;
  asm_delay_f

  val = val | (PIND & B00000001)<<6;
  asm_delay_f
  val = val | (PIND & B00000001)<<7;



#ifdef ACTIVE_DEBUG
    lcd.print(val);
    lcd.setCursor(0, 1); 
    lcd.print((val & B10000000) >> 7);
    lcd.print((val & B01000000) >> 6);
    lcd.print((val & B00100000) >> 5);
    lcd.print((val & B00010000) >> 4);
    lcd.print((val & B00001000) >> 3);
    lcd.print((val & B00000100) >> 2);
    lcd.print((val & B00000010) >> 1);
    lcd.print((val & B00000001) >> 0);
#endif 
 
    
       
return val;
     
  
}

/*-------------------------LER STRING DA PORTA USB---------------------------------
 
  Este m�todo apenas chama o m�todo "Read_Byte_from_USB" at� ler todos os
  caracteres de uma string. A leitura termina quando BUFFER_SIZE caracteres s�o 
  lidos ou at� que se encontre o final da string. 
--------------------------------------------------------------------------------*/
char* Read_String_From_USB(){
  char string[BUFFER_SIZE];
  
  string[0]=Read_Byte_from_USB();
  
  #ifdef ACTIVE_DEBUG
     lcd.write(string[0]);
  #endif
  
  for(int i=1;(i<BUFFER_SIZE)&&(string[i-1]!=10);i++){
     string[i]=Read_Byte_from_USB();
     
     #ifdef ACTIVE_DEBUG
        lcd.print(string[i]);     
     #endif
  }
  return string;

}

/*-------------------------ESCREVER NA PORTA USB---------------------------------
 
  Este m�todo permite o envio de bytes do Arduino para o computador.
  Uma diferen�a observada � que, na escrita do FTDI para o PC, o FTDI 
  n�o espera, por exemplo, que um buffer encha para transferi-lo. Em vez disso, 
  Ele est� constantemente enviando dados via USB.
  Devido a esse motivo, at� ent�o, o PC deve fazer um acesso ao arduino para 
  ler cada bit dp byte que deseja oter.
  Devido a limita��es de tempo, limita��es do kernel do linux (SO para o qual 
  foi desenvolvido o driver que comunica com esta biblioteca) e alta complexidade,
  este m�todo foi feito de maneira ineficiente: Para cada bit desejado, um 
  n�mero � enviado ao Arduino solicitando o mesmo, onde o conte�do do byte 
  enviado � o �ndice do bit desejado. Logo, 8 bytes s�o transferidos do PC para
  o Arduino para cada 8 bytes que deseja-se enviar do Arduino para o PC.
  
  Essa esp�cie de "HandShaking" reduz o n�mero de erros de transmiss�o, uma vez 
  que cada bit � enviado apenas se for confirmado que ele � realmente o bit desejado.

  
  Uma poss�vel melhora futura � a utiliza��o de um m�todo de sincronizar o envio 
  de forma mais eficiente, usando uma abordagem mais semelhante � utilizada em
  Read_Byte_from_USB()
  
--------------------------------------------------------------------------------*/
 void Write_To_USB(int valor){
 
 
  while(Read_Byte_from_USB()!=1); 
  PORTD &= B11111101;
  PORTD |= ((valor&B00000001)<<1);
  while(Read_Byte_from_USB()!=2); 
  PORTD &= B11111101;
  PORTD |= ((valor&B00000010)>>0);
  while(Read_Byte_from_USB()!=3); 
  PORTD &= B11111101;
  PORTD |= ((valor&B00000100)>>1);
  while(Read_Byte_from_USB()!=4); 
  PORTD &= B11111101;
  PORTD |= ((valor&B00001000)>>2);
  while(Read_Byte_from_USB()!=5); 
  PORTD &= B11111101;
  PORTD |= ((valor&B00010000)>>3);
  while(Read_Byte_from_USB()!=6); 
  PORTD &= B11111101;
  PORTD |= ((valor&B00100000)>>4);
  while(Read_Byte_from_USB()!=7); 
  PORTD &= B11111101;
  PORTD |= ((valor&B01000000)>>5);
  while(Read_Byte_from_USB()!=8); 
  PORTD &= B11111101;
  PORTD |= ((valor&B10000000)>>6);   


 }

