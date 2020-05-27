/*
//Pinos digitais utilizados - 2,3,4,5,6,7,8,9,10,11,12,13

                                    **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                                        ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                                          ******O USO DE LEDS É APENAS PARA FINS DE TESTES TEMPORÁRIOS, A VERSÃO FINAL NÃO TERÁ LEDS******
 
 Pinos Utilizados pela Shield Ethernet                   SoftSerial (Saídas Digitais)       Objeto EBYTE (Saídas Digitais)    LEDs Indicativos
  ** MOSI - pin 11                                       						                        ** 5, 4, 2 - M0, M1, AUX          ** 9 - LED Equipe 6
  ** MISO - pin 12                                        ** 3,6 -  Rx, Tx (LORA)                                             ** 8 - LED Equipe 8
  ** CLK - pin 13                                                                                                             ** 7 - LED Equipe 7
  ** CS - pin 10
  
 */

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <EBYTE.h>
#include <SoftwareSerial.h>

#define ARDUINO_CLIENT_ID "ARDUINO"           // Client ID for Arduino pub/sub
#define PUB_6 "/app/dados/equipe6"            // Tópico MQTT para equipe 6
#define PUB_7 "/app/dados/equipe7"   					// Tópico MQTT para equipe 7
#define PUB_8 "/app/dados/equipe8"   					// Tópico MQTT para equipe 8

#define LED1 9
#define LED2 8
#define LED3 7

char mensagem[25];  //Array de caracteres que irá armazenar a mensagem a ser recebida
size_t bytesRecebidos;

SoftwareSerial mySerial(3,6); 				        //Rx - Tx

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

	 Serial.begin(9600); //Inicia a serial física
	 mySerial.begin(9600); //Inicia a serial lógica

  	 
  	 client.setServer(server, 1883);  // Endereço IP e porta do servidor MQTT
 
  	 
  	 Ethernet.begin(mac, ip); // Inicia o Ethernet shield

	 iniciarLORA();
}

void loop()  
	{
 	 if (!client.connected())
     	{
     	 reconectar();
    	} 
 
     receberEnviar();
     delay (1000);  
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
     emissor.SetWORTIming(OPT_WAKEUP250);                      //WakeUP Time(?) 2000
     emissor.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor.SetTransmissionMode(OPT_FMDISABLE);                //Transmission Mode
     emissor.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor.PrintParameters();                                 //Exibe os parâmetros configurados
  	} //Fim iniciarLORA 

void reconectar()
	{
  	 // Loop até reconectar
  	 while (!client.connected()) 
  	 	{
     	 Serial.print("Attempting MQTT connection ... ");

    	 // Attempt to connect
    	 if (client.connect(ARDUINO_CLIENT_ID)) 
    	 	{
      		 Serial.println("connected");
       		} 
    	 else 
    		{
      		 Serial.print("Connection failed, state: ");
      		 Serial.print(client.state());
      		 Serial.println(", retrying in 3 seconds");
      		 delay(3000); // Aguarda 3 seg antes de tentar conectar novamente
    		}
  		}
	}

void receberEnviar()
	{
  //Limpa a variavel mensagem, antes do uso
     for (byte a = 0; a < 26; a++ )
     	{
         mensagem[a] = "";
    	}
     
     //Recebe dados do buffer até encontrar o caracter * que indica o fim da mensagem          
     while (mySerial.available() > 0)
     	{
       	 bytesRecebidos = mySerial.readBytesUntil('*', mensagem, 26);
      	}
       
  //Analisa a mensagem recebida e publica no tópico de acordo com a ID da equipe (a ID da equipe está na posição 2 do array mensagem)   
     switch (mensagem[2])
     	{
        case '6':
         client.beginPublish (PUB_6, 26, false); //Inicia a publicação no MQTT
         client.print(mensagem); //Publica a mensagem
         client.endPublish(); //Encerra a publicação no MQTT
         Serial.println(mensagem); //Exibe a mensagem na serial (apenas para verificação e teste)
         break;
        
         case '7':
         client.beginPublish (PUB_7, 26, false);
         client.print(mensagem);
         client.endPublish();
         Serial.println(mensagem);
         break;
     
       	 case '8':
         client.beginPublish (PUB_8, 26, false);
         client.print(mensagem);
         client.endPublish();
         Serial.println(mensagem);
         break;
           
         default:
         return;
      	}
  	}
