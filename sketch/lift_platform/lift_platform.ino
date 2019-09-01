// Моторы движения платформы
#define MOTOR_LEFT_PIN_1       10
#define MOTOR_LEFT_PIN_2       11
#define MOTOR_RIGHT_PIN_1      5
#define MOTOR_RIGHT_PIN_2      6

// Мотор подъемника
#define MOTOR_LIFT_PIN_1       3
#define MOTOR_LIFT_PIN_2       9

// Концевик нижнего положения платформы
#define MOTOR_BUT_PIN_DOWN     20

// Время подъема платформы (ограничение)
#define TIME_UP_FOR_MOTOR_MS   15000
// Время до отключения моторов платформы (для управления движением платформы)
#define TIME_MOTOR_MOVE_MS     250
// Значение ШИМ подаваемое на драйверы моторов
#define MOTOR_VALUE            254

// Названия команд для консоли
#define MOVE_FRONT                     '1'
#define MOVE_REAR                      '2'
#define MOVE_LEFT                      '3'
#define MOVE_RIGHT                     '4'
#define MOVE_STOP                      '0'

#define MOVE_LIFT_UP                   '5'
#define MOVE_LIFT_DOWN                 '6'
#define MOVE_LIFT_STOP                 '7'

// Состояние подъеминка
volatile bool down = false;
volatile bool up = false;
// Состояние нажатия команды поднять платформу вверх
bool push_up = false;
// Время нажатия на кнопку для ограничении подъема по времени
long time_last;
// Состояние команды подъемника
int state_lift = MOVE_LIFT_STOP;

void setup()
{
  // Подключаем второй последовательный порт (управление через блютус)
  Serial2.begin(9600);

  // Инициализация выводов на драйверы моторов
  pinMode (MOTOR_RIGHT_PIN_1, OUTPUT);
  pinMode (MOTOR_RIGHT_PIN_2, OUTPUT);
  pinMode (MOTOR_LEFT_PIN_1, OUTPUT);
  pinMode (MOTOR_LEFT_PIN_2, OUTPUT);
  pinMode (MOTOR_LIFT_PIN_1, OUTPUT);
  pinMode (MOTOR_LIFT_PIN_2, OUTPUT);

  // Инициализация выводов на питание для драйверов (!!осторожно костыль!!)
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  digitalWrite(7, HIGH);
  digitalWrite(8, HIGH);

  // Инициализация концевика нижнего положения подъемника и проверка его текущего состояния, затем инициализация прерываний
  pinMode (MOTOR_BUT_PIN_DOWN, INPUT_PULLUP);
  if(!digitalRead(MOTOR_BUT_PIN_DOWN))
  {
    butLiftDown();
  }
  attachInterrupt(digitalPinToInterrupt(MOTOR_BUT_PIN_DOWN), butLiftDown, FALLING);
}


void loop()
{
  // Проверяем, поступают ли какие-то команды
  if (Serial2.available())
  {
    // Проверяем какая команда поступила
    switch (Serial2.read())
    {
      // Выполняем действия с номером поступившей командой
      case MOVE_FRONT:
        motor(MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, MOTOR_VALUE);
        motor(MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, MOTOR_VALUE);
        stop_motors();
        break;
      case MOVE_REAR:
        motor(MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, -MOTOR_VALUE);
        motor(MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, -MOTOR_VALUE);
        stop_motors();
        break;
      case MOVE_RIGHT:
        motor(MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, -MOTOR_VALUE);
        motor(MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, MOTOR_VALUE);
        stop_motors();
        break;
      case MOVE_LEFT:
        motor(MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, MOTOR_VALUE);
        motor(MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, -MOTOR_VALUE);
        stop_motors();
        break;
      case MOVE_STOP:
        motor(MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, 0);
        motor(MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, 0);
        break;
      case MOVE_LIFT_UP:
        state_lift = MOVE_LIFT_UP;
        if(!push_up)
        {
          push_up = true;
          time_last = millis();
        }
        break;
      case MOVE_LIFT_DOWN:
        state_lift = MOVE_LIFT_DOWN;
        break;
      case MOVE_LIFT_STOP:
        motor(MOTOR_LIFT_PIN_1, MOTOR_LIFT_PIN_2, 0);
        break;
    }
  }
  // Управление подъемником
  move_lift();
}

// Функция остановки моторов после задержки
void stop_motors()
{
   delay(TIME_MOTOR_MOVE_MS);
   motor(MOTOR_LEFT_PIN_1, MOTOR_LEFT_PIN_2, 0);
   motor(MOTOR_RIGHT_PIN_1, MOTOR_RIGHT_PIN_2, 0);
}

// Функция управления мотором подъемника
void move_lift()
{
  // Команда вверх?
  if (state_lift == MOVE_LIFT_UP)
  {
    // Не верхнее положение платформы?
    if (!up)
    {
      // Время от нажатия кнопки меньше ограничения и состояние кнопки (команды вверх) действует?
      if (millis() - time_last <= TIME_UP_FOR_MOTOR_MS && push_up)
      {
        // Поднимаем платформу и сбрасываем состояние нижнего положения платформы 
        motor(MOTOR_LIFT_PIN_1, MOTOR_LIFT_PIN_2, MOTOR_VALUE);
        down = false;
      }
      else
      {
        // фиксируем состояние подъемника в верхнем положении
        up = true;  
      }
    }
    else
    {
      // Останавливаем подъемник, фиксируем его состояние и сбрасываем состояние нажания кнопки подъемника вверх
      motor(MOTOR_LIFT_PIN_1, MOTOR_LIFT_PIN_2, 0);
      state_lift = MOVE_LIFT_STOP;
      push_up = false;
    }
  }
  else
  {
    // Команда вниз?
    if (state_lift == MOVE_LIFT_DOWN)
    {
      // Не нижнее положение платформы?
      if (!down)
      {
        // Опускаем подъемник
        motor(MOTOR_LIFT_PIN_1, MOTOR_LIFT_PIN_2, -MOTOR_VALUE);
      }
      else
      {
        // Останавливаем подъемник, фиксируем его состояние и сбрасываем состояние подъемника
        motor(MOTOR_LIFT_PIN_1, MOTOR_LIFT_PIN_2, 0);
        state_lift = MOVE_LIFT_STOP;
        up = false;
      }
    }
  }
}

// Функция (обработчик прерываний) состояний концевика нижнего положения платформы
void butLiftDown()
{
  down = true;
}

// Функция управления моторами (Драйвер L298N)
void motor(int pin_1, int pin_2, int value)
{
  if (value == 0)
  {
    analogWrite (pin_1, 0);
    analogWrite (pin_2, 0);
  }
  if (value > 0)
  {
    analogWrite (pin_1, abs(value));
    analogWrite (pin_2, 0);
  }
  if (value < 0)
  {
    analogWrite (pin_1, 0);
    analogWrite (pin_2, abs(value));
  }
}
