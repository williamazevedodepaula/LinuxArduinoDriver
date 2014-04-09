//#define ACTIVE_DEBUG        //Descomentar esta linha para habilitar
                              //Debug no USB
#include <Arduino_USB.h>
#include <LiquidCrystal.h>
char *texto;
int  saida;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

/*********************************************
 *  Método para inicialização do   firmware  *
 *********************************************/
void setup(){
  Usb_Setup();  //Inicializa a biblioteca Arduino_USB
}
/********************************************
 *  Loop infinto (corpo do Programa)        *
 ********************************************/
void loop(){ 
  texto = Read_String_From_USB();  //Aguarda o envio de uma String via USB
                                   //e a armazena na variável "texto"                                  
                                   
  //-----Percorre todos os caracteres da String lida------                                   
  for(int z=0;(z<1024)&&(texto[z]!=10);z++){   
       lcd.write(texto[z]);        //Escreve a String no display de LCD
  }
  //------------------------------------------------------
  digitalWrite(13,HIGH);           //Acende LED para indicar que está em 
                                   //"modo de escrita" 

  saida = 65;                      //Inicia a escrita com o caractere 'A'
  while(1){
      Write_To_USB(saida++);        //Envia para o USB. Cada vez que o
                                    //caractere for lido, ele é incrementado. 
  }
}
