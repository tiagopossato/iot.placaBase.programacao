//https://github.com/Seeed-Studio/CAN_BUS_Shield

#include <SPI.h>
#include "mcp_can.h"
#include <avr/wdt.h>
#include "util.h"
#include "definicoes.h"

#define DEBUG

#define CENTRAL_ID 0x00
#define INTERVALO_ONLINE 10000 //Intervalo para enviar sinal de online

char input[64]; // a string to hold incoming data

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 6;

MCP_CAN CAN(SPI_CS_PIN); // Set CS pin

/**
   Função para resetar o programa
   Utiliza o WatchDog Timer
*/
void resetSensor()
{
#if defined(DEBUG)
  Serial.println("Reiniciando...");
#endif
  wdt_enable(WDTO_15MS);
  while (1);
}

void setup()
{
  Serial.begin(115200);

  input[0] = '\0';

  if (CAN_OK != CAN.begin(CAN_100KBPS)) // init can bus : baudrate = 500k
  {
#if defined(DEBUG)
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(250);
    resetSensor();
#endif
  }
  //envia sinal de online
  isOnline();
}

void isOnline()
{
  Serial.print(F("["));
  Serial.print(CENTRAL_ID);
  Serial.print(F("/"));
  Serial.print(ESPECIAL + 9900);
  Serial.print(F("/"));
  Serial.print(ONLINE + 9900);
  Serial.print(F("/"));
  Serial.print(1);
  Serial.println(F("]"));
  Serial.flush();
}

void loop()
{
  unsigned char len = 0;
  unsigned char rawMsg[8];
  static unsigned long previousMillis = 0; // will store last time LED was updated

  if (CAN.checkReceive() == CAN_MSGAVAIL)
  {

    CAN.readMsgBuf(&len, rawMsg); // read data,  len: data length, buf: data buf
    Serial.print(F("["));
    Serial.print(CAN.getCanId());
    Serial.print("/");
    if (rawMsg[0] == ESPECIAL)
    {
      Serial.print(F("9999"));
      Serial.print(F("/"));
      Serial.print(rawMsg[1] + 9900);
      Serial.print(F("/"));
      Serial.print(rawMsg[2]);
    }
    else
    {
      Serial.print(rawMsg[0] + 3200);
      Serial.print(F("/"));
    }


    //se for valor analógico
    if (rawMsg[0] == ENTRADA_ANALOGICA)
    {
      Serial.print(rawMsg[1] + 3300);
      Serial.print(F("/"));
      Serial.print((int16_t)(word(rawMsg[2], rawMsg[3])) / 100.0);
    }

    if (rawMsg[0] == ENTRADA_DIGITAL || rawMsg[0] == SAIDA_DIGITAL)
    {
      Serial.print(rawMsg[1]);
      Serial.print(F("/"));
      Serial.print(rawMsg[2]);
    }

    Serial.println(F("]"));
    Serial.flush();
  }

  //envia sinal de online a cada tempo predeterminado
  if (millis() - previousMillis >= INTERVALO_ONLINE)
  {
    previousMillis = millis();
    isOnline();
  }
}

/**
   Recebe os dados lidos na porta serial
   Chamado depois de cada Loop
   Não utilizar delay no Loop
*/
void serialEvent()
{
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    if (inChar != '\n')
    {
      sprintf(input, "%s%c", input, inChar);
    }
    if (inChar == '\n' || strlen(input) == 63)
    {
      if (strlen(input) >= 1) {
        if (sanitizaEntrada(input)) {
          trataComando();
        }
      }
      for (uint8_t i = 0; i < 64; i++) {
        input[i] = '\0';
      }
    }
  }
}

void trataComando()
{
  unsigned char sendMsg[] = {0, 0, 0, 0, 0, 0, 0, 0};
  //idRede
  sendMsg[0] = (uint8_t)extraiCodigo(input);
  //tipoGrandeza
  uint16_t tmp = (uint16_t)extraiCodigo(input);
  //SE FOR GRANDEZA ESPECIAL
  if (tmp == 9999) {
    sendMsg[1] = 99;
    //pega o codigo da grandeza
    sendMsg[2] = (uint8_t)(extraiCodigo(input) - 9900);
    //pega o valor a ser escrito
    // em uma variável de 16 bits,
    //para aumentar a compatibilidade do codigo dos transdutores
    int16_t valor = (int16_t)(extraiCodigo(input) * 100);
    sendMsg[3] = highByte(valor);
    sendMsg[4] = lowByte(valor);
    CAN.sendMsgBuf(CENTRAL_ID, 0, sizeof(sendMsg), sendMsg);
    return;

  }
  //tipoGrandeza
  sendMsg[1] = (uint8_t)( tmp - 3200);
  //verifica se a grandeza é do tipo entradaDigital
  if (sendMsg[1] == 0)
  {
    //pega a entrada que será lida
    sendMsg[2] = (uint8_t)extraiCodigo(input);
    CAN.sendMsgBuf(CENTRAL_ID, 0, sizeof(sendMsg), sendMsg);
    return;
  }

  //verifica se a grandeza é do tipo saidaDigital
  if (sendMsg[1] == 1)
  {
    //pega a saída que será alterada
    sendMsg[2] = (uint8_t)extraiCodigo(input);
    //pega o valor a ser escrito na saída
    // em uma variável de 16 bits,
    //para aumentar a compatibilidade do codigo dos transdutores
    int16_t valor = (int16_t)(extraiCodigo(input) * 100);
    sendMsg[3] = highByte(valor);
    sendMsg[4] = lowByte(valor);
    CAN.sendMsgBuf(CENTRAL_ID, 0, sizeof(sendMsg), sendMsg);
    return;
  }

  //se nao for nenhum dos outros casos,
  //deve ser uma leitura analogica

  //pega a grandeza a ser lida
  sendMsg[2] = (uint8_t)(extraiCodigo(input) - 3300);
  CAN.sendMsgBuf(CENTRAL_ID, 0, sizeof(sendMsg), sendMsg);

}

