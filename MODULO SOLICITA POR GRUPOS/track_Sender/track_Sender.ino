/*
                                                                 ***********EMISSOR*********
                          **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                              ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                          
                   SoftSerial (Saídas Digitais)      Objeto EBYTE (Saídas Digitais)    
                   ** 12,11 -  RX,TX  (GPS)           ** 7, 8, 10 - M0, M1, AUX (Pino AUX definido mas não conectado)          
                   ** 9,6 -    RX,TX (LORA)                                                
  
 */

#include <TinyGPS.h>
#include <EBYTE.h>
#include <SoftwareSerial.h>

#define AMOSTRAS 15                               //Usado na função lePorta. Numero de vezes que a função irá ler o valor na porta A2, para tirar a média dos valores, para calculo de tensão na bateria.

#define LED8  A0                                  //LED de status POWER
#define LED9  A1                                  //LED de status GPS

char   idGrupo = 'B';                             //Define a qual grupo este módulo pertence
char   infoReceptor [] = {0, 30, 5};              //Defiine o endereço e o canal do módulo receptor - addressH, addressL, channel / addressH e addressL se definidos ambos como 255, envia mensagem para todos módulos do canal simultaneamente(broadcast)
String idModulo = "01";                           //Define o id deste módulo em0issor. Define o id do módulo emissor contido na mensagem enviada para o receptor em terra - Definir um Id diferente para cada módulo sempre no formato de dois dígitos Ex.: 01,02,10
byte   canalModulo = 5;                           //Define o canal em que este módulo irá operar. Deve ser o mesmo canal que o receptor

byte   dados2 [49];                               //Armazena os valores em bytes da String dados, que contém a informação do GPS

String tensaoBat;                                 //Usado na função tensaoBat. Variável que armazena o valor da tensão da bateria

SoftwareSerial serialLORA(9,6);            //Rx - Tx (LORA)
SoftwareSerial serialGPS(11,12);           //Rx - Tx (GPS)

EBYTE emissor(&serialLORA, 7, 8, 10);      //Parâmetros do módulo LORA (RX,TX,M0,M1,AUX)
TinyGPS gps;                               //Objeto TinyGPS
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
  } //Fim iniciarLORA  

float lePorta(uint8_t portaAnalogica) {                         //Função que lê o valor de tensão recebido na porta A2 
  float total=0;  
  for (int i=0; i<AMOSTRAS; i++) {
    total += 1.0 * analogRead(portaAnalogica);
    delay(5);
  }
  return total / (float)AMOSTRAS;                               //retorna o valor médio obtido das 15 leituras na porta A2
} //Fim lePorta

void tensaoBateria() {                                          //Função que calcula o valor de tensão da bateria, de acordo com o valor lido na porta A2

float tensaoA2;
float aRef=5;                                                   //Máximo valor de tensão aceito pela porta A2
float relacaoA2=2.5;                                            //Power Bank Fator para transformar o valor Vout recebido na porta A2, no valor de Vin (que entra antes do divisor de tensão) 
//float relacaoA2=2.8;                                          //Bateria 9V
  
  tensaoA2 = ((lePorta(A2) * aRef) / 1024.0)* relacaoA2;        //Calculo de tensão
  
  if (tensaoA2 <= 9.9){
    tensaoBat = "0"+String(tensaoA2,1);                         //Adiciona zero a esquerda para tensões menores que 10, mantendo o valor padrão de 3 char(TT.T)
  } 
  if (tensaoA2 >= 10){
    tensaoBat = String(tensaoA2,1);                             //Para horas maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
  }  
} //Fim tensaoBateria  

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

    Serial.println(dados);                                    //Exibe os dados (apenas para fins de teste)
    receber();
  } //Fim getGPS       
          
void receber(){
serialLORA.listen();    
 
byte incomingByte;                                          //Armazena o char que indica quando deve se enviar os dados do GPS
  
  do {                                                      //Se mantém em loop enquanto não receber solicitação do receptor em terra
    if (serialLORA.available() > 0){
      incomingByte = serialLORA.read();
    }
  } while (incomingByte != idGrupo);
  
  if (incomingByte == idGrupo){                             //Caso receba o char de solicitação (o mesmo que foi definido na variavel idGrupo), chama a função transmitir para enviar os dados
       transmitir(); 
  }
} //Fim receber
  
void transmitir(){
serialLORA.listen();

int tempoEnvio = (idModulo.toInt())*1200;    //Define o tempo de envio de cada módulo, afim de sincronizar os envios e evitar o envio de dois ou mais módulos simultaneamente

byte mensagem [] = {
  infoReceptor[0], infoReceptor [1], infoReceptor [2],     //Endereço e canal do módulo receptor
  dados2[0],dados2[1],dados2[2],dados2[3],dados2[4],dados2[5],dados2[6],dados2[7],dados2[8],dados2[9],
  dados2[10],dados2[11],dados2[12],dados2[13],dados2[14],dados2[15],dados2[16],dados2[17],dados2[18],dados2[19],
  dados2[20],dados2[21],dados2[22],dados2[23],dados2[24],dados2[25],dados2[26],dados2[27],dados2[28],dados2[29],
  dados2[30],dados2[31],dados2[32],dados2[33],dados2[34],dados2[35],dados2[36],dados2[37],dados2[38],dados2[39],
  dados2[40],dados2[41],dados2[42],dados2[43],dados2[44],dados2[45],dados2[46],dados2[47]
  };   

  delay(tempoEnvio);                                         //Aguarda o tempo determinado para o envio                        
  serialLORA.write(mensagem, sizeof(mensagem));              //envia a msg pelo LORA
  
  serialGPS.end();
  Serial.end();                                              //Encerra a serial para limpar o buffer de recebimento
  serialLORA.end();                                          //Encerra a softSerial para limpar o buffer de recebimento
  delay(500);
  Serial.begin(9600);                                        //Reinicia a Serial
  serialLORA.begin(9600);                                    //Reinicia a softSerial
  serialGPS.begin(9600);
} //Fim transmitir

void dadosFake() {                      //Função que gera dados FAKE (apenas para testes temporariamente)

  String dados = "["+idModulo+",-26.242370,-48.642423,2806,182100,00.2,00.1]"; 
  dados.getBytes(dados2, sizeof(dados2));
  //Serial.println(dados);                                    //Exibe os dados (apenas para fins de teste
  receber();
}

void setup() {

pinMode(LED8,  OUTPUT);                 
pinMode(LED9,  OUTPUT);

digitalWrite(LED8,  HIGH);
//digitalWrite(LED9,  HIGH);
  
Serial.begin(9600);                     //Inicia a serial física
Serial.println("Serial F. OK");
serialLORA.begin(9600);                 //Inicia a serial LORA
Serial.println("Serial L. OK");
serialGPS.begin(9600);                  //Inicia a serial GPS
Serial.println("Serial G. OK");
delay(500);

iniciarLORA();                          //Função que passa os parâmetros de funcionamento do módulo LORA
}
 
void loop() {             

//getGPS();                             //Obtém dados do modulo GPS
dadosFake();                        //Obtém dados simulados para fins de teste
}
