/*

                          **************SKETCH PARA MÓDULOS DE 1W/8km "EBYTE E32 433T30D" E 100mW/3km "EBYTE E32 433T20D"**************
                              ***SUBSTITUIR O VALOR DO PARÂMETRO "SetTransmitPower" NA FUNÇÃO "INICIAR LORA" CONFORME NECESSÁRIO***
                          
                   SoftSerial (Saídas Digitais)      Objeto EBYTE (Saídas Digitais)    
                   ** 3,2 -  RX,TX  (GPS)           ** 7, 8, 10 - M0, M1, AUX(Definido mas não conectado)          
                   ** 9,6 -  RX,TX (LORA)                                                
  
 */

#include <EBYTE.h>
#include <SoftwareSerial.h>

int id_equipe;

char mensagem[3];  //Array de caracteres que irá armazenar a mensagem a ser recebida
size_t bytesRecebidos;

SoftwareSerial mySerial(9,6); //Rx - Tx (LORA)

EBYTE emissor(&mySerial, 7, 8, 10); //Parâmetros do módulo LORA (RX,TX,M0,M1,AUX)

void setup() {
  
Serial.begin(9600); //Inicia a serial física
mySerial.begin(9600); //Inicia a serial lógica

iniciarLORA(); //Função que passa os parâmetros de funcionamento do módulo LORA
}

void loop() 
 {  
             
             percursoFake(); //Função que gera um percurso FAKE (apenas para testes temporariamente)
 }


 void iniciarLORA () 
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
     emissor.SetTransmitPower(OPT_TP30);                        //Força de transmissão 30db (Para módulos de 1W/8km definir como 30)
     //emissor.SetTransmitPower(OPT_TP20);                      //Força de transmissão 20db (Para módulos 100mW/3km definir como 20)
     emissor.SetWORTIming(OPT_WAKEUP250);                       //WakeUP Time(?) 2000
     emissor.SetFECMode(OPT_FECENABLE);                         //FEC(?) ENABLE
     emissor.SetTransmissionMode(OPT_FMDISABLE);                //Transmission Mode
     emissor.SetPullupMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
     emissor.SaveParameters(PERMANENT);                         //Salva as modificações na memória do módulo
         
     emissor.PrintParameters();                                 //Exibe os parâmetros configurados
  } //Fim iniciarLORA 

void percursoFake() //Função que gera um percurso FAKE (apenas para testes temporariamente)
 {

   int vel = 100; //Define de quanto em quanto o loop irá avançar até atingir o valor limite definido no while
   int LO = 4660; //Valores de longitude
   int NS = 2370; //Valores de latitude
   String dados;
   unsigned long pausa;
   
   while (LO > 1880)
    {
     pausa = millis() + 3000;
     LO -= vel ;
     dados = "[08,-26.242370,-48.64"+(String)LO+"]*";
     //dados = "[09,-26.242370,-48.64"+(String)LO+"]*"; //Para cada equipe, substituir de acordo, para que os módulos não enviem informações idênticas
     //dados = "[10,-26.242370,-48.64"+(String)LO+"]*";
     Serial.println(dados);
             
     while (pausa > millis()){
        receber();
        //if (id_equipe == 10 ){
        //if (id_equipe == 9){
        if (id_equipe == 8){
          mySerial.print(dados);
          break;
          }
        }
      }
   
   while (NS < 5150)
    {
     pausa = millis() + 3000;
     LO = 1880;
     NS += vel;
     dados = "[08,-26.24"+(String)NS+",-48.641880]*";
     //dados = "[09,-26.24"+(String)NS+",-48.641880]*";
     //dados = "[10,-26.24"+(String)NS+",-48.641880]*";
     Serial.println(dados);
     
     while (pausa > millis()){
        receber();
        //if (id_equipe == 10){
        //if (id_equipe == 9){
        if (id_equipe == 8){
          mySerial.print(dados);
          break;
          }
       }
     }
    
   while (LO < 4660)
    {
      pausa = millis() + 3000;
     NS = 5150;
     LO += vel;

     dados = "[08,-26.245150,-48.64"+(String)LO+"]*";
     //dados = "[09,-26.245150,-48.64"+(String)LO+"]*";
     //dados = "[10,-26.245150,-48.64"+(String)LO+"]*";
     Serial.println(dados);
     
     while (pausa > millis()){
        receber();
        //if (id_equipe == 10){
        //if (id_equipe == 9){
        if (id_equipe == 8){
          mySerial.print(dados);
          break;
          }
       }
    }
    
   while (NS > 2370)
    {
      pausa = millis() + 3000;
     NS -= vel;

     dados = "[08,-26.24"+(String)NS+",-48.644660]*";
     //dados = "[09,-26.24"+(String)NS+",-48.644660]*";
     //dados = "[10,-26.24"+(String)NS+",-48.644660]*";
     Serial.println(dados);

     while (pausa > millis()){
        receber();
        
        //if (id_equipe == 10){
        //if (id_equipe == 9){
        if (id_equipe == 8){
          mySerial.print(dados);
          break;
       }
     }
   }
 }
 
void receber() //Função responsavel por receber as mensagens de solicitação do módulo receptor
  {
    
    
  //Limpa a variavel mensagem, antes do uso
     for (byte a = 0; a < 2; a++ ){
       mensagem[a] = "";
     }
     while (mySerial.available()){ //Se houver dados a receber pela serial lógica
       bytesRecebidos = mySerial.readBytesUntil('*', mensagem, 2);//Recebe o caracter e o atribui a variavek mensagem
    }
      String id_txt = String(mensagem[0])+""+String(mensagem[1]); //cria uma string com os valores contidos em mesagem[1] e mensagem[2] **esta no formato 09(dois caracteres
      id_equipe = id_txt.toInt();   //converte a string em int para uso no switch case **quando convertido de string para int, passa para 1 caracter 09 -> 9, 10 -> 10
    }
