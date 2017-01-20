  //https://github.com/Seeed-Studio/CAN_BUS_Shield

#include <SPI.h>
#include "mcp_can.h"

#define DEBUG

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
  unsigned char msg[8];
} canMsg;

void setup()
{
  Serial.begin(115200);

  inputString.reserve(20);

  while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 500k
  {
#if defined(DEBUG)
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
#endif
    delay(100);
  }
#if defined(DEBUG)
  Serial.println("Central: CAN BUS init ok!");
#endif
}

void loop()
{

  unsigned char len = 0;

  if (CAN.checkReceive() == CAN_MSGAVAIL) {


    CAN.readMsgBuf(&len, canMsg.msg);    // read data,  len: data length, buf: data buf

    canMsg.id = CAN.getCanId();

    Serial.print(F("{\"id\":"));
    Serial.print(canMsg.id);
    Serial.print(F(", \"msg\":["));
    
    for (char i = 0; i < len; i++) {
      Serial.print(canMsg.msg[i]);
      Serial.print(F(","));
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
   enderecoRemoto:comando:mensagem;
*/
byte extraiComando() {
  uint8_t i = 0;
  struct {
    unsigned char id;
    unsigned char comando;
    unsigned char valor;
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

  for (; i < inputString.length(); i++) {
    //if (i > 15)return false;
    if (inputString[i] == ';') {
      i++;
      break;
    }
    tmp += inputString[i];
  }
  sendMsg.valor = tmp.toInt();
  if (sendMsg.valor == 0) return false;
  tmp = "";

#if defined(DEBUG)
  Serial.print(" -> sendMsg.id: ");
  Serial.println(sendMsg.id);
  Serial.print(" -> sendMsg.comando: ");
  Serial.println(sendMsg.comando);
  Serial.print(" -> sendMsg.valor: ");
  Serial.println(sendMsg.valor);
#endif

  unsigned char msg[8];
  msg[0] = sendMsg.id;
  msg[1] = sendMsg.comando;
  msg[2] = sendMsg.valor;

  CAN.sendMsgBuf(CENTRAL_ID, 0, sizeof(msg), msg);

}
