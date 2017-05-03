float extraiCodigo(char *entrada) {
  char i = 0;
  String tmp;
  tmp.reserve(5);
  for (i = 0;; i++) {
    //aceita valores de -32,768 até 32,767
    if (i > 5) break;
    if (entrada[i] == '/') break;
    tmp += entrada[i];
  }

  sprintf(entrada, entrada + i + 1);
  //Serial.print("Numero: ");
  //Serial.println(tmp.toFloat());
  return tmp.toFloat();
}
