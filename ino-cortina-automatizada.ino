// Bibliotecas
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <DS1307.h>

// Macros
#define DEBUG 0

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

//PinOut - RTC
#define SDA A4
#define SCL A5

// Pseudo-classe para estruturação das variáveis
// e funções relacionadas ao controle do tempo
struct OperationData
{
  Time actualTime;
  Time openTime;
  Time closeTime;

  // Incrementa tempo passado no parâmetro como ponteiro
  // Faz as validações para manter dentro do formato 23:59:00
  void increaseTime(Time *t, int incMin)
  {
    t->min += incMin;
    if (t->min > 59)
    {
      t->min = 0;
      t->hour += 1;

      if (t->hour > 23)
      {
        t->hour = 0;
      }
    }
  }

  // Decrementa tempo passado no parâmetro como ponteiro
  // Faz as validações para manter dentro do formato 23:59:00
  void decreaseTime(Time *t, int decMin)
  {
    t->min -= decMin;
    if (t->min > 59)
    {
      t->min = 60 - decMin;
      t->hour -= 1;
      if (t->hour > 23)
      {
        t->hour = 23;
      }
    }
  }

  // Verifica se o tempo é diferente de zero, ou seja,
  // se foi inicializado
  bool isNotNull(Time t)
  {
    return (t.sec + t.min + t.hour) > 0;
  }

  // Verifica se tempo atual é igual ao tempo de abertura
  bool isTimeToOpen()
  {
    return isEqual(this->openTime);
  }

  // Verifica se o tempo atual é igual ao tempo de fechamento
  bool isTimeToClose()
  {
    return isEqual(this->closeTime);
  }

  // Operador de igualdade entre os tempos atual e passado via parâmetro
  bool isEqual(Time t)
  {
    bool second = this->actualTime.sec == t.sec;
    bool minute = this->actualTime.min == t.min;
    bool hour = this->actualTime.hour == t.hour;
    return second && minute && hour;
  }

  // Funcção de utilidade para conversão da classe Time para String
  char *time2str(Time t)
  {
    char *output = "xxxxxxxx";

    if (t.hour < 10)
      output[0] = 48;
    else
      output[0] = char((t.hour / 10) + 48);

    output[1] = char((t.hour % 10) + 48);
    output[2] = 58;

    if (t.min < 10)
      output[3] = 48;
    else
      output[3] = char((t.min / 10) + 48);

    output[4] = char((t.min % 10) + 48);
    output[5] = 58;

    if (t.sec < 10)
      output[6] = 48;
    else
      output[6] = char((t.sec / 10) + 48);

    output[7] = char((t.sec % 10) + 48);
    output[8] = 0;
    return output;
  }
} opData;

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
  MENU_ONE,
  MENU_TWO,
  END_STATE
} machineState;

// Declaração de bibliotecas externas
Stepper stepper = Stepper(axisData.stepsPerRevolution, M1, M3, M2, M4);
LiquidCrystal lcd = LiquidCrystal(RS, E, D4, D5, D6, D7);
DS1307 rtc = DS1307(SDA, SCL);

// Variáveis globais de controle de tempo do menu
const unsigned long updateWaitMs = 15000;
unsigned long lastScreenChangeMs;

void setup()
{
#if DEBUG
  // Inicializa porta serial com baud rate de 115200 bps
  Serial.begin(115200);
#endif

  // Inicialização dos horários de abertura e fechamento
  opData.openTime.hour = 22;
  opData.openTime.min = 0;

  opData.closeTime.hour = 21;
  opData.closeTime.min = 50;

  // Setup - LCD
  lcd.begin(16, 2);

  // Setup - Botões
  pinMode(BTN_ONE, INPUT_PULLUP);
  pinMode(BTN_TWO, INPUT_PULLUP);
  pinMode(BTN_THREE, INPUT_PULLUP);

  // Setup - Stepper
  pinMode(HOME_SWITCH, INPUT_PULLUP);
  stepper.setSpeed(500);

  // Setup - RTC
  rtc.halt(false);
  // rtc.setDOW(MONDAY);
  // rtc.setTime(22, 00, 0);
  // rtc.setDate(6, 7, 2020);
  rtc.setSQWRate(SQW_RATE_1);
  rtc.enableSQW(true);

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

  // Atualiza tempo atual na estrutura por requisição no módulo RTC
  opData.actualTime = rtc.getTime();

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
    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.print(opData.time2str(opData.actualTime));
    lcd.print("    ");

    if (buttonOne)
      if (!axisData.opened)
        machineState = OPEN_WINDOW;

    if (buttonTwo)
      if (!axisData.closed)
        machineState = CLOSE_WINDOW;

    if (buttonThree)
    {
      machineState = MENU_ONE;
      lastScreenChangeMs = millis();
      delay(250);
    }

    if (!axisData.opened)
      if (opData.isNotNull(opData.openTime))
        if (opData.isTimeToOpen())
          machineState = OPEN_WINDOW;

    if (!axisData.closed)
      if (opData.isNotNull(opData.closeTime))
        if (opData.isTimeToClose())
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

  case MENU_ONE:
    lcd.setCursor(0, 0);
    lcd.print("HORA ABERTURA   ");
    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.print(opData.time2str(opData.openTime));
    lcd.print("    ");

    if (buttonOne)
    {
      lastScreenChangeMs = millis();
      opData.increaseTime(&opData.openTime, 5);
      delay(250);
    }

    if (buttonTwo)
    {
      lastScreenChangeMs = millis();
      opData.decreaseTime(&opData.openTime, 5);
      delay(250);
    }

    if (buttonThree)
    {
      lastScreenChangeMs = millis();
      machineState = MENU_TWO;
      delay(250);
    }

    if (millis() - lastScreenChangeMs >= updateWaitMs)
      machineState = CHECK_BUTTTONS;

    break;

  case MENU_TWO:
    lcd.setCursor(0, 0);
    lcd.print("HORA FECHAMENTO ");
    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.print(opData.time2str(opData.closeTime));
    lcd.print("    ");

    if (buttonOne)
    {
      lastScreenChangeMs = millis();
      opData.increaseTime(&opData.closeTime, 5);
      delay(250);
    }

    if (buttonTwo)
    {
      lastScreenChangeMs = millis();
      opData.decreaseTime(&opData.closeTime, 5);
      delay(250);
    }

    if (buttonThree)
    {
      lastScreenChangeMs = millis();
      machineState = CHECK_BUTTTONS;
      delay(250);
    }

    if (millis() - lastScreenChangeMs >= updateWaitMs)
      machineState = CHECK_BUTTTONS;

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
  Serial.println(homeSwitchState);

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

  Serial.print("time: ");
  Serial.println(opData.time2str(opData.actualTime));
  Serial.print("openTime: ");
  Serial.println(opData.time2str(opData.openTime));
  Serial.print("closeTime: ");
  Serial.println(opData.time2str(opData.closeTime));
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
