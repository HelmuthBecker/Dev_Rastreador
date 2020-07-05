/*
                                                         ********Pinos digitais utilizados - 2,3,4,5,6,7,10,11,12,13***********
                                                               *******Pinos analogicos utilizados - A0, A1, A2 **********

                                    **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                                        ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                                          ******O USO DE LEDS É APENAS PARA FINS DE TESTES TEMPORÁRIOS, A VERSÃO FINAL NÃO TERÁ LEDS******
 
 Pinos Utilizados pela Shield Ethernet                   SoftSerial (Saídas Digitais)       Objeto EBYTE (Saídas Digitais)    LEDs Indicativos
  ** MOSI - pin 11                                                                           ** 7,5,2 - M0, M1, AUX           ** A2 - LED Equipe 10
  ** MISO - pin 12                                        ** 3,6 -  Rx, Tx (LORA)                                             ** A1 - LED Equipe 09
  ** CLK - pin 13                                                                                                             ** A0 - LED Equipe 08
  ** CS - pin 10
  ** SD - pin 4
  
 */

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SD.h>
#include <EBYTE.h>
#include <SoftwareSerial.h>

byte mac[]    = {  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };  // Ethernet shield (W5100) MAC address
byte ip[] = { 192, 168, 3, 1 };                          // Ethernet shield (W5100) IP address
byte server[] = {192,168,3,2};

File myFile;                             //Objeto File

size_t bytesRecebidos;

char mensagem[45];                       //mensagem contendo os dados recebidos dos módulos transmissores

#define ARDUINO_CLIENT_ID "ARDUINO"      // ID do cliente para publicação no broker MQTT
#define LED8 A0                          //LED equipe 8 na porta A0
#define LED9 A1                          //LED equipe 9 na porta A1
#define LED10 A2                         //LED equipe 10 na porta A2

SoftwareSerial mySerial(3,6);           //Software serial do módulo LORA (3-Rx - 6-Tx)
EBYTE emissor(&mySerial, 7, 5, 2);      // Objeto Ebyte

EthernetClient ethClient;               
PubSubClient client(ethClient);         //Objeto PubSubClient

void setup() {

pinMode(LED8, OUTPUT);                 
pinMode(LED9, OUTPUT);
pinMode(LED10, OUTPUT);

digitalWrite(LED8, LOW);
digitalWrite(LED9, LOW);
digitalWrite(LED10, LOW);

Ethernet.begin(mac, ip);           //Inicia o Ethernet shield
Serial.begin(9600);                //Inicia Serial Fisíca
mySerial.begin(9600);              //Inicia Serial Lógica
client.setServer(server, 1883);    //Inicia e define IP e porta do servidor MQTT

iniciar_LORA();                    //Inicia o módulo LORA com os parâmetros definidos na função iniciar_LORA
conectar_MQTT();                   //Conecta ao broker MQTT
iniciar_SD();                      //Inicia o módulo SD

}

void loop() {

      solicitar();                //Solicita dados aos módulos transmissores

}   //Fim loop

void iniciar_LORA ()              //Função que define os parâmetros de configuração do módulo LORA
  {
     mySerial.listen();                   
       
     emissor.init();                                            //Inicia o módulo

     emissor.SetMode(MODE_NORMAL);                              //Modo de funcionamento do módulo
     emissor.SetAddressH(0);                                    //Endereço H(Não alterar)
     emissor.SetAddressL(0);                                    //Endereço L(Alterar este valor para trocar o endereço do módulo)
     emissor.SetAirDataRate(ADR_2400);                          //AirDataRate 2400kbps
     emissor.SetUARTBaudRate(UDR_9600);                         //BAUDRate 9600
     emissor.SetChannel(23);                                    //Canal 23 (Pode variar de 0 até 32)
     emissor.SetParityBit(PB_8N1);                              //Bit Paridade 8N1
     emissor.SetTransmitPower(OPT_TP30);                        //Força de transmissão 30db (Para módulos de 1W/8km)
     //emissor.SetTransmitPower(OPT_TP20);                        //Força de transmissão 20db (Para módulos 100mW/3km)
     emissor.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor.SetTransmissionMode(OPT_FMDISABLE);                //Transmission Mode (Fixed or Transparent)
     emissor.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor.PrintParameters();                                 //Exibe os parâmetros configurados
    } //Fim iniciarLORA 

void iniciar_SD(){                      //Função que inicia o módulo SD
    while (!SD.begin(4)){
    Serial.println("ERRO SD"); 
     } //Serial.println("SD OK");
}// Fim iniciar_SD

void salvar_SD(char id_equipe[3]) {                            //Função que salva os dados recebidos no cartão SD
    String nomeArquivo = "team"+String(id_equipe)+".txt";      //Cria o nome do arquivo de LOG de acordo com o ID da equipe recebido
    myFile = SD.open(nomeArquivo, FILE_WRITE);                 //Abre o arquivo para edição
    if(myFile){                                                
       myFile.println(mensagem);                               //Edita o arquivo com os dados recebidos
       myFile.close();                                         //Fecha o arquivo após a edição
       Serial.println("Salvo SD");                             //Exibe mensagem de confirmação de edição concluida
    }
} //Fim salva_SD

void conectar_MQTT(){                                           //Função que conecta ao MQTT
  while (!client.connect(ARDUINO_CLIENT_ID)){                   
        Serial.println("FALHA MQTT");
        client.connect(ARDUINO_CLIENT_ID);
        delay(1000); // Aguarda 1 seg antes de tentar conectar novamente
  
        /* if (client.connect(ARDUINO_CLIENT_ID)){
      
          } else{
           Serial.println("FALHA MQTT");
           client.connect(ARDUINO_CLIENT_ID);
           delay(1000); // Aguarda 1 seg antes de tentar conectar novamente
          }*/
  }
}  //Fim conectar_MQTT

void Receber(){                                              //Função que recebe os dados dos módulos
   for (byte a = 0; a < 44; a++ ){                           //Limpa a variavel mensagem, antes do uso
        mensagem[a] = "";
        }         
   while (mySerial.available() > 0){                          
         bytesRecebidos = mySerial.readBytesUntil('*', mensagem, 44);    //Recebe dados do buffer até encontrar o caracter * que indica o fim da mensagem
         }
        Publicar();                       //Aciona a função que irá publicar e salvar no SD os dados recebidos
}  //Fim Receber

void solicitar(){                         //Função que solicita dados aos módulos
  unsigned long pausa;

  for (byte a = 8; a < 11 ; a++){         //Loop que a cada interação envia o numero da equipe como ID para solicitar informações aos módulos
  pausa = millis() + 3000;                //Define uma pausa de 3 seg para que o módulo aguarde resposta com dados do módulo solicitado

 conectar_MQTT();                         //A cada iteração do loop verifica se a conexão com o MQTT está ativa
  
  if (a < 10){                            //Caso o numero da equipe seja menor que 10, adiciona um zero a esquerda como padronização e o char "*" de fim de mensagem
    Serial.println("0"+(String)a+"*");    
    mySerial.print("0"+(String)a+"*");
    while (pausa > millis()){             //Após o envio da solicitação, aguarda 3s por uma respota
    Receber();                            //Função que recebe os dados dos módulos
    }
  }
  if (a >= 10){                          //Caso o numero da equipe seja maior que 10, não adiciona nada, apenas o char "*" de fim de mensagem
    Serial.println((String)a+"*");
    mySerial.print((String)a+"*");
    while (pausa > millis()){             //Após o envio da solicitação, aguarda 3s por uma respota
    Receber();                            //Função que recebe os dados dos módulos 
    }
  }
 }
}

void Publicar(){                                                  //Função que publica os dados recebidos
      if(mensagem[0] == '[' && mensagem[43] == ']'){              //Verifica se a mensagem está completa
      //if(mensagem[0] == '['){
      
      char id_equipe [3] = { mensagem[1], mensagem[2] };          //Retira da mensagem a ID da equipe
      String topico_str = "/app/dados/equipe"+String(id_equipe);  //Cria o endereço do tópico MQTT onde serão publicados os dados
      char topico_char[21];
      topico_str.toCharArray(topico_char, 21);                    //Converte o endereço criado em CharArray

      Serial.println(mensagem);                                   //Exibe a mensagem na serial (apenas para verificação e teste)
                                     
         client.beginPublish(topico_char,44,false);               //Inicia a publicação no MQTT
         client.print(mensagem);                                  //Publica a mensagem
         client.endPublish();                                     //Encerra a publicação no MQTT
         salvar_SD(id_equipe);                                    //Aciona a função para salvar no SD e passa a ID da equipe como parâmetro

         if (String(id_equipe) == "10"){                          //Pisca LED indicativo (apenas a fins de teste)
         digitalWrite(LED10, HIGH);
         delay(500);
         digitalWrite(LED10, LOW); 
         }

         if (String(id_equipe) == "09"){
         digitalWrite(LED9, HIGH);
         delay(500);
         digitalWrite(LED9, LOW); 
         }

         if (String(id_equipe) == "08"){
         digitalWrite(LED8, HIGH);
         delay(500);
         digitalWrite(LED8, LOW); 
         }
         
        } 
    }
