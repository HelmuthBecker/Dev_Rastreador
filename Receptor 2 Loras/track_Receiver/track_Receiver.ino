/*
                                                                     *******************RECEPTOR*******************
                                    **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                                        ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***

                                           
 Pinos Utilizados pela Shield Ethernet                          Objeto EBYTE (Pinos Digitais)                     LEDs (Pinos Analógicos)   
  **50, 51, 52, 4, 10        (Arduino Mega)                     ** emissor1(&Serial1, 3, 2, 34);                  ** A0 - Status MQTT  
  **10, 11, 12, 13, 4, 10    (Arduino UNO)                      ** emissor2(&Serial2, 6, 5, 36);                  ** A1 - Status Lora 1
                                                                                                                  ** A2 - Status Cartão Sd 
                                                                                                                  ** A3 - Status Lora 2
                                                                                                                  ** A4 - PWR_Led

*/  

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SdFat.h>
#include <EBYTE.h>

SdFat SD;
File myFile;     //Objeto File

//CONFIGURAR ESTES VALORES DE ACORDO COM NECESSÁRIO PARA FUNCIONAMENTO
byte mac[]    = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };       // Ethernet shield (W5100) MAC address
byte ip[]     = { 192, 168, 1, 49  };                         // Ethernet shield (W5100) IP address
byte server[] = { 192, 168, 1, 104 };                         // IP do servidor MQTT


char   idSolicitacao   = 'B';                          //Id de solicitação de dados
char   idModulo1       = 31;                           //Define o id do modulo LORA1
char   idModulo2       = 32;                           //Define o id do modulo LORA2
char   canalModulo     = 5;                            //Define o canal em que os módulos irão operar
int    qtdEmissores    = 3;                            //Define a quantidade de módulos emissores disponíveis
//String topico_str      = "/rastreador/dados";        //Cria o endereço do tópico MQTT onde serão publicados os dados Ex:. /app/dados/equipe07 ou /app/dados/equipe10
String topico_str      = "/app1/dados/equipe00";       //Cria o endereço do tópico MQTT onde serão publicados os dados Ex:. /app/dados/equipe07 ou /app/dados/equipe10
int    tempoEspera     = 1000 * qtdEmissores;          //Define por quantos segundos o módulo vai aguardar por dados dos rastreadores (qtd de rastreadores * 1000ms de espera por módulo)
//FIM COFIGURAÇÕES

boolean newData1 = false;                         //Indica se novos dados foram recebidos pelo módulo Lora 1
String  mensagem1;                                //Armazena a mensagem contendo todos os dados enviados de todos os emissores de numeração 0 ou par
int     qtdCharRcb1  = 0;                         //Contador que armazena quantos caracteres foram recebidos pelo módulo LoRa 1, vindos dos emissores de numeração 0 ou par

boolean newData2 = false;                         //Indica se novos dados foram recebidos pelo módulo Lora 2
String  mensagem2;                                //Armazena a mensagem contendo todos os dados enviados de todos os emissores de numeração impar
int     qtdCharRcb2  = 0;                         //Contador que armazena quantos caracteres foram recebidos pelo módulo, vindos dos emissores de número impar

#define ARDUINO_CLIENT_ID "ARDUINO"               //ID do cliente para publicação no broker MQTT

#define MQTT_LED   A0                             //LED de status MQTT na porta A0
#define LORA1_LED  A1                             //LED de status SD na porta A1
#define SD_LED     A2
#define LORA2_LED  A3                             //LED de status Power
#define PWR_LED    A4

EBYTE emissor1(&Serial1, 3, 2, 34);               //Objeto Ebyte
EBYTE emissor2(&Serial2, 6, 5, 36);               //Objeto Ebyte

EthernetClient  ethClient;                        //Objeto EthernetClient
PubSubClient    client(ethClient);                //Objeto PubSubClient

/*
 * Função iniciarLORA
 * Executada uma vez dentro da função Setup, é responsável por definir e passar todos os parâmetros de funcionamento do módulo Lora em uso 
 * Após ser executada exibe no monitor serial os parametros que foram definidos no módulo.
 * Analise sempre essa informação, pois é um indicativo importante se o módulo está corretamente configurado ou não.
 * Erros no sketch ou mesmo pinos conectados incorretamente podem atrapalhar o funcionamento correto do módulo LORA.
 * Caso isso ocorra revise o sketch e as conexões do módulo.
 * Para mais informações de configuração e funcionamento do módulo consulte o Usermanual do mesmo.
 */

void iniciarLORA1(){                                             //Função que define os parâmetros de configuração do módulo LORA
     emissor1.init();                                            //Inicia o módulo

     emissor1.SetMode(MODE_NORMAL);                              //Modo de funcionamento do módulo
     emissor1.SetAddressH(0);                                    //Endereço H. Pode Variar entre 0 e 254 (Preferível não alterar)
     emissor1.SetAddressL(idModulo1);                            //Endereço L. Pode Variar entre 0 e 254 
     emissor1.SetAirDataRate(ADR_2400);                           //AirDataRate 2400kbps
     emissor1.SetUARTBaudRate(UDR_9600);                         //BAUDRate 9600
     emissor1.SetChannel(canalModulo);                           //Canal de operação 
     emissor1.SetParityBit(PB_8N1);                              //Bit Paridade 8N1
     emissor1.SetTransmitPower(OPT_TP30);                        //Força de transmissão 30db (Para módulos de 1W/8km)
     //emissor1.SetTransmitPower(OPT_TP20);                      //Força de transmissão 20db (Para módulos 100mW/3km)
     emissor1.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor1.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor1.SetTransmissionMode(OPT_FMENABLE);                 //Transmission Mode (Fixed ou Transparent)
     emissor1.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor1.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor1.PrintParameters();                                 //Exibe os parâmetros configurados

//Verifica se o módulo LoRa iniciou com os parâmetros corretos
     if (emissor1.GetModel() == 50 && emissor1.GetFeatures() == 30){
      Serial.println("Lora 1 OK");
     } else {
        digitalWrite(LORA1_LED,  HIGH);
        falhaLORA();
      }
} //Fim iniciarLORA1

void iniciarLORA2(){                                             //Função que define os parâmetros de configuração do módulo LORA
     emissor2.init();                                            //Inicia o módulo

     emissor2.SetMode(MODE_NORMAL);                              //Modo de funcionamento do módulo
     emissor2.SetAddressH(0);                                    //Endereço H. Pode Variar entre 0 e 254 (Preferível não alterar)
     emissor2.SetAddressL(idModulo2);                            //Endereço L. Pode Variar entre 0 e 254 
     emissor2.SetAirDataRate(ADR_2400);                          //AirDataRate 2400kbps
     emissor2.SetUARTBaudRate(UDR_9600);                         //BAUDRate 9600
     emissor2.SetChannel(canalModulo);                           //Canal de operação 
     emissor2.SetParityBit(PB_8N1);                              //Bit Paridade 8N1
     emissor2.SetTransmitPower(OPT_TP30);                        //Força de transmissão 30db (Para módulos de 1W/8km)
     //emissor2.SetTransmitPower(OPT_TP20);                      //Força de transmissão 20db (Para módulos 100mW/3km)
     emissor2.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor2.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor2.SetTransmissionMode(OPT_FMENABLE);                 //Transmission Mode (Fixed ou Transparent)
     emissor2.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor2.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor2.PrintParameters();                                 //Exibe os parâmetros configurados

//Verifica se o módulo LoRa iniciou com os parâmetros corretos
    if (emissor2.GetModel() == 50 && emissor2.GetFeatures() == 30){
      Serial.println("Lora 2 OK");      
    } else {
        digitalWrite(LORA2_LED,  HIGH);
        falhaLORA();
     }
  } //Fim iniciarLORA2

/*
 *Função falhaLORA
 *Caso algum módulo LORA não inicie corretamente, pausa a inicialização do receptor, escreve mensagem de erro na serial e acende LED indicativo 
 */

void falhaLORA(){
  bool falha = true;
  Serial.println("Erro módulo LORA");
  while (falha == true){    
    delay(10000);
  }
}

/*
 * Função iniciar_SD
 * Responsável por verificar se há um cartão SD conectado no slot SD da shield ethernet, e se o mesmo está formatado corretamente e pronto para uso.
 * Caso haja algu problema com cartão SD, entra em loop infinito exibindo a mensagem "ERRO SD" até que o SD seja trocado.
 */
 
void iniciar_SD(){                                              //Função que inicia o módulo SD
   
  if (!SD.begin(4)){
    Serial.println("ERRO SD");
    while (!SD.begin(4)){
      digitalWrite(SD_LED, HIGH);
      delay(1000);
    }
  } 
}// Fim iniciar SD

/*
 * Função salvar_SD
 * Cria um arquvo chamado "log.txt" caso não exista no cartão, ou abre para edição caso exista.
 * Esse arquivo irá salvar a mensagem contendo os dados emitidos pelos módulos transmissores
 * Caso não consiga criar ou editar o arquivo txt, exibe uma mensagem de "ERRO SALVAR SD"
 */
   
void salvar_SD(String mensagem){                     //Função que salva os dados recebidos no cartão SD
  myFile = SD.open("log.txt", FILE_WRITE);           //Abre o arquivo para edição
  if(myFile){
    myFile.println(mensagem);                       //Edita o arquivo com os dados recebidos
    myFile.close();                                  //Fecha o arquivo após a edição
    //Serial.println("Salvo SD");                      //Exibe mensagem de confirmação de edição concluida
    return;
  }else {
    Serial.println("ERRO SALVAR SD");
    return;
   }
} //Fim salva_SD

/*
 * Função conectar_MQTT
 * Tenta se conectar ao servidor MQTT remoto, utilizando os dados de IP[], SERVER[], MAC[] e  ARDUINO_CLIENT_ID fornecidos no início do sketch.
 * Caso não seja possível se conectar ao servidor MQTT entra em loop exibindo a mensagem "FALHA MQTT", e acendendo o LED indicativo correspondende.
 * Em caso de falha de conexão, verifique se os dados relacionados ao servidor estão corretos, se o servidor está ativo e se o cabo de rede está conectado.
 * Há um problema de hardware na shield ethernet, que faz com que ela não inicie o tráfego IP mesmo que a configuração esteja correta.
 * Caso após verificar o sketch, o servidor mqtt e os cabos de rede e mesmo com tudo OK a conexão não funcionar, verifique se o LED verde na porta de rede
 * da shield está aceso (o que indica trafégo de dados na porta), caso esteja apagado pressione o botão reset da shield e verifique se o LED acende.
 * Essa falha de hardware pode ser solucionada conectando um capacitor de 20uF entre o pino GND  e reset da shield. Porém toda vez que for necessário
 * fazer upload de sketch para o arduino é necessário desconectar esse capacitor da shield ou desconectar a shield do arduino.
 */

void conectar_MQTT(){                                          //Função que conecta ao MQTT
  if (!client.connect(ARDUINO_CLIENT_ID)){
    Serial.println("FALHA MQTT");
    while (!client.connect(ARDUINO_CLIENT_ID)){                   
      digitalWrite(MQTT_LED, HIGH);
      client.connect(ARDUINO_CLIENT_ID);
      delay(1000);                                              // Aguarda 1 seg antes de tentar conectar novamente
    }
  }
}  //Fim conectar_MQTT

/*
 * Função Receber
 * Responsável por receber os dados dos módulos transmissores.
 * É programada para esperar por um tempo determinado de segundos pelo recebimento, ou até que atinja o limite do buffer de recebimento
 * Ao fim de uma dessas opções chama a função publicar que irá publicar os dados no MQTT e salvar no cartão SD
 */

void receber() {
  unsigned long pausa = millis() + tempoEspera;          //Adiciona a função millis() aos segundos de espera
    
  const int numChars1 = (49 * qtdEmissores);            //Limite máximo de caracteres a serem recebidos                                (qtd de rastreadores * tamanho da mensagem enviada (49 char))
  byte dados1[numChars1];                               //Array de bytes que irá armazenar os dados fornecidos pelos rastreadores
  static int ndx1 = 0;                                  //Indice que conta quantos caracteres foram recebidos
  
  const int numChars2 = (49 * qtdEmissores);            //Limite máximo de caracteres a serem recebidos                                (qtd de rastreadores * tamanho da mensagem enviada (49 char))
  byte dados2[numChars2];                               //Array de bytes que irá armazenar os dados fornecidos pelos rastreadores
  static int ndx2 = 0;                                  //Indice que conta quantos caracteres foram recebidos

  while (pausa > millis()){
     if (Serial1.available() > 0){
      dados1[ndx1] = Serial1.read();
      if (dados1[ndx1] != 'B'){
        ndx1++;
        if (ndx1 >= numChars1) {                        //Caso a quantidade de dados recebida atinja o limite
           ndx1 = numChars1 - 1;
           dados1[ndx1] = '\0';
        }
      }      
    }
    if (Serial2.available() > 0){
      dados2[ndx2] = Serial2.read();
      if (dados2[ndx2] != 'B'){
        ndx2++;
        if (ndx2 >= numChars2) {                        //Caso a quantidade de dados recebida atinja o limite
           ndx2 = numChars2 - 1;
           dados2[ndx2] = '\0';
        }
      }      
    }
  }
  if (ndx1 > 0) {                     //Caso ndx1 seja maior que 0 indica que dados foram recebidos
    
    dados1[ndx1] = '\0';              //Atribui o caracter "NULL" e com isso encerra o recebimento
    mensagem1 = (char*)dados1;        //Atribui os dados a variavel global mensagem para ser utilizada na função publicar()
    qtdCharRcb1 = ndx1;               //Atribui a quantidade de caracteres recebidos para uso na função publicar
    ndx1 = 0;                         //Define o indice novamente para 0
    newData1 = true;                  //Define newData como true indicando que há novos dados recebidos
  } 
  if (ndx2 > 0) {                     //Caso ndx2 seja maior que 0 indica que dados foram recebidos
    
    dados2[ndx2] = '\0';              //Atribui o caracter "NULL" e com isso encerra o recebimento
    mensagem2 = (char*)dados2;        //Atribui os dados a variavel global mensagem para ser utilizada na função publicar()
    qtdCharRcb2 = ndx2;               //Atribui a quantidade de caracteres recebidos para uso na função publicar
    ndx2 = 0;                         //Define o indice novamente para 0
    newData2 = true;                  //Define newData como true indicando que há novos dados recebidos
  }

  if (newData1 == true || newData2 == true){      //Caso dados tenham sido recebidos por algum dos modulos, aciona a função publicar()
  publicar();
  return; 
  }
 }    //Fim Receber

/*
 * Função solicitar
 * Envia a Id de solicitação aos módulos transmissores do canal em que está operando. 
 * Antes de solicitar verifica o status da conexão com o servidor MQTT e o status do cartão SD
 * Após a solicitação chama a função receber(), responsável por receber e armazenar os dados recebidos, se houverem
 */

void solicitar(){                                                         //Função que solicita dados dos rastreadores
     
    conectar_MQTT();                                                      //A cada iteração do loop verifica se a conexão com o MQTT está ativa
    digitalWrite(MQTT_LED, LOW);
    iniciar_SD();                                                         //A cada iteração do loop verifica se o módulo SD está ativo
    digitalWrite(SD_LED, LOW);
   
    byte solicitacao[] = {0xFF, 0xFF, canalModulo, idSolicitacao};       //Solicita dados a todos os módulos (0xFF 0xFF) operando no canal (canalModulo) que estejam no grupo chamado
    Serial1.write(solicitacao,sizeof(solicitacao));                      //Módulo LORA na serial1 envia o char de solicitação de dados
    receber();                                                           //Função que recebe dados dos rastreadores

} //Fim Solicitar()

/*
 * Função Publicar
 * Verifica se novos dados foram recebidos, caso tenham sido recebidos, publica no servidor mqtt e salva como log no cartão sd
 * O endereço do tópico onde será feita a publicação é definido na variável topico_str, no inicio do sketch
 * Ao fim da publicação reinicia as seriais física e lógica a fim de limpar o buffer de armazenamento de cada uma.
 */

void publicar(){                                                 //Função que publica os dados recebidos no MQTT e salva no cartão SD

  if (newData1 == true) {                                        //Caso tenha recebido dados
     
    newData1 = false;                                            //Define dados recebidos como falso
    char topico_char[((topico_str.length())+1)];                 //Vetor que irá armazenar o endereço do tópico em formato char
    topico_str.toCharArray(topico_char, sizeof(topico_char));    //Converte o endereço criado em char e atribui ao vetor topico_char
                   
    Serial.println(mensagem1);                                   //Exibe os dados recebidos na Serial
    salvar_SD(mensagem1);                                        //Salva os dados recebidos no SD
                                                    
    client.beginPublish(topico_char,qtdCharRcb1,false);          //Inicia a publicação no MQTT      **Parâmetros - topico_char(endereço do topico MQTT) / qtdCharRcb(quantos caracteres serão publicados) / false(Não alterar)**                 
    client.print(mensagem1);                                     //Publica a mensagem
    client.endPublish();                                         //Encerra a publicação no MQTT

    digitalWrite(MQTT_LED,  HIGH);
    delay(150);
    digitalWrite(MQTT_LED,  LOW);
  }
  if (newData2 == true) {                                        //Caso tenha recebido dados
     
    newData2 = false;                                            //Define dados recebidos como falso
    char topico_char[((topico_str.length())+1)];                 //Vetor que irá armazenar o endereço do tópico em formato char
    topico_str.toCharArray(topico_char, sizeof(topico_char));    //Converte o endereço criado em char e atribui ao vetor topico_char
                   
    Serial.println(mensagem2);                                   //Exibe os dados recebidos na Serial
    salvar_SD(mensagem2);                                        //Salva os dados recebidos no SD
                                                    
    client.beginPublish(topico_char,qtdCharRcb2,false);          //Inicia a publicação no MQTT      **Parâmetros - topico_char(endereço do topico MQTT) / qtdCharRcb(quantos caracteres serão publicados) / false(Não alterar)**                 
    client.print(mensagem2);                                     //Publica a mensagem
    client.endPublish();                                         //Encerra a publicação no MQTT

    digitalWrite(MQTT_LED,  HIGH);
    delay(150);
    digitalWrite(MQTT_LED,  LOW);
  }
    Serial.end();                                                //Encerra a serial para limpar o buffer de recebimento
    Serial1.end();
    Serial2.end();
    delay(500);
    Serial.begin(9600);                                          //Reinicia a Serial
    Serial1.begin(9600);
    Serial2.begin(9600);
}// Fim Publicar()

void setup(){

  Serial.begin(9600);                                            //Inicia a Serial
  Serial1.begin(9600);
  Serial2.begin(9600);
 
  pinMode(MQTT_LED,   OUTPUT);                 
  pinMode(SD_LED,     OUTPUT);
  pinMode(PWR_LED,    OUTPUT);
  pinMode(LORA1_LED,  OUTPUT);
  pinMode(LORA2_LED,  OUTPUT);
  
  digitalWrite(PWR_LED,   HIGH);

  Serial.println("Iniciando...");
  
  iniciarLORA1();                                  //Inicia o módulo LORA 1 com os parâmetros definidos na função iniciar_LORA1
  iniciarLORA2();                                  //Inicia o módulo LORA 2 com os parâmetros definidos na função iniciar_LORA2
  delay(500);

  iniciar_SD();                                    //Inicia o módulo SD
  Serial.println("SD OK"); 
  digitalWrite(SD_LED,  LOW);
  Ethernet.init(10);
  Ethernet.begin(mac, ip);                         //Inicia o Ethernet shield
  client.setServer(server, 1883);                  //Inicia e define IP e porta do servidor MQTT
  conectar_MQTT();                                 //Conecta ao broker MQTT
  Serial.println("REDE/MQTT OK");
  digitalWrite(MQTT_LED,  LOW);
} //Fim Setup()

void loop() {
  
  solicitar();  //Solicita dados aos módulos transmissores
}   //Fim loop
