#include <Looper.h>
#include <GyverIO.h>

#include <GyverDS3231Min.h>
GyverDS3231Min rtc;

#include "AsyncStream.h"  // асинхронное чтение сериал
AsyncStream<50> serial(&Serial, ';');   // указываем обработчик и стоп символ

// Концевики отбора
#define CONCEVIC1 4 // D4
boolean ccv1 = false;
#define CONCEVIC2  5 // D5
boolean ccv2 = false;

// Датчики температуры
#include <GyverDS18.h>
// T1 - датчик отбора, датчик старт/стопа
#define T1_PIN 6 // D6
GyverDS18Single tds1(T1_PIN);  // пин
float t1;
// T2 - датчик температуры куба 
#define T2_PIN 7 // D7
GyverDS18Single tds2(T2_PIN);  // пин
float t2;

// Управление регулятором мощности РМЦ-3-3500
boolean W_on = false;
boolean W_stop = false;
uint16_t W_pwm_t = 1500;
#define W_pwm_T 1500 // Период ШИМ

// Реле отбора
#define RELE_OTBORA1_PIN 14 // A0
bool ro1_on = false, ro1_state = false;
#define RELE_OTBORA2_PIN 15 // A1
bool ro2_on = false, ro2_state = false;

// ШИМ для управления РМЦ-3-3500
LP_THREAD({
  // ждем запуска управлением мощности
  LP_WAIT((W_on && !W_stop && !(W_pwm_t == W_pwm_T || W_pwm_t == 0)));  

  //Serial.print("Set W_on: ");
  //Serial.println(W_on);

  //if (!W_stop && !(W_pwm_t == W_pwm_T || W_pwm_t == 0)) {
    //Serial.println("PWMflagUP");
    LP_DELAY(W_pwm_t);
    //Serial.println("PWMflagDOWN");
    LP_DELAY(W_pwm_T - W_pwm_t);
  //}
  //flag = 0;
});

// Отправка данных в Serial
LP_TIMER(1000, []() {

  ro1_state = gio::read(RELE_OTBORA1_PIN);
  ro2_state = gio::read(RELE_OTBORA2_PIN);

  Serial.print("{");
  Serial.print("t1:");
  Serial.print(t1);
  Serial.print(";t2:");
  Serial.print(t2);

  Serial.print(";r01:");
  Serial.print(ro1_state);
  Serial.print(";r02:");
  Serial.print(ro2_state);
  
  Serial.print(";c1:");
  Serial.print(ccv1);
  Serial.print(";c2:");
  Serial.print(ccv2);

  Serial.print(";d:");
  Serial.print(rtc.getTime().toString());
  Serial.println("}");
});

// Функция тестирования регулятора мощности РМЦ-3-3500
LP_TIMER(10000, []() {
  W_pwm_t += 200;
  if (W_pwm_t > W_pwm_T) {
    W_pwm_t = 100;
  }
  //Serial.print("Set W_pwm_t: ");
  //Serial.println(W_pwm_t);
});

// Чтение датчиков температуры
LP_TIMER(900, []() {
  // чтение 1
  if (tds1.readTemp()) {
    t1 = tds1.getTemp();
  } else {
    t1 = 0;
  }

  // запрос 1
  if (!tds1.requestTemp()) {
    t1 = 0;
  }

  // чтение 2
  if (tds2.readTemp()) {
    t2 = tds2.getTemp();
  } else {
    t2 = 0;
  }

  // запрос
  if (!tds2.requestTemp()) {
    t2 = 0;
  }
});

// Чтение концевиков
LP_TIMER(100, []() {
  uint8_t ccv1_ = gio::read(CONCEVIC1);
  uint8_t ccv2_ = gio::read(CONCEVIC2);
  //if (ccv1 != ccv1_) {
  //  Serial.print("Концевик 1: ");
  //  Serial.println(ccv1_);
  //}

  //if (ccv2 != ccv2_) {
  //  Serial.print("Концевик 2: ");
  //  Serial.println(ccv2_);
  //}
  
  ccv1 = (ccv1_ == 1); 
  ccv2 = (ccv2_ == 1);

  // ВРЕМЕННО! Включение выключение регулятора мощности
  if (ccv1) W_on = 1;
  if (ccv2) W_on = 0;
});

void setup() {
  Serial.begin(9600);
  serial.setTimeout(100);

  // Часы
  setStampZone(3);  // часовой пояс
  Wire.begin();
  rtc.begin();

  // Реле отбора
  //pinMode(RELE_OTBORA1_PIN, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(14, OUTPUT);
  //pinMode(RELE_OTBORA2_PIN, OUTPUT);
  
  // Инициализация датчиков температуры
  tds1.setParasite(1);
  tds1.setResolution(12);
  tds2.setParasite(1);
  tds2.setResolution(12);
}

void loop() {
  if (serial.available()) {     // если данные получены
    Serial.println(serial.buf); // выводим их (как char*)    
  }

  ccv1 = gio::read(CONCEVIC1);
  ccv2 = gio::read(CONCEVIC2);

  ro1_state = gio::read(RELE_OTBORA1_PIN);
  ro2_state = gio::read(RELE_OTBORA2_PIN);

  //if (ccv2 == 1) Serial.println("DONE");
  ro1_on = ccv1;
  if (!ro1_state && ro1_on) {gio::write(RELE_OTBORA1_PIN, 1);ro1_state = true;}
  if (ro1_state && !ro1_on) {gio::write(RELE_OTBORA1_PIN, 0);ro1_state = true;}
  
  ro2_on = ccv2;
  if (!ro2_state && ro2_on) {gio::write(RELE_OTBORA2_PIN, 1);ro2_state = true;}
  if (ro2_state && !ro2_on) {gio::write(RELE_OTBORA2_PIN, 0);ro2_state = true;}

  Looper.loop();
}

