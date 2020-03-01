/*
 
 SD card attached to SPI bus as follows:                  SoftSerial                        Objeto EBYTE                      LED de mensagem enviada/recebida
  ** MOSI - pin 11                                        ** 2,3 - GPS Rx, Tx               ** 7, 8, 9 - M0, M1, AUX          ** 4 - LED verde
  ** MISO - pin 12                                        ** 5,6 - LORA Rx, Tx      
  ** CLK - pin 13
  ** CS - pin 10 (for MKRZero SD: SDCARD_SS_PIN)
  
 */
 
//#include <SPI.h>
#include <SoftwareSerial.h> //Biblioteca SoftwareSerial
//#include <SD.h> //Biblioteca SD
#include <EBYTE.h> //Biblioteca dos módulos LORA

SoftwareSerial serialGPS(2,3); //Rx-Tx
SoftwareSerial serialLORA(5,6); //Rx-TX)

#define DEBUG true
//#define SD_CARD_CS      10

String NUMERO_BARCO = "7";                            // Número da embarcação
String response;                                      //Contém a informação bruta, enviada pelo módulo GPS, após o comando AT.
int    tamanhoString;                                 //Informa a qtd de char da variável response; Acima de 110 char, indica que os dados fornecidos pelo GPS são utilizaveis.
String data[7];                                       //Vetor de string que, após separados os dados relevantes existentes na variável response, 
                                                      //irá armazenar os valores referentes a Lat, Long, Time e status separadamente.
String pacoteMensagem;                                //Contém o pacote de mensagem a ser enviado pelo módulo LORA
//File   myFile;                                      //Arquivo de LOG do SD

EBYTE emissor(&serialLORA, 7, 8, 9);                  //Objeto EBYTE - parâmetros PINOS Rx,Tx, M0, M1, AUX

void setup()
{
  
   pinMode(4, OUTPUT);                               //LED verde de confirmação de recebimento de mensagem pelo módulo receptor
   digitalWrite(4, LOW);
      
   Serial.begin     (9600);
   serialGPS.begin  (9600);
   serialLORA.begin (9600);
  
 /*
   if (!SD.begin(10)) {
   Serial.println("Falha ao iniciar SD!!!\nDesligue o arduino e insira um cartão SD!");
   delay (3000);
   while (1);
   }
   Serial.println("Cartão SD iniciado");
*/

 Serial.println("Iniciando GPS...");
   while (response.length() != 21)                     //Analisa o tamanho da string de resposta do módulo GPS, caso seja 21 prossegue com o sketch
   {                     
    Serial.println("Iniciando GPS...");
    getgps();                                          //Função com comandos para iniciar o GPS.
    delay (3000);
   }  
 Serial.println("GPS iniciado");

 iniciarLORA();
}

      void loop()
       {
        serialGPS.listen();
        
        sendData("AT+CGNSINF",1000,DEBUG);            //Envia comando ao módulo solicitando dados do GPS
        criarMensagem();                              //Faz o tratamento da string recebida pelo módulo com todos dados fornecidos
        if (data[1] != 0)                             //Se o valor da variavel status enviado pelo GPS for diferente de 0, indica que os dados recebidos estão OK
         {                            
          while (data[1] != 0)                        //Enquanto o status for diferente de 0, permanece no loop enviando o comando para obter info, recebendo a 
           {                                          //string de resposta, salvando no SD, exibindo na serial, e enviando via LORA
            sendData("AT+CGNSINF",1000,DEBUG);  
            delay(1000);
            criarMensagem();
            enviarLORA();
            confirmaRecebimento(5000);
            //gravarSD();
            // delay(1000);
           }
        }
         else 
          {                                      //Caso o módulo perca o sinal com o GPS, tenta reconectar
           getgps();
           Serial.println("Aguardando Sinal GPS...");
           delay(5000);
          }       
        }

      void iniciarLORA () 
       {
         serialLORA.listen();                   
       
         emissor.init();                                        //Inicia o módulo
/*
         emissor.SetMode(MODE_NORMAL);                          //Modo de funcionamento do módul
         emissor.SetAddressH(0);                                //Endereço H(?)
         emissor.SetAddressL(0);                                //Endereço L(?)
         emissor.SetAirDataRate(ADR_2400);                      //AirDataRate 2400kbps
         emissor.SetUARTBaudRate(UDR_9600);                     //BAUDRate 9600
         emissor.SetChannel(23);                                //Canal 23
         emissor.SetParityBit(PB_8N1);                          //Bit Paridade 8N1
         emissor.SetTransmitPower(OPT_TP30);                    //Força de transmissão 30db
         emissor.SetWORTIming(OPT_WAKEUP2000);                  //WakeUP Time(?) 2000
         emissor.SetFEC(OPT_FECENABLE);                         //FEC(?)
         emissor.SetOptionTrans(OPT_OPTIONTRANSDISABLE);
         emissor.SetIOMode(OPT_IOPUSHPULL);                     //IO Mode PushPull
         emissor.SaveParameters(PERMANENT);                     //Salva as modificações na memória do módulo
*/         
         emissor.PrintParameters();                             //Exibe os parâmetros configurados
      }

    void confirmaRecebimento(const int timeout)                 //Função que aguarda por um char de resposta do receptor para piscar o LED
     {
      serialLORA.listen();
      long int time = millis();
      
      Serial.println("Aguardando resposta");
        
      while((time+timeout) > millis())
       {
        while (serialLORA.available())
         {
          char c = serialLORA.read();
          if (c == '1')
           {
            digitalWrite(4, HIGH);
            delay(500);
            digitalWrite(4, LOW);
            delay(500);
            digitalWrite(4, HIGH);
            delay(500);
            digitalWrite(4, LOW);
            break;
          }
        }
      }
    Serial.println("FIM");
   }
  
  void enviarLORA()                                                                                   //Função que envia o pacote com os dados do GPS
   {
    serialLORA.listen();
    
    if (pacoteMensagem.length() == 49 && pacoteMensagem[0] == '[' && pacoteMensagem[48] == ']' )      //Verifica se a string está inteira e consistente antes do envio
     {
      serialLORA.println (pacoteMensagem);
      serialLORA.println("OK");
     }
   }
   
/*
  void gravarSD()
   {
    //abre o arquivo para edição
    File myFile = SD.open("dados.txt", FILE_WRITE);
      if (myFile) 
       {
        Serial.println(pacoteMensagem);
        Serial.print("Writing to test.txt...");
        //grava o que foi digitado no arquivo
        myFile.println(pacoteMensagem);
        //fecha e salva o arquivo após edição
        myFile.close();
        Serial.println("done.");
       } else 
         {
          // if the file didn't open, print an error:
          Serial.println("error opening test.txt");
         } 
    }*/
   
   void criarMensagem ()                            //Função que faz o tratamento da string com os dados do GPS
    {                                    
     int i = 0;                                     //contador da string data
     tamanhoString = response.length();             //Informa a qtd de char da variável response; Acima de 110 char, indica que os dados fornecidos pelo GPS são utilizaveis.
     Serial.println();
     if (tamanhoString > 110)                       //indica que os dados fornecidos pelo GPS são utilizaveis.
      {                     
       for (byte a = 1; a < tamanhoString; a++)     //Enquanto o contador a for menor que o tamanho total da string, executa o loop para tratamento de response
        { 
         char h = response[a];                      //char h, variavel que armazena o valor da string response byte a byte
         if (h != ',')                              //enquanto h for diferente de "," preenche a string
          {    
           data[i] +=h;
           h = "";                                   //esvazia a variavel para o próximo loop
           delay(100);
          } else 
             {                                      //caso h seja igual "," incrementa o contador i o que faz criar uma nova string para armazenar dados da variavel response
              i++;  
             }
               if (i == 7)                          //quando o contador i chega a 7, todas variáveis com os dados relevantes do GPS estão criadas.
                {   
                 pacoteMensagem = "["+NUMERO_BARCO+","+data[2]+","+data[3]+","+data[4]+","+data[6]+"]";

                 Serial.println("Barco Nº: "+NUMERO_BARCO);
                 Serial.println("Status: "     +data[1]);
                 Serial.println("Time: "       +data[2]);
                 Serial.println("Latitude: "   +data[3]);
                 Serial.println("Longitude: "  +data[4]);
                 Serial.println("Velocidade: " +data[6]+" Nós");
                 Serial.println(pacoteMensagem);
                 break;
                }
        }
     }    
   }
  
      void getgps(void)                                   //Função que ativa o GPS
       {
        sendData( "AT+CGNSPWR=1",1000,DEBUG);            //Liga o GPS em modo GNSS
        Serial.println(response); 
        sendData( "AT+CGNSSEQ=RMC",1000,DEBUG);          //Parâmetro de configuração
        Serial.println(response); 
      }
      
     String sendData(String command, const int timeout, boolean debug)
      {
       serialGPS.listen();
    
        //Efetua a limpeza das variáveis antes de executar a função
        data[0]="";                                     //Informações irrelevantes
        data[1]="";                                     //Status do GPS
        data[2]="";                                     //Time
        data[3]="";                                     //Latitude
        data[4]="";                                     //Longitude
        data[5]="";                                     //Informações irrelevantes
        data[6]="";                                     //Velocidade Nós
        data[7]="";                                     //Informações irrelevantes
        response = "";                                  //Informações completas do módulo GPS
        pacoteMensagem = "";
    
        serialGPS.println(command); 
        
        long int time = millis();   
    
        while( (time+timeout) > millis())               //Enquanto a soma de time(millis()) + timeout for maior do que o valor atual de millis(), lê caracteres da Serial.
         {
          while(serialGPS.available())
           {       
            char c = serialGPS.read(); 
            response+=c;
           }  
         }    
          if(debug)                                       //Se debug for true, retorna o valor da variavel response
           {
            //Serial.print(response);
            return response;
           }
     }
