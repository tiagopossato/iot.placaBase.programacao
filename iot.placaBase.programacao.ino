#include <avr/wdt.h>

#define CENTRAL_ID 0x00
#define INTERVALO_ONLINE 1000 //Intervalo para enviar sinal de online

#define ESPECIAL 9999
#define ONLINE 1

#define DEBUG
String inputString;// a string to hold incoming data

/**
   Estrutura com os estados das entradas e saídas de uma placa remota
   idRede/tipoGrandeza/grandeza/valor
*/
struct Mensagem {
  uint16_t idRede;
  uint16_t tipoGrandeza;
  uint16_t grandeza;
  int16_t valor;
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

  inputString.reserve(30);

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
  mostraDados(&msg);

}

void mostraDados(Mensagem *msg) {
  String tmp = String("");
  String outputString;
  outputString.reserve(30);
  outputString = msg->idRede;
  outputString += "/";

  if (msg->tipoGrandeza != ESPECIAL) {
    outputString += "32";
    tmp = msg->tipoGrandeza;
    if (tmp.length() == 1) tmp = "0" + tmp;
    outputString += tmp;
  } else {
    outputString += msg->tipoGrandeza;
  }

  outputString += "/";

  // tipo 00 representa entrada digital
  if (msg->tipoGrandeza == 00 || msg->tipoGrandeza == ESPECIAL) {
    outputString += msg->grandeza;
    outputString += '/';
    outputString += msg->valor;
  } else {
    outputString += "33";
    tmp = msg->grandeza;
    if (tmp.length() == 1) tmp = "0" + tmp;
    outputString += tmp;
    outputString += '/';
    outputString += msg->valor / 100.0;
  }

  Serial.println(outputString);
}

void loop()
{
  //  static Mensagem msg;
  //  static unsigned long previousMillis = 0;
  //  //envia sinal de online a cada tempo predeterminado
  //  if (millis() - previousMillis >= INTERVALO_ONLINE) {
  //
  //    previousMillis = millis();
  //    msg.idRede = random(1, 5);
  //    if (msg.idRede == 3) {
  //      msg.tipoGrandeza = 00;
  //      msg.grandeza = random(0, 8);
  //      msg.valor = random(0, 2);
  //    } else {
  //      msg.tipoGrandeza = 02;
  //      msg.grandeza = random(03, 05);
  //      msg.valor = random(-1500, 5000);
  //    }
  //
  //    mostraDados(&msg);
  //
  //  }
}

/**
   Recebe os dados lidos na porta serial
   Chamado depois de cada Loop
   Não utilizar delay no Loop
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = Serial.read();
    if (inChar == '\n') {
      extraiComando();
      inputString = String("");
      continue;
    }
    //Insere na string somente numeros positivos e a barra;
    if (isDigit(inChar) ||  (char)inChar == '/') {
      // convert the incoming byte to a char
      // and add it to the string:
      inputString += (char)inChar;
    }
  }
}

/**
   Extrai comando da mensagem recebida e envia pela rede
   A mensagem é recebida com os parametros separados por uma barra ( / )
   idRede:uint8_t / objeto:uint16_t / recurso:uint16_t / operacao:uint8_t / informacao:uint16_t
*/
byte extraiComando() {
  uint8_t i = 0;
  struct {
    uint8_t idRede;
    uint16_t objeto;
    uint16_t recurso;
    uint8_t operacao;
    uint16_t informacao;
  } sendMsg;

  String tmp;
  tmp.reserve(8);

  //Extrai o ID de rede
  for (i = 0; i < inputString.length(); i++) {
    if (i > 3) {
#if defined(DEBUG)
      Serial.println("idRede muito longo");
#endif
      return false;
    }
    if (inputString[i] == '/') {
      i++;
      break;
    }
    tmp += inputString[i];
  }
  sendMsg.idRede = (uint8_t) tmp.toInt();
  tmp = "";

  //Extrai o objeto
  for (; i < inputString.length(); i++) {
    if (i > 8) {
#if defined(DEBUG)
      Serial.println("Objeto muito longo");
#endif
      return false;
    }
    if (inputString[i] == '/') {
      i++;
      break;
    }
    tmp += inputString[i];
  }
  sendMsg.objeto = (uint16_t) tmp.toInt();
  if (sendMsg.objeto == 0) {
#if defined(DEBUG)
    Serial.println("Objeto inválido");
#endif
    return false;
  }
  tmp = "";

  //Extrai o recurso
  for (; i < inputString.length(); i++) {
    if (i > 13) {
#if defined(DEBUG)
      Serial.println("Recurso muito longo");
#endif
      return false;
    }
    if (inputString[i] == '/') {
      i++;
      break;
    }
    tmp += inputString[i];
  }
  sendMsg.recurso = (uint16_t) tmp.toInt();
  if (sendMsg.recurso == 0) {
#if defined(DEBUG)
    Serial.println("Recurso inválido");
#endif
    return false;
  }
  tmp = "";

  //Extrai a operacao
  for (; i < inputString.length(); i++) {
    if (i > 17) {
#if defined(DEBUG)
      Serial.println("Operacao muito longo");
#endif
      return false;
    }
    if (inputString[i] == '/') {
      i++;
      break;
    }
    tmp += inputString[i];
  }
  sendMsg.operacao = (uint8_t) tmp.toInt();
  tmp = "";


  //cria uma string com a ultima parte, referente à informação que será enviada
  String info = inputString.substring(i);
  //verifica se existe somente uma barra
  if (info.indexOf('/') != info.lastIndexOf('/')) {
#if defined(DEBUG)
    Serial.println("Mais de uma barra na String");
#endif
    return false;
  }

  //Se não tiver nenhuma barra na string, é um valor de 16 bits
  if (info.indexOf('/') == -1) {
    //Extrai a informacao
    for (; i < inputString.length(); i++) {
      if (i > 23) {
#if defined(DEBUG)
        Serial.println("Informacao muito longo");
#endif
        return false;
      }
      if (inputString[i] == '/') {
        i++;
        break;
      }
      tmp += inputString[i];
    }
    sendMsg.informacao = (uint16_t) tmp.toInt();
    tmp = "";
  } else {
    Serial.println("Ainda nao implementado com barra no meio");
  }

#if defined(DEBUG)
  Serial.print(" -> sendMsg.idRede: ");
  Serial.println(sendMsg.idRede);
  Serial.print(" -> sendMsg.objeto: ");
  Serial.println(sendMsg.objeto);
  Serial.print(" -> sendMsg.recurso: ");
  Serial.println(sendMsg.recurso);
  Serial.print(" -> sendMsg.operacao: ");
  Serial.println(sendMsg.operacao);
  Serial.print(" -> sendMsg.informacao: ");
  Serial.println(sendMsg.informacao);
#endif

  unsigned char msg[8];
  msg[0] = sendMsg.idRede;
  msg[1] = lowByte(sendMsg.objeto);
  msg[2] = highByte(sendMsg.objeto);
  msg[3] = lowByte(sendMsg.recurso);
  msg[4] = highByte(sendMsg.recurso);
  msg[5] = sendMsg.operacao;
  msg[6] = lowByte(sendMsg.informacao);
  msg[7] = highByte(sendMsg.informacao);

  //verifica se a mensagem é para este dispositivo
  if (sendMsg.idRede == CENTRAL_ID) {
    switch (sendMsg.objeto) {
      case ONLINE: {
          isOnline();
          break;
        }
    }
  } else {
    //envia pela rede
    //CAN.sendMsgBuf(CENTRAL_ID, 0, sizeof(msg), msg);
  }
}
