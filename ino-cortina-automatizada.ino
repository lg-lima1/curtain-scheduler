// Macros
#define DEBUG 1

// PinOut - Botões
#define BTN_ONE A1
#define BTN_TWO A2
#define BTN_THREE A3

void setup()
{
#if DEBUG
  // Inicializa porta serial com baud rate de 115200 bps
  Serial.begin(115200);
#endif

  // Setup - Botões
  pinMode(BTN_ONE, INPUT_PULLUP);
  pinMode(BTN_TWO, INPUT_PULLUP);
  pinMode(BTN_THREE, INPUT_PULLUP);
}

void loop()
{
  // Verifica estado dos botões a cada ciclo
  bool buttonOne = !digitalRead(BTN_ONE);
  bool buttonTwo = !digitalRead(BTN_TWO);
  bool buttonThree = !digitalRead(BTN_THREE);

#if DEBUG
  Serial.println("-----------");

  Serial.print("buttonState: ");
  Serial.print(buttonOne);
  Serial.print(buttonTwo);
  Serial.print(buttonThree);
#endif
}
