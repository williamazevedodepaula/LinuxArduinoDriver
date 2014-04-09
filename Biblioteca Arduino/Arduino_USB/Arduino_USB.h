/*******************************************************************************
*                              Arduino_USB                                                                             
*
* Nomes: Jonathan F. Bispo e William Azevedo de Paula
* 
*------------------------------------------------------------------------------
* 
* Biblioteca para transferência de dados entre a placa "Arduino" e o Computador
* Pessoal (PC), através do USB, sem utilizar o driver do conversor Serial-USB
* do arduino (o microcontrolador Atmega328, utilizado no arduino, não possui 
* controlador USB, sendo a comunicação realizada através de protocolo serial
* entre o mesmo e o conversor - FTDI FT232R -, sendo a comunicação USB apenas 
* entre o FTDI e o computador). Ao utilizar o driver do FTDI FT232R, 
* é gerada uma porta serial virtual no computador (exemplo: COM1), com a qual
* os processos se comunicam também e forma serial. O driver é quem converte os
* dados para envio via USB, para depois o FTDI converter para serial novamente.
* Esta biblioteca permite a comunicação direta entre a aplicação e o FTDI, sem
* a necessidade de utilizar porta serial virtual e nem mesmo os drivers oficiais
* do FTDI (Apenas o driver "Arduino_Driver", desenvolvido juntamente com este 
* trabalho), diminuindo o overhead, uma vez que a comunicação utilizando porta
* serial virtual mantém o "baud rate" de uma porta física, o qul tora a 
* transferência mais lenta do que utilizando neste trabalho.  
*
********************************************************************************/


//---------Inclusões-------------

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
 
  Este método deve ser chamado antes de realizar a primeira transferência
  de dados, para garantir que todos os dados necesários estejam corretamente 
  inicializados
  
--------------------------------------------------------------------------------*/

void Usb_Setup(){
   
  #ifdef ACTIVE_DEBUG 
     lcd.begin(16, 2);
  #endif
  
  
   pin_receptor  = 0;                         //Pino de recepção serial   (rx)                                                                                                                                                     
   pin_transmissor = 1;                       //Pino de transmissão serial(tx)
   
   pinMode(pin_transmissor,OUTPUT);   
   digitalWrite(pin_transmissor, LOW);        //Pino de transmissão (tx) deve ficar sempre em zero 
  

}


/*-------------------------LER BYTE DA PORTA USB---------------------------------
 
  Este método fica em espera, até que um byte chegue da porta USB (do FTDI).
  Após a execução, retorna o byte lido.
  
  A leitura é feita através de uma modificação no padrão serial:
            
  a) O método aguarda até que apareça um zero no pino de recepçção (rx).
  b) O método aguarda até que apareça um segundo zero no pino de recepçção,
     para evitar que a leitura seja disparada por causa de ruídos, por exemplo.
  c) Após a leitura do segundo zero, inicia-se o processo de leitura dos 8 bits
     (0-7), começando do LSB. Uma leitura é feita com um período de em média
     4 a 5 ciclos de clock do Atmeg328. Esse tempo de espera é gerado
     utilizando instruções nop, pois os métodos de geração de atraso disponíveis
     nas bibliotecas do arduino não são tão recisos.
     
  Ordem dos bits:
        
  
  VALOR     0  1  1  1  1  1  1  0     b0   b1  b2    b3  b4    b5   b6  b7
            | ------------------ |     |----|----|----|----|----|----|----|
          Prepara    Espera    Inicia  ----------8 bits a serem lidos------
          
          
          
      Uma possível melhoria futura é aproveitar os bits desperdiçados durante a 
      espera para criação de um código de verificação de erros, ou acrescentar
      um bit de Paridade.
      
      OBS: A obtenção dos valores dos pinos é obtida utilizando o valor do 
           registrador PIND, onde a posição 0 corresponde ao pino Rx e aposição 
           1 ao pino Tx. Esta estratégia deve ser utilizada no lugar do método 
           "digitalRead()", pois este apresenta overhead muito grande, não sendo
           possível verificar todos os bits em tempo.
  
--------------------------------------------------------------------------------*/
int Read_Byte_from_USB(){
  int val = 0;

  
  // Um byte enviado serialmente pelo FTDI
  
  while (val = (PIND & B00000001)<<0);    //Preara 
  asm_delay_s                             //Delay em assembly, para manter precisão
  while (val = (PIND & B00000001)<<0);    //Espera até que val=0, marcando o início
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
 
  Este método apenas chama o método "Read_Byte_from_USB" até ler todos os
  caracteres de uma string. A leitura termina quando BUFFER_SIZE caracteres são 
  lidos ou até que se encontre o final da string. 
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
 
  Este método permite o envio de bytes do Arduino para o computador.
  Uma diferença observada é que, na escrita do FTDI para o PC, o FTDI 
  não espera, por exemplo, que um buffer encha para transferi-lo. Em vez disso, 
  Ele está constantemente enviando dados via USB.
  Devido a esse motivo, até então, o PC deve fazer um acesso ao arduino para 
  ler cada bit dp byte que deseja oter.
  Devido a limitações de tempo, limitações do kernel do linux (SO para o qual 
  foi desenvolvido o driver que comunica com esta biblioteca) e alta complexidade,
  este método foi feito de maneira ineficiente: Para cada bit desejado, um 
  número é enviado ao Arduino solicitando o mesmo, onde o conteúdo do byte 
  enviado é o índice do bit desejado. Logo, 8 bytes são transferidos do PC para
  o Arduino para cada 8 bytes que deseja-se enviar do Arduino para o PC.
  
  Essa espécie de "HandShaking" reduz o número de erros de transmissão, uma vez 
  que cada bit é enviado apenas se for confirmado que ele é realmente o bit desejado.

  
  Uma possível melhora futura é a utilização de um método de sincronizar o envio 
  de forma mais eficiente, usando uma abordagem mais semelhante à utilizada em
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

