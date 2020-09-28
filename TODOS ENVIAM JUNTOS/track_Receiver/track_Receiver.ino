/*
                                                         ********Pinos digitais utilizados - 2,3,4,5,6,7,10,11,12,13***********
                                                               *******Pinos analogicos utilizados - A0, A1, A2 **********

                                    **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                                        ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                                          ******O USO DE LEDS É APENAS PARA FINS DE TESTES TEMPORÁRIOS, A VERSÃO FINAL NÃO TERÁ LEDS******
 
 Pinos Utilizados pela Shield Ethernet                   SoftSerial (Saídas Digitais)       Objeto EBYTE (Saídas Digitais)    
  ** MOSI - pin 11                                                                          ** 6,5,2 - M0, M1, AUX            
  ** MISO - pin 12                                       ** 7,3 -  Rx, Tx (LORA)                                              
  ** CLK  - pin 13                                                                                                            
  ** CS   - pin 10
  ** SD   - pin 4
  
 */

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SdFat.h>
#include <EBYTE.h>
#include <SoftwareSerial.h>

byte mac[]    = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };       // Ethernet shield (W5100) MAC address
byte ip[]     = { 192, 168, 1, 49  };                          // Ethernet shield (W5100) IP address
byte server[] = { 192, 168, 1, 156 };                         // IP do servidor MQTT

SdFat SD;
File myFile;                             //Objeto File do SD
size_t bytesRecebidos;

char mensagem[148];                      //mensagem contendo os dados recebidos dos módulos transmissores

#define ARDUINO_CLIENT_ID "ARDUINO"      //ID do cliente para publicação no broker MQTT
#define LED8 A0                          //LED status MQTT na porta A0
#define LED9 A1                          //LED status SD na porta A1

SoftwareSerial serialLORA(7,3);          //Software serial do módulo LORA (7-Rx - 3-Tx)
EBYTE emissor(&serialLORA, 6, 5, 2);     //Objeto Ebyte

EthernetClient ethClient;               
PubSubClient client(ethClient);          //Objeto PubSubClient

void setup() {

iniciar_SD();
Ethernet.begin(mac, ip);                 //Inicia o Ethernet shield
Serial.begin(9600);                      //Inicia Serial Fisíca
serialLORA.begin(9600);                  //Inicia Serial Lógica
client.setServer(server, 1883);          //Inicia e define IP e porta do servidor MQTT

Serial.println("Rede e Seriais OK");

pinMode(LED8, OUTPUT);                 
pinMode(LED9, OUTPUT);

digitalWrite(LED8, LOW);
digitalWrite(LED9, LOW);

Serial.println("LED OK");

iniciar_LORA();                    //Inicia o módulo LORA com os parâmetros definidos na função iniciar_LORA
delay(500);
conectar_MQTT();                   //Conecta ao broker MQTT
digitalWrite(LED8, LOW);
iniciar_SD();                      //Inicia o módulo SD
digitalWrite(LED9, LOW);

Serial.println("LORA, SD, MQTT OK");
}

void iniciar_LORA (){
     serialLORA.listen();                   
       
     emissor.init();                                            //Inicia o módulo

     emissor.SetMode(MODE_NORMAL);                              //Modo de funcionamento do módulo
     emissor.SetAddressH(0);                                    //Endereço H(Não alterar)
     emissor.SetAddressL(0);                                    //Endereço L(Alterar este valor para trocar o endereço do módulo)
     emissor.SetAirDataRate(ADR_2400);                          //AirDataRate 2400kbps
     emissor.SetUARTBaudRate(UDR_9600);                         //BAUDRate 9600
     emissor.SetChannel(23);                                    //Canal 23 (Pode variar de 0 até 32)
     emissor.SetParityBit(PB_8N1);                              //Bit Paridade 8N1
     emissor.SetTransmitPower(OPT_TP30);                        //Força de transmissão 30db (Para módulos de 1W/8km)
     //emissor.SetTransmitPower(OPT_TP20);                      //Força de transmissão 20db (Para módulos 100mW/3km)
     emissor.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor.SetTransmissionMode(OPT_FMDISABLE);                //Transmission Mode (Fixed or Transparent)
     emissor.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor.PrintParameters();                                 //Exibe os parâmetros configurados
  } //Fim iniciarLORA 

  void iniciar_SD(){                                            //Função que inicia o módulo SD
   
    while (!SD.begin(4)){
    Serial.println("ERRO SD");
    digitalWrite(LED9, HIGH);
    delay(1000);
   }
  
}// Fim iniciar_SD

void salvar_SD() {                                          //Função que salva os dados recebidos no cartão SD
    myFile = SD.open("log.txt", FILE_WRITE);                //Abre o arquivo para edição
    if(myFile){
      myFile.println(mensagem);                            //Edita o arquivo com os dados recebidos
       myFile.close();                                      //Fecha o arquivo após a edição
       Serial.println("Salvo SD");                          //Exibe mensagem de confirmação de edição concluida
       return;
      } 
    else {
      Serial.println("ERRO SALVAR SD");
      return;
    }
} //Fim salva_SD

void conectar_MQTT(){                                          //Função que conecta ao MQTT
  while (!client.connect(ARDUINO_CLIENT_ID)){                   
        Serial.println("FALHA MQTT");
        digitalWrite(LED8, HIGH);
        client.connect(ARDUINO_CLIENT_ID);
        delay(1000);                                           // Aguarda 1 seg antes de tentar conectar novamente
  }
}  //Fim conectar_MQTT

void Receber(){                                              //Função que recebe os dados dos módulos
    for (unsigned int a = 0; a < 147; a++ ){                           //Limpa a variavel mensagem, antes do uso
     mensagem[a] = '0';
     }
    serialLORA.listen();
    while (serialLORA.available() > 0){                          
         bytesRecebidos = serialLORA.readBytesUntil('@', mensagem, 147);    //Recebe dados do buffer até encontrar o caracter * que indica o fim da mensagem
        }
}       //Fim Receber

void Publicar(){                                                   //Função que publica os dados recebidos
      String topico_str = "/app1/dados/equipe00";                  //Cria o endereço do tópico MQTT onde serão publicados os dados Ex:. /app/dados/equipe07 ou /app/dados/equipe10
      char topico_char[21];                                        //Vetor que irá armazenar o endereço do tópico em formato char
      topico_str.toCharArray(topico_char, 21);                     //Converte o endereço criado em char e atribui ao vetor topico_char

        if(mensagem[0] == '['){
        Serial.println(mensagem);
        salvar_SD();
                                                    
         client.beginPublish(topico_char,147,false);                //Inicia a publicação no MQTT
         client.print(mensagem);                                   //Publica a mensagem
         client.endPublish();                                      //Encerra a publicação no MQTT
       }
}

    void loop() {
      Receber();  //Solicita dados aos módulos transmissores
      Publicar();
      conectar_MQTT();
      //iniciar_SD;
}   //Fim loop
