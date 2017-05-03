//https://github.com/Seeed-Studio/CAN_BUS_Shield

#include <SPI.h>
#include "mcp_can.h"
#include <avr/wdt.h>
#include "ctmNectar.h"

//#define DEBUG

#define CENTRAL_ID 0x00
#define INTERVALO_ONLINE 2000 //Intervalo para enviar sinal de online

String inputString = "";// a string to hold incoming data

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 6;

MCP_CAN CAN(SPI_CS_PIN);// Set CS pin

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
  isOnline();
}

void isOnline() {
  Serial.print(F("["));
  Serial.print(CENTRAL_ID);
  Serial.print(F("/"));
  Serial.print(F("9999"));
  Serial.print(F("/"));
  Serial.print(F("1"));
  Serial.print(F("/"));
  Serial.print(F("1"));
  Serial.println(F("]"));
  Serial.flush();
}

void loop()
{
  unsigned char len = 0;
  unsigned char rawMsg[8];
  static unsigned long previousMillis = 0;        // will store last time LED was updated


  if (CAN.checkReceive() == CAN_MSGAVAIL) {

    CAN.readMsgBuf(&len, rawMsg);    // read data,  len: data length, buf: data buf
    Serial.print("[");
    Serial.print(CAN.getCanId());
    Serial.print("/");
    if (rawMsg[0] == 99) {
      Serial.print("9999");
    }
    else {
      Serial.print(rawMsg[0] + 3200);
    }

    Serial.print("/");
    //se for valor analógico
    if (rawMsg[0] == 2) {
      Serial.print(rawMsg[1] + 3300);
      Serial.print("/");
      Serial.print(word(rawMsg[2], rawMsg[3]) / 100.0);
    } else {
      Serial.print(rawMsg[1]);
      Serial.print("/");
      Serial.print(rawMsg[2]);
    }
    Serial.println("]");
    Serial.flush();
  }

  //envia sinal de online a cada tempo predeterminado
  if (millis() - previousMillis >= INTERVALO_ONLINE) {
    previousMillis = millis();
    isOnline();
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

  //verifica se a mensagem é para este dispositivo
  if (sendMsg.id == CENTRAL_ID) {
    switch (sendMsg.comando) {
      case IS_ONLINE: {
          isOnline();
          break;
        }
    }
  } else {
    //envia pela rede
    CAN.sendMsgBuf(CENTRAL_ID, 0, sizeof(msg), msg);
  }
}
