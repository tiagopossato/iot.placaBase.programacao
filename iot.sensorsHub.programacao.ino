//https://github.com/Seeed-Studio/CAN_BUS_Shield

#include <SPI.h>
#include "mcp_can.h"
#include <avr/wdt.h>

//#define DEBUG

#define CENTRAL_ID 0x00
String inputString = "";// a string to hold incoming data

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 10;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

/**
   Estrutura com os estados das entradas e saídas de uma placa remota
*/
struct MENSAGEM {
  unsigned int id;
  unsigned char codigo;
  unsigned char msg[7];
} canMsg;

/**
   Função para resetar o programa
   Utiliza o WatchDog Timer
*/
void resetSensor() {
#if defined(DEBUG)
  Serial.println("Reiniciando...");
#endif
  wdt_enable(WDTO_15MS);
  while (1);
}

void setup()
{
  Serial.begin(115200);

  inputString.reserve(20);

  if (CAN_OK != CAN.begin(CAN_100KBPS))              // init can bus : baudrate = 500k
  {
#if defined(DEBUG)
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(250);
    resetSensor();
#endif
  }

  //envia sinal de online
  Serial.print(F("{\"id\":"));
  Serial.print(CENTRAL_ID);
  Serial.print(F(", \"codigo\":1}\n"));

}

void loop()
{
  unsigned char len = 0;
  unsigned char rawMsg[8];

  if (CAN.checkReceive() == CAN_MSGAVAIL) {

    CAN.readMsgBuf(&len, rawMsg);    // read data,  len: data length, buf: data buf

    canMsg.id = CAN.getCanId();
    canMsg.codigo = rawMsg[0];
    for (char i = 1; i < len; i++) {
      canMsg.msg[i - 1] = rawMsg[i];
    }

    Serial.print(F("{\"id\":"));
    Serial.print(canMsg.id);
    Serial.print(F(", \"codigo\":"));
    Serial.print(canMsg.codigo);
    Serial.print(F(", \"msg\":["));
    for (char i = 0; i < len-1; i++) {
      Serial.print(canMsg.msg[i]);
      if (i < len - 2)Serial.print(F(", "));
    }
    Serial.print(F("]}\n"));
  }
}

/**
   Recebe os dados lidos na porta serial
   Chamado depois de cada Loop
   Não utilizar delay no Loop
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      extraiComando();
      inputString = "";
      continue;
    }
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
  }
}

/**
   Extrai comando da mensagem recebida e envia par a rede
   A mensagem é recebida com os parametros separados por dois pontos (:)
   enderecoRemoto:comando:mensagem[6]
*/
byte extraiComando() {
  uint8_t i = 0;
  struct {
    unsigned char id;
    unsigned char comando;
    unsigned char msg[6];
  } sendMsg;

  String tmp;
  tmp.reserve(8);

  for (i = 0; i < inputString.length(); i++) {
    //if (i > 3)return false;
    if (inputString[i] == ':') {
      i++;
      break;
    }
    tmp += inputString[i];
  }
  sendMsg.id = tmp.toInt();
  if (sendMsg.id == 0) return false;
  tmp = "";

  for (; i < inputString.length(); i++) {
    //if (i > 8)return false;
    if (inputString[i] == ':') {
      i++;
      break;
    }
    tmp += inputString[i];
  }
  sendMsg.comando = tmp.toInt();
  if (sendMsg.comando == 0) return false;
  tmp = "";

  //identifica os proximos 6 bytes da mensagem
  for (unsigned char j = 0; j < 6; j++) {
    for (; i < inputString.length(); i++) {
      //if (i > 15)return false;
      if (inputString[i] == ':') {
        i++;
        break;
      }
      tmp += inputString[i];
    }
    sendMsg.msg[j] = tmp.toInt();
    tmp = "";
  }

#if defined(DEBUG)
  Serial.print(" -> sendMsg.id: ");
  Serial.println(sendMsg.id);
  Serial.print(" -> sendMsg.comando: ");
  Serial.println(sendMsg.comando);
  for (unsigned  char j = 0; j < 6; j++) {
    Serial.print(" -> sendMsg.msg[");
    Serial.print(j);
    Serial.print("]:");
    Serial.println(sendMsg.msg[j]);
  }
#endif
  unsigned char msg[8];
  msg[0] = sendMsg.id;
  msg[1] = sendMsg.comando;
  for (char j = 0; j < 6; j++) {
    msg[j + 2] = sendMsg.msg[j];
  }

  CAN.sendMsgBuf(CENTRAL_ID, 0, sizeof(msg), msg);

}
