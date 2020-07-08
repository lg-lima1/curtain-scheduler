// Bibliotecas
#include <LiquidCrystal.h>
#include <Stepper.h>

// Macros
#define DEBUG 1

// PinOut - Botões
#define BTN_ONE A1
#define BTN_TWO A2
#define BTN_THREE A3

// PinOut - Stepper
#define M1 22
#define M2 23
#define M3 24
#define M4 25
#define HOME_SWITCH A0

// PinOut - LCD
#define RS 2
#define E 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7

// Estrutura de variáveis referentes ao motor de passo, e acionamentos relacionados
struct AxisData
{
  int stepsPerRevolution = 64;
  int ratio = 32;
  int length = 10; // Voltas
  float turns = 0.0;
  bool opened = false;
  bool closed = false;
  bool homed = false;
} axisData;

// Enumerador para máquina de estado
enum MachineState
{
  INITIALIZE = 0,
  HOMING,
  CHECK_BUTTTONS,
  OPEN_WINDOW,
  CLOSE_WINDOW,
  END_STATE
} machineState;

// Declaração de bibliotecas externas
Stepper stepper = Stepper(axisData.stepsPerRevolution, M1, M3, M2, M4);
LiquidCrystal lcd = LiquidCrystal(RS, E, D4, D5, D6, D7);

void setup()
{
#if DEBUG
  // Inicializa porta serial com baud rate de 115200 bps
  Serial.begin(115200);
#endif

  // Setup - LCD
  lcd.begin(16, 2);

  // Setup - Botões
  pinMode(BTN_ONE, INPUT_PULLUP);
  pinMode(BTN_TWO, INPUT_PULLUP);
  pinMode(BTN_THREE, INPUT_PULLUP);

  // Setup - Stepper
  pinMode(HOME_SWITCH, INPUT_PULLUP);
  stepper.setSpeed(500);

  // Delay de Inicialização
  lcd.setCursor(0, 0);
  lcd.print("CORTINA         ");
  lcd.setCursor(0, 1);
  lcd.print("    AUTOMATIZADA");
  delay(5000);
  lcd.clear();
}

void loop()
{
  // Verifica estado dos botões a cada ciclo
  bool homeSwitchState = !digitalRead(HOME_SWITCH);
  bool buttonOne = !digitalRead(BTN_ONE);
  bool buttonTwo = !digitalRead(BTN_TWO);
  bool buttonThree = !digitalRead(BTN_THREE);

  switch (machineState)
  {
  case INITIALIZE:
    machineState = HOMING;
    break;

  case HOMING:
    if (axisData.homed || homeSwitchState)
    {
      if (!axisData.homed)
      {
        axisData.homed = true;
        axisData.opened = true;
        axisData.closed = false;
        axisData.turns = 0.0f;
      }
      lcd.setCursor(0, 0);
      lcd.print("REFERENCIADO!   ");
      machineState = CHECK_BUTTTONS;
    }
    else
    {
      lcd.setCursor(0, 0);
      lcd.print("REFERENCIANDO...");
      stepper.step(-1 * axisData.stepsPerRevolution);
    }

    break;

  case CHECK_BUTTTONS:
    lcd.setCursor(0, 0);
    lcd.print("HORA ATUAL      ");

    if (buttonOne)
      if (!axisData.opened)
        machineState = OPEN_WINDOW;

    if (buttonTwo)
      if (!axisData.closed)
        machineState = CLOSE_WINDOW;

    break;

  case OPEN_WINDOW:
    lcd.setCursor(0, 0);
    lcd.print("COMANDO         ");
    lcd.setCursor(0, 1);
    lcd.print("ABRINDO JANELA  ");
    axisData.opened = true;
    axisData.closed = false;
    stepper.step(getAbsoluteSteps(0));
    machineState = END_STATE;
    break;

  case CLOSE_WINDOW:
    lcd.setCursor(0, 0);
    lcd.print("COMANDO         ");
    lcd.setCursor(0, 1);
    lcd.print("FECHANDO JANELA ");
    axisData.opened = false;
    axisData.closed = true;
    stepper.step(getAbsoluteSteps(axisData.length));
    machineState = END_STATE;
    break;

  case END_STATE:
    machineState = HOMING;
    break;

  default:
    break;
  }

#if DEBUG
  Serial.println("-----------");

  Serial.print("buttonState: ");
  Serial.print(buttonOne);
  Serial.print(buttonTwo);
  Serial.print(buttonThree);

  Serial.print("machineState: ");
  Serial.println(machineState);

  Serial.print("axisData : ");
  Serial.print(axisData.turns);
  Serial.print(", ");
  Serial.print(axisData.opened);
  Serial.print(", ");
  Serial.print(axisData.closed);
  Serial.print(", ");
  Serial.println(axisData.homed);
#endif
}

long getAbsoluteSteps(float act)
{
  float newTurns = act - axisData.turns;
  axisData.turns = act;
  return newTurns * axisData.ratio * axisData.stepsPerRevolution;
}

long getRelativeSteps(float act)
{
  axisData.turns += act;
  return (act * axisData.ratio * axisData.stepsPerRevolution);
}
