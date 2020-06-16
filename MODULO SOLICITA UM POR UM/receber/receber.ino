/*
//Pinos digitais utilizados - 2,3,4,5,6,7,8,9,10,11,12,13

                                    **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                                        ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                                          ******O USO DE LEDS É APENAS PARA FINS DE TESTES TEMPORÁRIOS, A VERSÃO FINAL NÃO TERÁ LEDS******
 
 Pinos Utilizados pela Shield Ethernet                   SoftSerial (Saídas Digitais)       Objeto EBYTE (Saídas Digitais)    LEDs Indicativos
  ** MOSI - pin 11                                                                           ** 5, 4, 2 - M0, M1, AUX          ** 9 - LED Equipe 6
  ** MISO - pin 12                                        ** 3,6 -  Rx, Tx (LORA)                                             ** 8 - LED Equipe 8
  ** CLK - pin 13                                                                                                             ** 7 - LED Equipe 7
  ** CS - pin 10
  
 */

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <EBYTE.h>
#include <SoftwareSerial.h>
#include <SD.h>

#define ARDUINO_CLIENT_ID "ARDUINO"      // Client ID for Arduino pub/sub

#define LED1 9
#define LED2 8
#define LED3 7

char mensagem[27];                      //Array de caracteres que irá armazenar a mensagem a ser recebida
size_t bytesRecebidos;
SoftwareSerial mySerial(3,6);           //Rx - Tx
File myFile;

EBYTE emissor(&mySerial, 5, 4, 2);

// Networking details
byte mac[]    = {  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };  // Ethernet shield (W5100) MAC address
IPAddress ip(192, 168, 3, 105);                           // Ethernet shield (W5100) IP address
IPAddress server(192, 168, 3, 114);                       // MTTQ server IP address

EthernetClient ethClient;
PubSubClient client(ethClient);
 

void setup() {

   pinMode(LED1, OUTPUT);
   pinMode(LED2, OUTPUT);
   pinMode(LED3, OUTPUT);

   Serial.begin(9600);                //Inicia a serial física
   mySerial.begin(9600);              //Inicia a serial lógica
     
   client.setServer(server, 1883);    //Endereço IP e porta do servidor MQTT
 
   Ethernet.begin(mac, ip);           //Inicia o Ethernet shield

  if (!SD.begin(8)) {
    Serial.println("Erro SD");
    while (1);
  }
  Serial.println("SD iniciado");

   iniciarLORA();
}

void loop()  
  {
   /*if (!client.connected())
      {
       reconectar();
      } */
      solicitar();
  }

void iniciarLORA ()   //Função que define os parâmetros de configuração do módulo LORA
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
     emissor.SetTransmitPower(OPT_TP30);                        //Força de transmissão 30db (Para módulos de 1W/8km)
     //emissor.SetTransmitPower(OPT_TP20);                        //Força de transmissão 20db (Para módulos 100mW/3km)
     emissor.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor.SetTransmissionMode(OPT_FMDISABLE);                //Transmission Mode
     emissor.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor.PrintParameters();                                 //Exibe os parâmetros configurados
    } //Fim iniciarLORA 

void reconectar()                   //Função que reconecta caso a conexão com o MQTT seja perdida
  {
     // Loop até reconectar
     while (!client.connected()){
       Serial.print("Attempting MQTT connection ... ");

       // Attempt to connect
       if (client.connect(ARDUINO_CLIENT_ID)){
           Serial.println("connected");
          } 
       else{
           Serial.print("Connection failed, state: ");
           Serial.print(client.state());
           Serial.println(", retrying in 3 seconds");
           delay(3000); // Aguarda 3 seg antes de tentar conectar novamente
        }
      }
  }

  void solicitar(){                       //Função que solicita dados aos módulos
  unsigned long pausa;

  for (byte a = 8; a < 11 ; a++){
  pausa = millis() + 3000;
  
  if (a < 10){
    Serial.println("0"+(String)a+"*");
    mySerial.print("0"+(String)a+"*");
    while (pausa > millis()){
    Receber();  
    }
  }
  if (a >= 10){
    Serial.println((String)a+"*");
    mySerial.print((String)a+"*");
    while (pausa > millis()){
    Receber(); 
    }
  }
 }
}

void Receber(){                          //Função que recebe os dados dos módulos
          
  //Limpa a variavel mensagem, antes do uso
     for (byte a = 0; a < 26; a++ ){
         mensagem[a] = "";
      }
     
     //Recebe dados do buffer até encontrar o caracter * que indica o fim da mensagem          
     while (mySerial.available() > 0){
         bytesRecebidos = mySerial.readBytesUntil('*', mensagem, 26);
        }
      Publicar();
  }

void Publicar(){                      //Função que publica os dados recebidos
      if(mensagem[0] == '[' && mensagem[25] == ']'){
      
      String id_equipe = String(mensagem[1])+""+String(mensagem[2]); //cria uma string com os valores contidos em mesagem[1] e mensagem[2]
      int equipe = id_equipe.toInt();   //converte a string em int para uso no switch case
                   
         myFile = SD.open("team"+id_equipe+".txt", FILE_WRITE);   //Abre o arquivo para edição
         if(myFile){
            Serial.print("Gravando LOG no SD...  ");
            myFile.println(mensagem);
            myFile.close();   //Fecha o arquivo após a exibição
         }
        Serial.println(mensagem); //Exibe a mensagem na serial (apenas para verificação e teste)
         switch (equipe){
          
         case 8:
         client.beginPublish ("/app/dados/equipe8", 26, false); //Inicia a publicação no MQTT
         client.print(mensagem); //Publica a mensagem
         client.endPublish(); //Encerra a publicação no MQTT
      
         digitalWrite(LED1, HIGH);
         delay(500);
         digitalWrite(LED1, LOW);
         break;
        
         case 9:
         client.beginPublish ("/app/dados/equipe9", 26, false); //Inicia a publicação no MQTT
         client.print(mensagem); //Publica a mensagem
         client.endPublish(); //Encerra a publicação no MQTT
     
         digitalWrite(LED2, HIGH);
         delay(500);
         digitalWrite(LED2, LOW);
         break;
     
         case 10:
         client.beginPublish ("/app/dados/equipe10", 26, false); //Inicia a publicação no MQTT
         client.print(mensagem); //Publica a mensagem
         client.endPublish(); //Encerra a publicação no MQTT
    
         digitalWrite(LED3, HIGH);
         delay(500);
         digitalWrite(LED3, LOW);
         break;
           
         default:
         return;
        }
    }
  }
