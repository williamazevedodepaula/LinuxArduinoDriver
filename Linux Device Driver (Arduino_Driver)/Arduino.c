
/***************************************************************
* PONTIFÍCIA UNIVERSIDADE CATÓLICA DE MINAS GERAIS
*
* LINUX DEVICE DRIVER PARA COMUNICAÇÃO COM ARDUINO, VIA USB,
* SEM NECESSIDADE DE UTILIZAR PORTA SERIAL VIRTUAL NEM
* FTDI RS232 DRIVER.
*
* AUTIORES: Jonathan Ferreira Bispo e William Azevedo de Paula
* PROFESSOR: Paulo César Amaral
*
*****************************************************************/
//***********************INCLUSÃO DE BIBLIOTECAS********************************
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <asm/uaccess.h>
#include <linux/usb.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>                /*copy_*_user */


#define VENDOR_ID     0x0403 //Future Technonoly Devices International
#define PRODUCT_ID    0x6001  //conversor USB-Serial (utilizado no Arduino)

#define USB_ARDUINO_MINOR_BASE 0

//==========================================================================

//Tabela de dispositivos aceitos pelo Driver
static struct usb_device_id Arduino_Id[] = {
    {USB_DEVICE(VENDOR_ID , PRODUCT_ID)},
    { }
};

//Struct que define o dispositivo
struct Usb_Arduino{
   struct usb_device        *udev;
   struct usb_interface     *interface;

   size_t                   bulk_in_size;          //Tamanho do Buffer de entrada
   int                      bulk_in_endpointAddr;  //Endereço do Endpoint Bulk de Entrada
   char                      *bulk_in_buffer;       //Apontador para o buffer de entrada

   int                      bulk_out_endpointAddr; //Endereço do Endpoint Bulk de Saída
   char                      *bulk_out_buffer;     //Apontador para o buffer de saída
   size_t                   bulk_out_size;	   //Tamanho do Buffer de saída

   struct mutex             Dev_Mutex;             //Mutex para prevenir situações de corrida

};

//-------------Protótipos dos métodos utilizados--------------------------------------------
static int Arduino_Probe(struct usb_interface *interface, const struct usb_device_id *id);
static void Arduino_Disconnect(struct usb_interface *interface);
//------------------------------------------------------------------------------------------


//Estrutura que define o driver
static struct usb_driver Arduino_Driver = {
   .name       = "Arduino_Driver",	    //Nome do Driver
   .id_table   = Arduino_Id,		    //Tabela de Dispositivos aceitos
   .probe      = Arduino_Probe,		    //Método chamado ao conectar Arduino ao Barramento USB
   .disconnect = Arduino_Disconnect,    //Método chamado ao desconectar Arduino do Barramento USB
};


//================================================================
//========= OPERAÇÔES SUPORTADAS PELO DISPOSITIVO=================
//================================================================


/*************************************************************
      			Método Open
 *************************************************************
 *   Este método é chamado ao abrir o Arquivo de caractere
 *   associado ao dispositivo no diretório "/dev"
 *
 * FUNÇÂO: Obter dados do dispositivo para permitir as
 *         operações de entrada e saída.
 *************************************************************/
static int Arduino_Open(struct inode *inode, struct file *file){
    int retval = 0;
    struct Usb_Arduino *dev;
    struct usb_interface *interface;
    int subminor;


    subminor = iminor(inode);				      //Obtém minor number do dispositivo
    interface = usb_find_interface(&Arduino_Driver,subminor); //Obtém interface do dispositivo
    if(!interface){
        printk(KERN_INFO "Arduino_Driver: [ERRO] Não foi encontrado o minor number %d",subminor);
        goto exit;
    }
    dev = usb_get_intfdata(interface);			      //Obtém dados do dispositivo a partir da interface
    if (!dev){
        retval = -ENODEV;
        printk(KERN_INFO "Arduino_Driver: [ERRO] Não foi possível obter interface");
        goto exit;
    }
    file->private_data = dev;	//Salva dados do dispositivo em file->private data, para ser acessado depois
    return 0;

exit:
     return retval;
}

/*************************************************************
      			Método Write
 *************************************************************
 *   Este método é chamado ao escrever no Arquivo de
 *   caractere associado ao dispositivo no diretório "/dev"
 *
 * FUNÇÂO: Transmitir os dados escritos para o dispositivo
 *************************************************************/
static ssize_t Arduino_Write(struct file *file,const char *user_buffer,size_t count,loff_t  *ppos){
    int qt_bits=0,a,l,retval = -ENODEV;

    struct Usb_Arduino *dev;
    char buf        = 0;
    char buf2[4]    = "";
    char command[1027]="";
    char out[1024]="";
    char y[2]="";
    int temp = 0,contador =0;

    //Copia o dado escrito pelo usuário no User Space para o Kernel Space
    if(copy_from_user(command,user_buffer,1027)){
       return -EFAULT;
    }


   //Se o comando começar com "byt", envia 1 byte (num de 0 a 255) --> Tres primeiros caracteres depois do "byt"
    if(command[0]== 'b'&&command[1]=='y'&&command[2]=='t'){


       for(l=0;l<3;l++){
          buf2[l]= command[l+3];
       }
       temp= simple_strtoul(buf2,NULL,10);
       buf = (char)temp;
       out[0]=buf;
       contador = 1;


    }else
    //se começar com "str" transfere uma string de ate 1024 bytes (1 byte por vez)
    if(command[0]== 's'&&command[1]=='t'&&command[2]=='r' ){
        contador =  count-3;

        for(l=0;l<count;l++){
           out[l]= command[l+3];
           out[l+1]='\0';
        }


    }

    dev = file->private_data; //Obtém os dados do dispositivos, salvos no método "Open"

    //Envia para o dispositivo todos os caracteres obtidos do User Space
    printk(KERN_INFO "==============Escrita no Arduino%d=========================",dev->interface->minor);
    for(a =0;a < contador;a++){


       y[0] = 126;
       y[1] = out[a];

          printk(KERN_INFO "A: %d      ENVIANDO: %c     %d",a,out[a],(int)out[a]); //Log do dado enciado

          retval = usb_bulk_msg(dev->udev,usb_sndbulkpipe(dev->udev,dev->bulk_out_endpointAddr),y,
                         2,&qt_bits,0); //Envia mensagem do tipo "bulk" (automaticamente aloca e envia URB)

          if(retval < 0){
            retval = -EFAULT;
            printk(KERN_INFO "Arduino_Driver: [ERRO] 0 bytes foram enviados.");
            goto error;
          }


    }
    return count;

error:

    return retval;
}



/*************************************************************
      			Método Read
 *************************************************************
 *   Este método é chamado ao ler o Arquivo de caractere
 *   associado ao dispositivo no diretório "/dev"
 *
 * FUNÇÂO: Receber dados do dispositivo e copiar para o User
 *	   space.
 *************************************************************/
static ssize_t Arduino_Read(struct file *file,char *user_buffer,size_t count,loff_t  *ppos){
    int retval=-EFAULT,a = 0,valor=0;
    int parcial=0,Qtde_Read;
    struct Usb_Arduino *dev;
    char *y;
    int Qt_Write=0;


    dev = file->private_data; //Obtém os dados do dispositivos, salvos no método "Open"


    printk(KERN_INFO "==============Leitura do Arduino%d=========================",dev->interface->minor);

    Qtde_Read = 0;

    y = kmalloc(2,GFP_KERNEL);
    y[0]=126;
    y[1]=0;


    //Cada interação lê um bit. Para ler um byte, realiza 8 interações
    for(a=0;a<8;a++){

        y[0]=126;
        y[1]=a+1;

        //-------------------Envia um byte (a+1) solicitando assim o envio do bit a-----------------
        usb_bulk_msg(dev->udev,usb_sndbulkpipe(dev->udev,dev->bulk_out_endpointAddr),y,
                             2,&Qt_Write,0);

        //---------------------------Lê o Bit-------------------------------------------------------

        usb_bulk_msg(dev->udev,usb_rcvbulkpipe(dev->udev,dev->bulk_in_endpointAddr),y,
                              2,&count,0);//Obtém dois bytes. O primeiro é inútil para o processo

           if((int)y[1]==96){ //Se o segundo byte for '96', o bit a ser lido possui valor "1"
               parcial = 1;
           }
           else{
               if((int)y[1]==112){//Se o segundo byte for '112', o bit a ser lido possui valor "0"
                  parcial = 0;
               }
               else{
                  goto error;
               }

           }
           printk(KERN_INFO "BIT : %d \n\n",parcial);


        valor |= (parcial<<a); //concatena os bits, obtendo assim o byte transferido


    }
    Qtde_Read++;

    printk(KERN_INFO "VALOR: %d \n\n",valor);
    *dev->bulk_in_buffer = valor;//Salva valor lido no buffer de entrada da struct do dispositivo

    if(Qtde_Read>0){
        if(copy_to_user(user_buffer,dev->bulk_in_buffer,1)){
           retval = -EFAULT;
            printk(KERN_INFO "Arduino_Driver: [ERRO] 0 bytes foram recebidos.");
            goto error;
        }
        else{
           retval = count;

        }
    }

error:
    return retval;
}


//================================================================
//========= ESTRUTURAS UTILIZADAS PARA O DISPOSITIVO==============
//================================================================


//Operações suportadas pelo dispositivo
static const struct file_operations Arduino_Fops = {
    .owner     = THIS_MODULE,
    .open      = Arduino_Open,	//Informa o método nome do "Open"
    .read      = Arduino_Read,  //Informa o método nome do "Read"
    .write     = Arduino_Write, //Informa o método nome do "Write"
};

//Classe do dispositivo, para poder registrá-lo no USB-Core
static struct usb_class_driver Arduino_Class = {
    .name       = "Arduino%d",	// Nome a ser utilizado para o arquivo de caractere
				                // O '%d'indica que será concatenado o minor number do dispositivo

    .fops       = &Arduino_Fops,// Estrutura file_operations, com as operaçoes suportadas
    .minor_base = USB_ARDUINO_MINOR_BASE, //Minor Number inicial (para o primeiro dispositivo)
};



//================================================================
//========= MÉTODOS DE INICIALIZAÇÃO DO DISPOSITIVO ==============
//================================================================



/*************************************************************
      			Método PROBE
 *************************************************************
 *   Este método é chamado quando o dispositivo é conectado
 *   ao barramento USB e atribuído a este driver, com base
 *   na id_table.
 *
 * FUNÇÂO: Inicializar o dispositivo e registrá-lo
 *
 *************************************************************/
static int Arduino_Probe(struct usb_interface *interface, const struct usb_device_id *id){
    //Configurar as informações de EndPoints
    //Encontrar EndPoints de Entrada e saída, do tipo bulk.
    int i,   buffer_size,retval = -ENODEV;
   // struct Usb_Arduino *udev = interface_to_usbdev(interface);
    struct Usb_Arduino *dev = NULL;
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;

    dev = kzalloc(sizeof(struct Usb_Arduino), GFP_KERNEL);	//Aloca memória para a estrutura do dispositivo
    if (! dev)
    {
        printk(KERN_ERR "Arduino_Driver: [EERRO] Não foi possível alocar memória para a estrutura Usb_Arduino");
        retval = -ENOMEM;
        goto exit;
    }

    //Salva informações do dispositivo na estrutura do mesmo
    dev->udev = usb_get_dev(interface_to_usbdev(interface));
    dev->interface = interface;

    //Exclusão mútua para evitar corrida entre Probe() e Disconnect()
    mutex_init(&dev->Dev_Mutex);

    //-----------Procura os Endpoints do dispositivo-------------------------------
    iface_desc = interface->cur_altsetting;
    for(i = 0;i < iface_desc->desc.bNumEndpoints;++i){
       endpoint = &iface_desc->endpoint[i].desc;
       if(!dev->bulk_in_endpointAddr &&
          (endpoint->bEndpointAddress & USB_DIR_IN) &&
          ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)==USB_ENDPOINT_XFER_BULK)){
          //Foi encontrado um endpoint de entrada do tipo 'bulk'
          buffer_size = endpoint->wMaxPacketSize; //Busca tamanho do buffer

          //Salvar dados na estrutura do dispositivo
          dev->bulk_in_size = buffer_size;
          dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
          dev->bulk_in_buffer = kmalloc(buffer_size,GFP_KERNEL); //Aloca memória para o buffer
          printk(KERN_INFO "Arduino_Driver: Encontrado Endpoint bulk de entrada");

          if (!dev->bulk_in_buffer){
              printk(KERN_INFO"Arduino_Driver: [EERRO] Não foi possível alocar memória para o buffer");
              goto error;
          } else{
             printk(KERN_INFO "Arduino_Driver: Foram alocados '%d' bytes para o buffer de entrada",buffer_size);
          }


       }

       if(!dev->bulk_out_endpointAddr &&
          !(endpoint->bEndpointAddress & USB_DIR_IN) &&
          ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)==USB_ENDPOINT_XFER_BULK))  {
           //Foi encontrado um endpoint de saída do tipo 'bulk'
           dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
           printk(KERN_INFO "Arduino_Driver: Encontrado Endpoint bulk de saída");
            buffer_size = endpoint->wMaxPacketSize; //Busca tamanho do buffer
            dev->bulk_out_buffer = kmalloc(buffer_size,GFP_KERNEL);
            dev->bulk_out_size = buffer_size;

        }
    }
    //----------------------------------------------------------------------------------------

    if (!(dev->bulk_in_endpointAddr) && (!(dev->bulk_out_endpointAddr ))){
       printk(KERN_INFO "Arduino_Driver: [ERRO] Não foram encontrados endpoints de entrada nem de saída");
       goto error;
    }
    //Salva ponteiro de dados no dispositivo de interface
    usb_set_intfdata(interface, dev);

    //Registra o dispositivo e atribui Minor Number
    retval = usb_register_dev(interface, &Arduino_Class);
    if (retval){//Se houver retorno, é porque ocorreu erro ao registrar o dispositivo
       printk(KERN_INFO "Arduino_Driver: [ERRO] Não foi possível atribuir 'minor number' ao dispositivo");
       usb_set_intfdata(interface, NULL);
       goto error;
    }

    printk(KERN_INFO "Arduino_Driver: dispositivo 'Arduino' conectado via USB. Minor: %d'",interface->minor);
    return 0;


error:
    usb_put_dev(dev->udev);
    kfree(dev->bulk_in_buffer);
    kfree(dev);
    return retval;
exit:
   return retval;
}


/*************************************************************
      			Método DISCONNECT
 *************************************************************
 *   Este método é chamado quando o dispositivo é
 *   desconectado do barramento USB.
 *
 * FUNÇÂO: Desregistrá-lo e liberar o minor number
 *
 *************************************************************/
static void Arduino_Disconnect(struct usb_interface *interface){
    struct Usb_Arduino *dev;
    int minor = interface->minor;

    dev = usb_get_intfdata(interface);
    usb_set_intfdata(interface,NULL);
    //Bloqueia o Kernel para evitar corrida entre Open() e Disconnect()
    mutex_lock(&dev->Dev_Mutex);

    //Devolve o Minor Number
    usb_deregister_dev(interface,&Arduino_Class);

    //Desbloqueia o Kernel
    mutex_unlock(&dev->Dev_Mutex);

    printk(KERN_INFO "Arduino_Driver: Arduino desconectado via USB [Minor %d]",minor);
}



//================================================================
//========= MÉTODOS DE INICIALIZAÇÃO DO DEVICE DRIVER ============
//================================================================


/*************************************************************
      			Método __init
 *************************************************************
 *   Este método é chamado quando o driver é carregado no
 *   kernel.
 *
 * FUNÇÂO: Inicializar e registrar o driver
 *
 *************************************************************/
static int __init Usb_Arduino_init(void){
    int result = 0;

    //Registra este drive com o USB core
    result = usb_register(&Arduino_Driver);
    if(result){
       printk(KERN_INFO "[EERRO] Falha ao registrar o Arduino_Driver. Erro numero %d",result);
    }
    else{
       printk(KERN_INFO "Arduino_Driver registrado com sucesso");
    }
    return result;
}

/*************************************************************
      			Método __exit
 *************************************************************
 *   Este método é chamado quando o driver é removido do
 *   kernel.
 *
 * FUNÇÂO: Desregistrar o driver
 *
 *************************************************************/
static void __exit Usb_Arduino_Exit(void){
    //Desregistra o driver do USB Core
  usb_deregister(&Arduino_Driver );
  printk(KERN_INFO "Arduino_Driver Desregistrado\n");
}


module_init(Usb_Arduino_init);		//Define o Método Arduino_init como sendo o de inicialização
module_exit(Usb_Arduino_Exit);  	//Define o Método Arduino_exit como sendo o de finalização
MODULE_AUTHOR("William & Jonathan");//Autores do Driver
MODULE_DESCRIPTION("Device Driver para Arduino Duemilanove"); //Descrição do Driver
MODULE_VERSION("1.0.0");		   //Versão do Driver
MODULE_LICENSE("GPL");			   //Licensa
