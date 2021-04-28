/*
                                                                 ***********EMISSOR*********
                          **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" OU 100mW/3km "EBYTE E32 433T20D"**************
                              ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                          
                   SoftSerial (Saídas Digitais)      Objeto EBYTE (Saídas Digitais)                                                   LEDS Indicativos
                   ** 12,11 -  RX,TX  (GPS)           ** 7, 8, 10 - M0, M1, AUX (Pino AUX definido mas não conectado)                 ** Status Lora - A1
                   ** 9,6 -    RX,TX (LORA)                                                                                           ** Status Transmissão - A2
                                                                                                                                      ** Status PWR - A3
 */

#include <TinyGPS.h>
#include <EBYTE.h>
#include <SoftwareSerial.h>

#define AMOSTRAS 15                               //Usado na função lePorta. Numero de vezes que a função irá ler o valor na porta A7, para tirar a média dos valores, para calculo de tensão na bateria.

#define LORA_LED A1                               //LED de status LORA
#define PWR_LED  A3                               //LED de status POWER
#define TRANSMISSION_LED  A2                      //LED de status TRANSMISSÃO

//CONFIGURAR ESTES VALORES DE ACORDO COM NECESSÁRIO PARA FUNCIONAMENTO
char   idGrupo = 'B';                             //Define a qual grupo este módulo pertence
char   infoReceptor1 [] = {0, 31, 5};             //Define o endereço e o canal do módulo LORA receptor 1 - addressH, addressL, channel / addressH e addressL se definidos ambos como 255, envia mensagem para todos módulos do canal simultaneamente(broadcast)
char   infoReceptor2 [] = {0, 32, 5};             //Define o endereço e o canal do módulo LORA receptor 2 - addressH, addressL, channel / addressH e addressL se definidos ambos como 255, envia mensagem para todos módulos do canal simultaneamente(broadcast)
String idModulo = "02";                           //Define o id deste módulo emissor. Define o id do módulo emissor contido na mensagem enviada para o receptor em terra - Definir um Id diferente para cada módulo sempre no formato de dois dígitos Ex.: 01,02,10
byte   canalModulo = 5;                           //Define o canal em que este módulo irá operar. Deve ser o mesmo canal que o receptor
int    sincEnvio = (idModulo.toInt())*700;        //Define o tempo de envio de cada módulo, afim de sincronizar os envios e evitar o envio de dois ou mais módulos simultaneamente. idModulo*500ms (500ms é o tempo de envio entre cada módulo).
//FIM COFIGURAÇÕES

byte   dados2 [49];                               //Armazena os valores em bytes da String dados, que contém a informação do GPS

String tensaoBat;                                 //Usado na função tensaoBat. Variável que armazena o valor da tensão da bateria

SoftwareSerial serialLORA(9,6);            //Rx - Tx (LORA)
SoftwareSerial serialGPS(11,12);           //Rx - Tx (GPS)

EBYTE emissor(&serialLORA, 7, 8, 10);      //Parâmetros do módulo LORA (RX,TX,M0,M1,AUX)
TinyGPS gps;                               //Objeto TinyGPS

/*
 * Função iniciarLORA
 * Executada uma vez dentro da função Setup, é responsável por definir e passar todos os parâmetros de funcionamento do módulo Lora em uso 
 * Após ser executada exibe no monitor serial os parametros que foram definidos no módulo.
 * Analise sempre essa informação, pois é um indicativo importante se o módulo está corretamente configurado ou não.
 * Erros no sketch ou mesmo pinos conectados incorretamente podem atrapalhar o funcionamento correto do módulo LORA.
 * Caso isso ocorra revise o sketch e as conexões do módulo.
 * Para mais informações de configuração e funcionamento do módulo consulte o Usermanual do mesmo.
 */

void iniciarLORA (){
     int addressL = idModulo.toInt();                          //Utiliza o valor definido na String idModulo para atribuir o endereçoL do modulo nas configurações da placa LORA (função iniciarLORA)
    
     serialLORA.listen();                   
       
     emissor.init();                                            //Inicia o módulo

     emissor.SetMode(MODE_NORMAL);                              //Modo de funcionamento do módulo
     emissor.SetAddressH(0);                                    //Endereço H. Pode Variar entre 0 e 254 (Preferível não alterar, a menos que a quantidade de módulos em funcionamento ultrapasse 255)
     emissor.SetAddressL(addressL);                             //Endereço L. Pode Variar entre 0 e 254 (A fins de facilitar, é automaticamente definido de acordo com o valor contido na variavel idModulo)
     emissor.SetAirDataRate(ADR_2400);                          //AirDataRate 2400kbps
     emissor.SetUARTBaudRate(UDR_9600);                         //BAUDRate 9600
     emissor.SetChannel(canalModulo);                           //Canal. Pode variar de 0 até 32
     emissor.SetParityBit(PB_8N1);                              //Bit Paridade 8N1
     //emissor.SetTransmitPower(OPT_TP30);                      //Força de transmissão 30db (Para módulos de 1W/8km)
     emissor.SetTransmitPower(OPT_TP20);                        //Força de transmissão 20db (Para módulos 100mW/3km)
     emissor.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor.SetTransmissionMode(OPT_FMENABLE);                 //Transmission Mode (Fixed or Transparent)
     emissor.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor.PrintParameters();                                 //Exibe os parâmetros configurados

//Verifica se o módulo LoRa iniciou com os parâmetros corretos para o funcionamento
     if (emissor.GetModel() == 50 && emissor.GetFeatures() == 20){
      Serial.println("Lora OK");      
    }else {
      digitalWrite(LORA_LED,  HIGH);
      falhaLORA();
     }
  } //Fim iniciarLORA

/*
 *Função falhaLORA
 *Caso algum módulo LORA não inicie corretamente, escreve mensagem de erro na serial e acende LED indicativo.
 */

void falhaLORA(){
  bool falha = true;
  Serial.println("Erro módulo LORA");
  while (falha == true){    
    delay(10000);
  }
}

/*
 * Função lePorta
 * Lê a tensão que entra na porta A7
 * Executa um número de leituras definido na variável AMOSTRAS, e calcula a média dessas leituras para definir o valor aproximado da tensão na porta
 * Retorna um valor float
 * Necessita como parâmetro o endereço da porta onde deve ser feita a leitura
 * Como as portas analógicas do arduino não suportam tensões acima de 5v, a tensão aplicada na porta deve ser reduzida por meio de divisores de tensão
 */

float lePorta(uint8_t portaAnalogica) {                         //Função que lê o valor de tensão recebido na porta A7 
  float total=0;  
  for (int i=0; i<AMOSTRAS; i++) {
    total += 1.0 * analogRead(portaAnalogica);
    delay(5);
  }
  return total / (float)AMOSTRAS;                               //retorna o valor médio obtido das 15 leituras na porta A7
} //Fim lePorta

/*
 * Função tensãoBateria
 * Calcula o valor real da tensão da bateria ou fonte de alimentação utilizada para alimentar o módulo
 * O cálculo é feito utilizando a tensão lida na porta A7 multiplicado por um fator de correção definido na variável relaçãoA7
 * Por fim atribui o resultado a variável global tensaoA7
 */

void tensaoBateria() {                                          //Função que calcula o valor de tensão da bateria, de acordo com o valor lido na porta A7

//float tensaoA2;
float tensaoA7;
float aRef=5;                                                   //Máximo valor de tensão aceito pela porta A7
//float relacaoA2=2.5;
float relacaoA7=2.5;                                            //Power Bank - Fator para transformar o valor Vout recebido na porta A7, no valor de Vin (que entra antes do divisor de tensão) 

  //tensaoA2 = ((lePorta(A2) * aRef) / 1024.0)* relacaoA2;
  tensaoA7 = ((lePorta(A7) * aRef) / 1024.0)* relacaoA7;        //Calculo de tensão
/*
  if (tensaoA2 <= 9.9){
    tensaoBat = "0"+String(tensaoA2,1);                         //Adiciona zero a esquerda para tensões menores que 10, mantendo o valor padrão de 3 char(TT.T)
  } 
  if (tensaoA2 >= 10){
    tensaoBat = String(tensaoA2,1);                             //Para horas maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
  }  */
  
  if (tensaoA7 <= 9.9){
    tensaoBat = "0"+String(tensaoA7,1);                         //Adiciona zero a esquerda para tensões menores que 10, mantendo o valor padrão de 3 char(TT.T)
  } 
  if (tensaoA7 >= 10){
    tensaoBat = String(tensaoA7,1);                             //Para horas maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
  } 
} //Fim tensaoBateria  

/*
 * Função getGPS
 * Obtém os dados do gps e juntamente com o valor obtido da função tensaoBateria e forma a mensagem que será enviada ao módulo em terra
 * Executa algumas verificações afim de manter a mensagem final com um padrão fixo de 49 caracteres(incluso o caracter null de fim de string /0)
 * Transforma a mensagem de string para um vetor de char
 * Após a mensagem pronta, chama a função receber, que irá aguardar pela solicitação dos dados
 */

void getGPS(){                                                  //Função que obtém os dados de GPS
 
serialGPS.listen();

bool recebido = false;
unsigned long idadeInfo,date,tempo;
float flat, flon;
String diaMes, horaMinSeg, dados, velocidade;

  do {                                                      //Fica em loop enquanto o modulo GPS não se conectar e receber valores
    while (serialGPS.available()) {                         
      recebido = gps.encode(serialGPS.read());
    }
  } while (!recebido);                                      //Caso o GPS conecte e tenha dados, prossegue

  if (recebido) {
    gps.get_datetime(&date, &tempo, &idadeInfo);            //Obtem data e hora dos dados recebidos anteriormente
    gps.f_get_position(&flat, &flon, &idadeInfo);           //Obtem latitude e longitude dos dados recebidos anteriormente

    if (tempo <= 3595999){                                  //Converte o formato da hora de HHMMSSMS para HHMMSS e configura o fuso horário para GMT -3
      tempo = ((tempo/100)+210000);                         
    }else if(tempo >= 4000000){
      tempo = ((tempo/100)-30000);                          //Converte o formato da hora de HHMMSSMS para HHMMSS e configura o fuso horário para GMT -3
    }
    date = (date/100);                                      //Converte o formato da data de DDMMAA para DDMM

    if (gps.f_speed_knots() <= 9.9){
      velocidade = "0"+String(gps.f_speed_knots(),1);       //Adiciona zero a esquerda para velocidades menores que 10, mantendo o valor padrão de 3 char(VV.V)
    } 
    if (gps.f_speed_knots() >= 10){
      velocidade = String(gps.f_speed_knots(),1);           //Para velocidades maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
    }

    if (tempo <= 99999){
      horaMinSeg = "0"+String(tempo);                       //Adiciona zero a esquerda para horas menores que 10, mantendo o numero padrão de 4 char(HHMMSS)
    } 
    if (tempo >= 100000){
      horaMinSeg = String(tempo);                           //Para horas maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
    }

    if (date <= 999){              
      diaMes = "0"+String(date);                            //Adiciona zero a esquerda para dias menores que 10, mantendo o numero padrão de 4 char (DDMM)
    } else {
        diaMes = String(date);                              //Para dias maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
      }

  tensaoBateria();                                          //Chama a função que irá obter a tensão da bateria

  dados = "["+idModulo+","+String(flat,6)+","+String(flon,6)+","+diaMes+","+horaMinSeg+","+velocidade+","+tensaoBat+"]";        //Forma a string de dados que será enviada
  dados.getBytes(dados2, sizeof(dados2));                   //Converte a String em vetor de bytes
}

    //Serial.println(dados);                                  //Exibe os dados (apenas para fins de teste)
    receber();
  } //Fim getGPS       
 
 /*         
  * Função receber          
  * Fica em loop até receber o Id de solicitação de dados emitido pelo módulo em terra
  * Ao receber o Id chama a função transmitir que irá enviar os dados do gps
  */
  
void receber(){
serialLORA.listen();    
 
byte incomingByte;                                          //Armazena o char que indica quando deve se enviar os dados do GPS
  
  do {                                                      //Se mantém em loop enquanto não receber a Id de solicitação do receptor em terra
    if (serialLORA.available() > 0){
      incomingByte = serialLORA.read();
    }
  } while (incomingByte != idGrupo);
  
  if (incomingByte == idGrupo){                             //Caso receba o char de solicitação (o mesmo que foi definido na variavel idGrupo), chama a função transmitir para enviar os dados
       transmitir(); 
  }
} //Fim receber

/*
 * Função transmitir
 * Envia a mensagem com os dados do gps em formato vetor de char
 * Aguarda por um tempo determinado antes de enviar
 * Após o envio reinicia a serial física, a serial do lora e a serial do gps afim de limpar o buffer de dados
 */
  
void transmitir(){
serialLORA.listen();

char enderecoReceptorL;
char enderecoReceptorH;
char canalReceptor; 

//Define automaticamente para qual dos dois módulos LORA receptores irá ser enviado o pacote de dados com as corrdenas do GPS

if (idModulo.toInt() == 0){                      //Caso o Id do módulo seja 0
  enderecoReceptorH = infoReceptor1[0];
  enderecoReceptorL = infoReceptor1[1];
  canalReceptor     = infoReceptor1[2];
  } else if ((idModulo.toInt() % 2) != 0){       //Caso o Id do módulo seja impar
      enderecoReceptorH = infoReceptor2[0];
      enderecoReceptorL = infoReceptor2[1];
      canalReceptor     = infoReceptor2[2];
    } else if ((idModulo.toInt() % 2) == 0){     //Caso o Id do módulo seja par
        enderecoReceptorH = infoReceptor1[0];
        enderecoReceptorL = infoReceptor1[1];
        canalReceptor     = infoReceptor1[2];
      }

byte mensagem [] = {
  enderecoReceptorH, enderecoReceptorL, canalReceptor,     //Endereço e canal do módulo receptor
  dados2[0],dados2[1],dados2[2],dados2[3],dados2[4],dados2[5],dados2[6],dados2[7],dados2[8],dados2[9],
  dados2[10],dados2[11],dados2[12],dados2[13],dados2[14],dados2[15],dados2[16],dados2[17],dados2[18],dados2[19],
  dados2[20],dados2[21],dados2[22],dados2[23],dados2[24],dados2[25],dados2[26],dados2[27],dados2[28],dados2[29],
  dados2[30],dados2[31],dados2[32],dados2[33],dados2[34],dados2[35],dados2[36],dados2[37],dados2[38],dados2[39],
  dados2[40],dados2[41],dados2[42],dados2[43],dados2[44],dados2[45],dados2[46],dados2[47]
  };   

 if (idModulo.toInt() == 0){                                  //Caso o id do módulo seja 0 envia a mensagem sem delay
    serialLORA.write(mensagem, sizeof(mensagem));             //envia a msg pelo LORA 
} else if (idModulo.toInt() != 0){
  delay(sincEnvio);                                          //Caso o Id seja diferente de 0, aguarda o tempo determinado para o envio                        
  serialLORA.write(mensagem, sizeof(mensagem));              //envia a msg pelo LORA
}
//Pisca o Led de transmissão
  digitalWrite(TRANSMISSION_LED,  HIGH);
  delay(200);
  digitalWrite(TRANSMISSION_LED,  LOW);

  serialGPS.end();                                           //Encerra a serialGPS para limpar o buffer de recebimento
  Serial.end();                                              //Encerra a serial para limpar o buffer de recebimento
  serialLORA.end();                                          //Encerra a softSerial para limpar o buffer de recebimento
  delay(500);
  Serial.begin(9600);                                        //Reinicia a Serial
  serialLORA.begin(9600);                                    //Reinicia a softSerial
  serialGPS.begin(9600);                                     //Reinicia a serialGPS
} //Fim transmitir

/*
 * Função dadosFake
 * Simula uma mensagem a ser enviada
 * Apenas para testes em que não seja necessário o uso dos dados reais do gps
 */

void dadosFake() {                                            //Função que gera dados FAKE (apenas para testes temporariamente)

    String dados = "[02,-26.242370,-48.642423,2806,182100,00.2,00.1]";
    dados.getBytes(dados2, sizeof(dados2));
    receber();
}

void setup() {

pinMode(PWR_LED,  OUTPUT);                 
pinMode(TRANSMISSION_LED,  OUTPUT);

digitalWrite(PWR_LED,  HIGH);
  
Serial.begin(9600);                     //Inicia a serial física
serialLORA.begin(9600);                 //Inicia a serial LORA
serialGPS.begin(9600);                  //Inicia a serial GPS
Serial.println("Iniciando...");
delay(500);

iniciarLORA();                          //Função que passa os parâmetros de funcionamento do módulo LORA
}
 
void loop() {             

getGPS();                             //Envia dados do modulo GPS
//dadosFake();                            //Envia dados simulados para fins de teste
}
