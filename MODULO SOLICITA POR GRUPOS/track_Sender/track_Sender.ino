/*

                          **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                              ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                          
                   SoftSerial (Saídas Digitais)      Objeto EBYTE (Saídas Digitais)    
                   ** 12,11 -  RX,TX  (GPS)           ** 7, 8, 10 - M0, M1, AUX(Definido mas não conectado)          
                   ** 9,6 -  RX,TX (LORA)                                                
  
 */
#include <TinyGPS.h>
#include <EBYTE.h>
#include <SoftwareSerial.h>

#define AMOSTRAS 15             //Usado na função lePorta. Numero de vezes que a função irá ler o valor na porta A2, para tirar a média dos valores, para calculo de tensão na bateria.

//String id_grupo = "A";          //Define que este módulo pertence ao grupo A
String id_grupo = "B";        //Define que este módulo pertence ao grupo B
String id_modulo = "03";        //Define o id do módulo emissor (ou id da equipe) ---- Definir um Id diferente para cada módulo
String tensaoBat;               //Usado na função tensaoBat. Variável que armazena o valor da tensão da bateria
String id_grupo_str;            //Usado na função receber. Armazena o id da equipe recebido pelo módulo e enviado pelo receptor em terra convertido em String
char id_grupo_char[2];          //Usado na função receber. Array de caracteres que irá armazenar a mensagem de requisição recebida em char

size_t bytesRecebidos;

SoftwareSerial serialLORA(9,6);         //Rx - Tx (LORA)
SoftwareSerial serialGPS(11,12);        //Rx - Tx (GPS)

EBYTE emissor(&serialLORA, 7, 8, 10);   //Parâmetros do módulo LORA (RX,TX,M0,M1,AUX)
TinyGPS gps;                            //Objeto TinyGPS

void iniciarLORA () 
  {
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
    // emissor.SetTransmitPower(OPT_TP20);                        //Força de transmissão 20db (Para módulos 100mW/3km)
     emissor.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor.SetTransmissionMode(OPT_FMDISABLE);                //Transmission Mode (Fixed or Transparent)
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
  return total / (float)AMOSTRAS;                               //retorna o valor médio obtido das 12 leituras na porta A2
}

void tensaoBateria() {                                          //Função que calcula o valor de tensão da bateria, de acordo com o valor lido na porta A2

float tensaoA2;
float aRef=5;                                                   //Máximo valor de tensão aceito pela porta A2
float relacaoA2=2.7;                                          //Power Bank Fator para transformar o valor Vout recebido na porta A2, no valor de Vin (que entra antes do divisor de tensão) 
//float relacaoA2=2.8;                                          //Bateria 9V
  tensaoA2 = ((lePorta(A2) * aRef) / 1024.0)* relacaoA2;        //Calculo de tensão

  if (tensaoA2 <= 9.9){
     tensaoBat = "0"+String(tensaoA2,1);                     //Adiciona zero a esquerda para tensões menores que 10, mantendo o valor padrão de 3 char(TT.T)
     } if (tensaoA2 >= 10){
      tensaoBat = String(tensaoA2,1);                           //Para horas maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
     }  
}  

void getGPS(){                                                  //Função que obtém os dados de GPS
 
   serialGPS.listen();

   bool recebido = false;
   unsigned long idadeInfo,date,tempo, pausa;
   float flat, flon;
   String diaMes, horaMinSeg, dados, velocidade;

  pausa = millis() + 4000;
 
     while (serialGPS.available()) {                             //Recebe os dados pela serial do GPS
     char cIn = serialGPS.read();
     recebido = gps.encode(cIn);
   }
  
  if (recebido) {
     gps.get_datetime(&date, &tempo, &idadeInfo);             //Obtem data e hora dos dados recebidos anteriormente
     gps.f_get_position(&flat, &flon, &idadeInfo);            //Obtem latitude e longitude dos dados recebidos anteriormente

     //tempo = ((tempo/10000)-300);                           //Converte o formato da hora de HHMMSSMS para HHMM
     tempo = ((tempo/100)-30000);                               //Converte o formato da hora de HHMMSSMS para HHMMSS
     date = (date/100);                                       //Converte o formato da data de DDMMAA para DDMM

    if (gps.f_speed_knots() <= 9.9){
        velocidade = "0"+String(gps.f_speed_knots(),1);       //Adiciona zero a esquerda para velocidades menores que 10, mantendo o valor padrão de 3 char(VV.V)
     } if (gps.f_speed_knots() >= 10){
        velocidade = String(gps.f_speed_knots(),1);           //Para velocidades maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
     }

     if (tempo <= 99999){
        horaMinSeg = "0"+String(tempo);                       //Adiciona zero a esquerda para horas menores que 10, mantendo o numero padrão de 4 char(HHMM)
     } if (tempo >= 100000){
        horaMinSeg = String(tempo);                           //Para horas maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
     }

     if (date <= 999){              
      diaMes = "0"+String(date);                              //Adiciona zero a esquerda para dias menores que 10, mantendo o numero padrão de 4 char (DDMM)
     } else{
      diaMes = String(date);                                  //Para dias maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
     }

     tensaoBateria();                                         //Chama a função que irá obter a tensão da bateria

     dados = "["+id_modulo+","+String(flat,6)+","+String(flon,6)+","+diaMes+","+horaMinSeg+","+velocidade+","+tensaoBat+"]*";        //Forma a string de dados que será enviada
          
          Serial.println(dados);                                  //Exibe os dados (apenas para fins de teste

          while (pausa > millis()){                               //deixa ativa a função receber por 3seg para aguardar, caso o receptor solicite dados
          receber();
          if (id_grupo_str == id_grupo){                         //caso a ID recebida seja igual a ID do grupo ao qual o modulo pertence, envia a string com os dados fornecidos pelo GPS para o receptor 
              serialLORA.listen();    
              serialLORA.print(dados);
          break;
          }
         }
          
          //serialLORA.listen();    
          //serialLORA.print(dados);                                //Transmite os dados ao receptor em terra
          //delay(1000);
        }
    }    

void receber(){                                                                 //Função responsavel por receber as mensagens de solicitação de dados do módulo receptor
   serialLORA.listen(); 
    
   for (byte a = 0; a < 1; a++ ){                                               //Limpa a variavel id_equipe_char, antes do uso
     id_grupo_char[a] = '0';
     }
     while (serialLORA.available() > 0){                                        //Se houver dados a receber pela serial lógica
       bytesRecebidos = serialLORA.readBytesUntil('*', id_grupo_char, 2);      //Recebe o caracter e o atribui a variavel id_equipe_char
     } 
      id_grupo_str = String(id_grupo_char[0]);                                //cria uma string com os valores contidos em mensagem[1]
   }
  
void dadosFake() {                      //Função que gera dados FAKE (apenas para testes temporariamente)

   tensaoBateria();                     //Chama a função que irá obter a tensão da bateria
   
   String dados = "["+id_modulo+",-26.242370,-48.642423,2806,182100,00.1,"+tensaoBat+"]*";
          
   Serial.println(dados);
                 
          serialLORA.listen();
          serialLORA.print(dados);
          delay(1500);
 }

void setup() {
  
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
   getGPS();                         //Função que obtem os dados do GPS
   //dadosFake();                        //Função que obtem os dados FAKES
   //delay(3000);
 }
