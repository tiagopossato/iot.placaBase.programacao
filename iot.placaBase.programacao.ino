#include <avr/wdt.h>

#define CENTRAL_ID 0x00
#define INTERVALO_ONLINE 500 //Intervalo para enviar sinal de online

#define ESPECIAL 9999
#define ONLINE 1

String outputString = String("");

/**
   Estrutura com os estados das entradas e saídas de uma placa remota
   idRede/tipoGrandeza/grandeza/valor
*/
struct Mensagem {
  uint16_t idRede;
  uint16_t tipoGrandeza;
  uint16_t grandeza;
  float valor;
};

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

  //envia sinal de online
  isOnline();

  randomSeed(1);
}

void isOnline() {
  Mensagem msg;

  msg.idRede = CENTRAL_ID;
  msg.tipoGrandeza = ESPECIAL;
  msg.grandeza = ONLINE;
  msg.valor = ONLINE;
  enviaDados(&msg);

}

void enviaDados(Mensagem *msg) {
  outputString = String("");
  outputString += msg->idRede;
  outputString += '\\';
  outputString += msg->tipoGrandeza;
  outputString += '\\';
  outputString += msg->grandeza;
  outputString += '\\';
  outputString += msg->valor;
  Serial.println(outputString);
}

void loop()
{
  static Mensagem msg;
  static unsigned long previousMillis = 0;
  //envia sinal de online a cada tempo predeterminado
  if (millis() - previousMillis >= INTERVALO_ONLINE) {
    
    previousMillis = millis();
    msg.idRede = random(1, 2);
    msg.tipoGrandeza = 3202;
    msg.grandeza = random(3303, 3304);
    msg.valor = random(1, 5000) / 100.0;
    enviaDados(&msg);
    
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
