/*

                          **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                              ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                          
                   SoftSerial (Saídas Digitais)      Objeto EBYTE (Saídas Digitais)    
                   ** 2,3 -  RX,TX  (GPS)           ** 7, 8, 10 - M0, M1, AUX(Definido mas não conectado)          
                   ** 9,6 -  RX,TX (LORA)                                                
  
 */
#include <TinyGPS.h>
#include <EBYTE.h>
#include <SoftwareSerial.h>

#define AMOSTRAS 12             //Usado na função lePorta. Numero de vezes que a função irá ler o valor na porta A2, para tirar a média dos valores, para calculo de tensão na bateria.
//String dados;
float tensaoA2;                 //Usado na função tensaoBat. Variável que armazena o valor da tensão da bateria
String id_equipe;               //Usado na função receber. Armazena o id da equipe recebido pelo módulo
char mensagem[3];               //Usado na função receber. Array de caracteres que irá armazenar a mensagem de requisição recebida
String mensagemrecebida;
size_t bytesRecebidos;

SoftwareSerial serialLORA(9,6);         //Rx - Tx (LORA)
SoftwareSerial serialGPS(2,3);          //Rx - Tx (GPS)

EBYTE emissor(&serialLORA, 7, 8, 10);   //Parâmetros do módulo LORA (RX,TX,M0,M1,AUX)
TinyGPS gps;                            //Objeto TinyGPS

void setup() {
  
Serial.begin(9600);                     //Inicia a serial física
serialLORA.begin(9600);                 //Inicia a serial LORA
serialGPS.begin(9600);                  //Inicia a serial GPS
delay(1000);

iniciarLORA();                          //Função que passa os parâmetros de funcionamento do módulo LORA
}

void loop() {             
     //getGPS();                        //Função que obtem os dados do GPS
     percursoFake();                    //Função que obtem os dados FAKES
 }

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
     //emissor.SetTransmitPower(OPT_TP30);                        //Força de transmissão 30db (Para módulos de 1W/8km)
     emissor.SetTransmitPower(OPT_TP20);                        //Força de transmissão 20db (Para módulos 100mW/3km)
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

void tensaoBat() {                                              //Função que calcula o valor de tensão da bateria, de acordo com o valor lido na porta A2

float aRef=5;                                                   //Máximo valor de tensão aceito pela porta A2
float relacaoA2=2.75;                                           //Fator para transformar o valor Vout recebido na porta A2, no valor de Vin (que entra antes do divisor de tensão) 
  
  tensaoA2 = ((lePorta(A2) * aRef) / 1024.0)* relacaoA2;        //Calculo de tensão
}  

void getGPS(){                                                  //Função que obtém os dados de GPS

   serialGPS.listen();
    
   bool recebido = false;
   unsigned long idadeInfo,date,tempo,pausa;
   float flat, flon;
   String diaMes, horaMin, dados;

   pausa = millis() + 10000;

  while (serialGPS.available()) {                             //Recebe os dados pela serial do GPS
     char cIn = serialGPS.read();
     recebido = gps.encode(cIn);
  }if (recebido) {
     gps.get_datetime(&date, &tempo, &idadeInfo);             //Obtem data e hora dos dados recebidos anteriormente
     gps.f_get_position(&flat, &flon, &idadeInfo);            //Obtem latitude e longitude dos dados recebidos anteriormente

     tempo = ((tempo/10000)-300);                             //Converte o formato da hora de HHMMSSMS para HHMM
     date = (date/100);                                       //Converte o formato da data de DDMMAA para DDMM

     if (tempo < 1000){
        horaMin = "0"+String(tempo);                          //Adiciona zero a esquerda para horas menores que 10, mantendo o numero padrão de 4 char(HHMM)
     } else{
      horaMin = String(tempo);                                //Para horas maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
     }

     if (date < 1000){              
      diaMes = "0"+String(date);                              //Adiciona zero a esquerda para dias menores que 10, mantendo o numero padrão de 4 char (DDMM)
     } else{
      diaMes = String(date);                                  //Para dias maiores que 10, não adiciona nada, apenas passa o valor como string para outra variavel
     }

     tensaoBat();                                             //Chama a função que irá obter a tensão da bateria

     dados = "[09,"+String(flat,6)+","+String(flon,6)+","+diaMes+","+horaMin+","+String(gps.f_speed_knots(),1)+","+String(tensaoA2,1)+"]*";        //Forma a string de dados que será enviada (
     Serial.println(dados);                                  //Exibe os dados (apenas para fins de teste)
     
     while (pausa > millis()){                               //deixa ativa a função receber por 10seg para aguardar caso o receptor solicite dados
        receber();
          if (id_equipe == "09"){                            //caso receba a ID correspondente, envia a string com os dados fornecidos pelo GPS 
          serialLORA.listen();    
          serialLORA.print(dados);
          break;
          }
        }
   }
}     
  
void receber()                                                          //Função responsavel por receber as mensagens de solicitação de dados do módulo receptor
  {
   serialLORA.listen(); 
    
   for (byte a = 0; a < 2; a++ ){                                       //Limpa a variavel mensagem, antes do uso
       mensagem[a] = "";
     }
     while (serialLORA.available()){                                    //Se houver dados a receber pela serial lógica
       bytesRecebidos = serialLORA.readBytesUntil('*', mensagem, 2);    //Recebe o caracter e o atribui a variavel mensagem
    } 
      id_equipe = String(mensagem[0])+""+String(mensagem[1]);           //cria uma string com os valores contidos em mesagem[1] e mensagem[2] **esta no formato 09(dois caracteres)
      }

void percursoFake()                                                     //Função que gera um percurso FAKE (apenas para testes temporariamente)
 {

   int vel = 100; //Define de quanto em quanto o loop irá avançar até atingir o valor limite definido no while
   int LO = 4660; //Valores iniciais de longitude
   int NS = 2370; //Valores iniciais de latitude
   String dados;
   unsigned long pausa;
   
   while (LO > 1880)
    {
     pausa = millis() + 10000;
     LO -= vel ;
     tensaoBat();                                                      //Chama a função que irá obter a tensão da bateria
     dados = "[08,-26.242370,-48.64"+(String)LO+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     //dados = "[09,-26.242370,-48.64"+(String)LO+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*"; //Para cada equipe, substituir de acordo, para que os módulos não enviem informações idênticas
     //dados = "[10,-26.242370,-48.64"+(String)LO+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     Serial.println(dados);
             
     while (pausa > millis()){
        receber();
        //if (id_equipe == "10" ){
        //if (id_equipe == "09"){
        if (id_equipe == "08"){
          serialLORA.listen();
          serialLORA.print(dados);
          serialLORA.print(mensagemrecebida);
          break;
          }
        }
      }
   
   while (NS < 5150)
    {
     pausa = millis() + 3000;
     LO = 1880;
     NS += vel;
     tensaoBat();         //Chama a função que irá obter a tensão da bateria
     dados = "[08,-26.24"+(String)NS+",-48.641880"+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     //dados = "[09,-26.24"+(String)NS+",-48.641880"+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     //dados = "[10,-26.24"+(String)NS+",-48.641880"+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     Serial.println(dados);
     
     while (pausa > millis()){
        receber();
        //if (id_equipe == "10" ){
        //if (id_equipe == "09"){
        if (id_equipe == "08"){
          serialLORA.listen();
          serialLORA.print(dados);
          break;
          }
       }
     }
    
   while (LO < 4660)
    {
      pausa = millis() + 3000;
     NS = 5150;
     LO += vel;
     tensaoBat();         
     dados = "[08,-26.245150,-48.64"+(String)LO+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     //dados = "[09,-26.245150,-48.64"+(String)LO+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     //dados = "[10,-26.245150,-48.64"+(String)LO+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     Serial.println(dados);
     
     while (pausa > millis()){
        receber();
        //if (id_equipe == "10" ){
        //if (id_equipe == "09"){
        if (id_equipe == "08"){
          serialLORA.listen();
          serialLORA.print(dados);
          break;
          }
       }
    }
    
   while (NS > 2370)
    {
      pausa = millis() + 3000;
     NS -= vel;
      tensaoBat();         
     dados = "[08,-26.24"+(String)NS+",-48.644660"+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     //dados = "[09,-26.24"+(String)NS+",-48.644660"+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     //dados = "[10,-26.24"+(String)NS+",-48.644660"+","+"2806,1821,0.1,"+String(tensaoA2,1)+"]*";
     Serial.println(dados);

     while (pausa > millis()){
        receber();
        
        //if (id_equipe == "10"){
        //if (id_equipe == "09"){
        if (id_equipe == "08"){
          serialLORA.listen();
          serialLORA.print(dados);
          break;
       }
     }
   }
 }
