/*
                                                                     *******************RECEPTOR*******************
                                    **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                                        ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***

                                           
 Pinos Utilizados pela Shield Ethernet                   SoftSerial (Pinos Digitais)       Objeto EBYTE (Pinos Digitais)        LEDs (Pinos Analógicos)   
  **50, 51, 52      (Arduino Mega)                        ** 12,3 -  Rx, Tx                  ** 6,5,2 - M0, M1, AUX                ** A0 - Status MQTT  
  **10, 11, 12, 13  (Arduino UNO)                                                                                                  ** A1 - Status SD
                                                                                                                                   ** A3 - Status Power 
*/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SdFat.h>
#include <EBYTE.h>
#include <SoftwareSerial.h>

SdFat SD;
File myFile;     //Objeto File

byte mac[]    = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };       // Ethernet shield (W5100) MAC address
byte ip[]     = { 192, 168, 1, 49  };                         // Ethernet shield (W5100) IP address
byte server[] = { 192, 168, 1, 104 };                         // IP do servidor MQTT

//char gruposEquipes[] = {'A','B'};                 //Id dos grupos que estão na prova - Cada grupo deve conter preferencialmente 10 equipes (ou módulos)
char  gruposEquipes[] = {'B'};                      //Id dos grupos que estão na prova - Cada grupo deve conter preferencialmente 10 equipes (ou módulos)
char  idModulo       = 30;                          //Define o id deste receptor
char  canalModulo    = 5;                           //Define o canal em que este módulo irá operar
int   qtdEmissores   = 2;                           //Define a quantidade de emissores disponíveis, somando os emissores de todos os grupos 

boolean newData = false;                         //Indica se novos dados foram recebidos
String  mensagem;                                //Armazena a mensagem contendo todos os dados enviados de todos os emissores
int     qtdCharRcb  = 0;                         //Contador que armazena quantos caracteres foram recebidos pelo módulo, vindos dos rastreadores

#define ARDUINO_CLIENT_ID "ARDUINO"              //ID do cliente para publicação no broker MQTT

#define LED8  A0                                 //LED de status MQTT na porta A0
#define LED9  A1                                 //LED de status SD na porta A1
#define LED10 A3                                 //LED de status Power

SoftwareSerial mySerial(12,3);                   //Software serial do módulo LORA (12-Rx - 3-Tx)
EBYTE emissor(&mySerial, 6, 5, 2);               //Objeto Ebyte

EthernetClient  ethClient;                       //Objeto EthernetClient
PubSubClient    client(ethClient);               //Objeto PubSubClient

void iniciarLORA(){                              //Função que define os parâmetros de configuração do módulo LORA
     mySerial.listen();                   
       
     emissor.init();                                            //Inicia o módulo

     emissor.SetMode(MODE_NORMAL);                              //Modo de funcionamento do módulo
     emissor.SetAddressH(0);                                    //Endereço H. Pode Variar entre 0 e 254 (Preferível não alterar)
     emissor.SetAddressL(idModulo);                             //Endereço L. Pode Variar entre 0 e 254 
     emissor.SetAirDataRate(ADR_2400);                          //AirDataRate 2400kbps
     emissor.SetUARTBaudRate(UDR_9600);                         //BAUDRate 9600
     emissor.SetChannel(canalModulo);                           //Canal de operação 
     emissor.SetParityBit(PB_8N1);                              //Bit Paridade 8N1
     emissor.SetTransmitPower(OPT_TP30);                        //Força de transmissão 30db (Para módulos de 1W/8km)
     //emissor.SetTransmitPower(OPT_TP20);                      //Força de transmissão 20db (Para módulos 100mW/3km)
     emissor.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor.SetTransmissionMode(OPT_FMENABLE);                 //Transmission Mode (Fixed ou Transparent)
     emissor.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor.PrintParameters();                                 //Exibe os parâmetros configurados
    } //Fim iniciarLORA 

void iniciar_SD(){                                              //Função que inicia o módulo SD
   
  while (!SD.begin(4)){
    Serial.println("ERRO SD");
    digitalWrite(LED9, HIGH);
    delay(1000);
  } 
}// Fim iniciar SD
   
void salvar_SD(){                                    //Função que salva os dados recebidos no cartão SD
  myFile = SD.open("log.txt", FILE_WRITE);           //Abre o arquivo para edição
  if(myFile){
    myFile.println(mensagem);                        //Edita o arquivo com os dados recebidos
    myFile.close();                                  //Fecha o arquivo após a edição
    Serial.println("Salvo SD");                      //Exibe mensagem de confirmação de edição concluida
    return;
  }else {
    Serial.println("ERRO SALVAR SD");
    return;
   }
} //Fim salva_SD

void conectar_MQTT(){                                          //Função que conecta ao MQTT
  while (!client.connect(ARDUINO_CLIENT_ID)){                   
    
    Serial.println("FALHA MQTT");
    digitalWrite(LED8, HIGH);
    client.connect(ARDUINO_CLIENT_ID);
    delay(1000);                                              // Aguarda 1 seg antes de tentar conectar novamente
  }
}  //Fim conectar_MQTT

void receber() {

  int tempoEspera = 1300 * qtdEmissores;                //Define por quantos segundos o módulo vai aguardar por dados dos rastreadores (qtd de rastreadores * 1300ms de espera por módulo)
  const int numChars = 49 * qtdEmissores;               //Limite máximo de caracteres a serem recebidos                                (qtd de rastreadores * tamanho da mensagem enviada (49 char))
  byte dados[numChars];                                 //Array de bytes que irá armazenar os dados fornecidos pelos rastreadores
  unsigned long pausa = millis() + tempoEspera;         //Adiciona os segundos de espera a função millis()
  static int ndx = 0;                                   //Indice que conta quantos caracteres foram recebidos
  byte rc;                                              //Armazena temporariamente cada caractere recebido

   
  while (pausa > millis()){
    
    if (mySerial.available() > 0){
      rc = mySerial.read();
      dados[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {               //Caso a quantidade de dados recebida atinja o limite
           
           dados[ndx] = '\0';              //Atribui o caracter "NULL" e com isso encerra o recebimento
           mensagem = (char*)dados;
           qtdCharRcb = ndx;               //Atribui a quantidade de caracteres recebidos para uso na função publicar
           ndx = 0;                        //Define o indice novamente para 0
           newData = true;                 //Define newData como true indicando que há novos dados recebidos
           publicar();                     //Chama a função publicar para enviar os dados via MQTT e salvar no cartão SD
           break;                          //Força o encerramento da função receber
      }
    }
  }
  if (ndx > 0) {              //Caso o tempo limite de recebimento se esgote e não seja preenchido o buffer
    
    dados[ndx] = '\0';        //Atribui o caracter "NULL" e com isso encerra o recebimento
    mensagem = (char*)dados;
    qtdCharRcb = ndx;         //Atribui a quantidade de caracteres recebidos para uso na função publicar
    ndx = 0;                  //Define o indice novamente para 0
    newData = true;           //Define newData como true indicando que há novos dados recebidos
    publicar();               //Chama a função publicar para enviar os dados via MQTT e salvar no cartão SD
  } 
}    //Fim Receber

void solicitar(){                                                     //Função que solicita dados dos rastreadores
  byte indice = (sizeof(gruposEquipes));                              //Define quantos grupos existem de acordo com o tamanho do vetor gruposEquipes

  for (byte a = 0; a < indice; a++){                                  //Loop que a cada interação envia o ID de um grupo para solicitar informações aos módulos
    
    conectar_MQTT();                                                  //A cada iteração do loop verifica se a conexão com o MQTT está ativa
    digitalWrite(LED8, LOW);
    iniciar_SD();                                                     //A cada iteração do loop verifica se o módulo SD está ativo
    digitalWrite(LED9, LOW);
   
    byte solicitacao[] = {0xFF, 0xFF, canalModulo, gruposEquipes[a]};       //Solicita dados a todos os módulos (0xFF 0xFF) operando no canal 5 (0x05) que estejam no grupo chamado
    mySerial.write(solicitacao,sizeof(solicitacao));
    receber();                                                       //Função que recebe dados dos rastreadores   
  }
} //Fim Solicitar()

void publicar(){                                               //Função que publica os dados recebidos no MQTT e salva no cartão SD

  if (newData == true) {                                       //Caso tenha dados recebidos
     
    newData = false;                                           //Define dados recebidos como falso
    String topico_str = "/app1/dados/equipe00";                //Cria o endereço do tópico MQTT onde serão publicados os dados Ex:. /app/dados/equipe07 ou /app/dados/equipe10
    char topico_char[21];                                      //Vetor que irá armazenar o endereço do tópico em formato char
    topico_str.toCharArray(topico_char, 21);                   //Converte o endereço criado em char e atribui ao vetor topico_char
                   
    Serial.println(mensagem);                                  //Exibe os dados recebidos na Serial
    salvar_SD();                                               //Salva os dados recebidos no SD
                                                    
    client.beginPublish(topico_char,qtdCharRcb,false);         //Inicia a publicação no MQTT      **Parâmetros - topico_char(endereço do topico MQTT) / qtdCharRcb(quantos caracteres serão publicados) / false(Não alterar)** 
                                
    client.print(mensagem);                                    //Publica a mensagem
    client.endPublish();                                       //Encerra a publicação no MQTT
    Serial.end();                                              //Encerra a serial para limpar o buffer de recebimento
    mySerial.end();                                            //Encerra a softSerial para limpar o buffer de recebimento
    delay(500);
    Serial.begin(9600);                                        //Reinicia a Serial
    mySerial.begin(9600);                                      //Reinicia a softSerial
  }
}// Fim Publicar()

void setup(){

  Serial.begin(9600);                               //Inicia a Serial
  mySerial.begin(9600);                             //Inicia a softSerial

  Serial.println("Seriais Física e Lógica OK");

  pinMode(LED8,  OUTPUT);                 
  pinMode(LED9,  OUTPUT);
  pinMode(LED10, OUTPUT);

  digitalWrite(LED8,  HIGH);
  digitalWrite(LED9,  HIGH);
  digitalWrite(LED10, HIGH);

  Serial.println("LED OK");

  iniciarLORA();                                    //Inicia o módulo LORA com os parâmetros definidos na função iniciar_LORA
  delay(500);

  iniciar_SD();                                    //Inicia o módulo SD
  Serial.println("SD OK"); 
  digitalWrite(LED9, LOW);
  Ethernet.init(10);
  Ethernet.begin(mac, ip);                         //Inicia o Ethernet shield
  client.setServer(server, 1883);                  //Inicia e define IP e porta do servidor MQTT
  conectar_MQTT();                                 //Conecta ao broker MQTT
  digitalWrite(LED8, LOW);
  Serial.println("REDE/MQTT OK");
} //Fim Setup()

void loop() {
  
  solicitar();  //Solicita dados aos módulos transmissores
}   //Fim loop
