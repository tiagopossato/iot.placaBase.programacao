char tmp[63] = {};

uint8_t buscaCaracter(char *entrada, char caracter)
{
  uint8_t pos = 0;
  for (; pos <= strlen(entrada); pos++)
  {
    if (caracter == entrada[pos])
    {
      return pos;
    }
  }
  return 255;
}

boolean sanitizaEntrada(char *entrada) {
  uint8_t inicio;
  uint8_t fim;

  inicio = buscaCaracter(entrada, '[');
  if (inicio == 255) {
    return false;
  }
  fim = buscaCaracter(entrada, ']');
  if (fim == 255) {
    return false;
  }

  memcpy(tmp, &entrada[inicio + 1], fim - (inicio + 1));
  sprintf(entrada, tmp);
  sprintf(entrada, "%s", tmp);
  for (uint8_t i = 0; i < 64; i++) {
    tmp[i] = '\0';
  }
  return true;
}

float extraiCodigo(char *entrada) {
  unsigned char i = 0;
  String tmp;
  tmp.reserve(5);
  for (i = 0;; i++) {
    //aceita valores de -32,768 até 32,767
    if (i > 5) break;
    if (entrada[i] == '/') break;
    tmp += entrada[i];
  }

  sprintf(entrada, entrada + i + 1);
  return tmp.toFloat();
}
