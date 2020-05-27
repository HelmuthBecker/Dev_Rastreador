/*

                          **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                              ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                          
                   SoftSerial (Saídas Digitais)      Objeto EBYTE (Saídas Digitais)    
                   ** 3,2 -  RX,TX  (GPS)           ** 7, 8, 10 - M0, M1, AUX(Definido mas não conectado)          
                   ** 9,6 -  RX,TX (LORA)                                                
  
 */

#include <EBYTE.h>
#include <SoftwareSerial.h>

#define DEBUG true

SoftwareSerial mySerial(9,6); //Rx - Tx (LORA)

EBYTE emissor(&mySerial, 7, 8, 10); //Parâmetros do módulo LORA (RX,TX,M0,M1,AUX)

void setup() {
  
Serial.begin(9600); //Inicia a serial física
mySerial.begin(9600); //Inicia a serial lógica

iniciarLORA(); //Função que passa os parâmetros de funcionamento do módulo LORA
}

void loop() 
 {  
             percursoFake(); //Função que gera um percurso FAKE (apenas para testes temporariamente)
 }


 void iniciarLORA () 
  {
     mySerial.listen();                   
       
     emissor.init();                                            //Inicia o módulo

     emissor.SetMode(MODE_NORMAL);                              //Modo de funcionamento do módulo
     emissor.SetAddressH(0);                                    //Endereço H(?)
     emissor.SetAddressL(0);                                    //Endereço L(?)
     emissor.SetAirDataRate(ADR_2400);                          //AirDataRate 2400kbps
     emissor.SetUARTBaudRate(UDR_9600);                         //BAUDRate 9600
     emissor.SetChannel(23);                                    //Canal 23
     emissor.SetParityBit(PB_8N1);                              //Bit Paridade 8N1
     emissor.SetTransmitPower(OPT_TP30);                        //Força de transmissão 30db (Para módulos de 1W/8km definir como 30)
     //emissor.SetTransmitPower(OPT_TP20);                      //Força de transmissão 20db (Para módulos 100mW/3km definir como 20)
     emissor.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor.SetTransmissionMode(OPT_FMDISABLE);                //Transmission Mode
     emissor.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor.PrintParameters();                                 //Exibe os parâmetros configurados
  } //Fim iniciarLORA 

  void percursoFake() //Função que gera um percurso FAKE (apenas para testes temporariamente)
 {

   int vel = 100; //Define de quanto em quanto o loop irá avançar até atingir o valor limite definido no while
   int LO = 4660; //Valores de longitude
   int NS = 2370; //Valores de latitude

   while (LO > 1880)
    {
     LO -= vel ;
     //mySerial.print("[06,-26.242370,-48.64"+(String)LO+"]*"); //Para cada equipe, substituir de acordo, para que os módulos não enviem informações idênticas
     //mySerial.print("[07,-26.242370,-48.64"+(String)LO+"]*");
     mySerial.print("[08,-26.242370,-48.64"+(String)LO+"]*");
     delay(random(1000, 1500));
    }
   
   while (NS < 5150)
    {
     LO = 1880;
     NS += vel;
     //mySerial.print("[06,-26.24"+(String)NS+",-48.641880]*");
     //mySerial.print("[07,-26.24"+(String)NS+",-48.641880]*");
     mySerial.print("[08,-26.24"+(String)NS+",-48.641880]*");
     delay(random(1000, 1500));
    }
    
   while (LO < 4660)
    {
     NS = 5150;
     LO += vel;
     //mySerial.print("[06,-26.245150,-48.64"+(String)LO+"]*");
     //mySerial.print("[07,-26.245150,-48.64"+(String)LO+"]*");
     mySerial.print("[08,-26.245150,-48.64"+(String)LO+"]*");
     delay(random(1000, 1500));
    }
    
   while (NS > 2370)
    {
     NS -= vel;
     //mySerial.print("[06,-26.24"+(String)NS+",-48.644660]*");
     //mySerial.print("[07,-26.24"+(String)NS+",-48.644660]*");
     mySerial.print("[08,-26.24"+(String)NS+",-48.644660]*");
     delay(random(1000, 1500));
    }
}
