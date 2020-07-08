// Bibliotecas
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

  // Setup - Stepper
  pinMode(HOME_SWITCH, INPUT_PULLUP);
  stepper.setSpeed(500);
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
    }
    else
    {
      stepper.step(-1 * axisData.stepsPerRevolution);
    }

    break;

  case CHECK_BUTTTONS:
    if (buttonOne)
      if (!axisData.opened)
        machineState = OPEN_WINDOW;

    if (buttonTwo)
      if (!axisData.closed)
        machineState = CLOSE_WINDOW;

    break;

  case OPEN_WINDOW:
    axisData.opened = true;
    axisData.closed = false;
    stepper.step(getAbsoluteSteps(0));
    machineState = END_STATE;
    break;

  case CLOSE_WINDOW:
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
