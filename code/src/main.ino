#define RESET_CLOCK 0     // сброс часов на время загрузки прошивки (для модуля с несъёмной батарейкой). Не забудь поставить 0 и прошить ещё раз!
#define SENS_TIME 10000   // время обновления показаний сенсоров на экране, миллисекунд


// управление яркостью
byte LED_BRIGHT = 10;         // при отсутствии сохранения в EEPROM: яркость светодиода СО2 (0 - 10) (коэффициент настраиваемой яркости индикатора по умолчанию, если нет сохранения и не автоматическая регулировка  )
byte LCD_BRIGHT = 10;         // при отсутствии сохранения в EEPROM: яркость экрана (0 - 10) (коэффициент настраиваемой яркости экрана по умолчанию, если нет сохранения и не автоматическая регулировка  )
byte LED_BRIGHT_local = LED_BRIGHT;
byte LCD_BRIGHT_local = LCD_BRIGHT;


#define BRIGHT_THRESHOLD 150  // величина сигнала, ниже которой яркость переключится на минимум (0-1023)
#define LED_BRIGHT_MAX 25    // макс яркость светодиода СО2 (0 - 255)
#define LED_BRIGHT_MIN 1      // мин яркость светодиода СО2 (0 - 255)
#define LCD_BRIGHT_MAX 25    // макс яркость подсветки дисплея (0 - 255)
#define LCD_BRIGHT_MIN 1      // мин яркость подсветки дисплея (0 - 255)


#define PRESSURE 0        // 0 - график давления, 1 - график прогноза дождя (вместо давления). Не забудь поправить пределы графика

#define DISPLAY_ADDR 0x27 // адрес платы дисплея

// пределы для индикатора  
#define maxCO2 1000       // и выше - красный
#define blinkLEDCO2 1500  // Значение СО2, при превышении которого будет мигать индикатор

#include <EEPROM.h>

// пределы отображения для графиков
#define TEMP_MIN 15
#define TEMP_MAX 35
#define HUM_MIN 0
#define HUM_MAX 100
#define PRESS_MIN 720
#define PRESS_MAX 760
#define CO2_MIN 400
#define CO2_MAX 2000

// пины
#define BACKLIGHT 10
#define PHOTO A3

#define MHZ_RX 2
#define MHZ_TX 3

#define LED_COM 7
#define LED_R 9
#define LED_G 6
#define LED_B 5
#define BTN_PIN 11 // D11
#define BTN_PIN2 4 // D4

#define IS_RED    1
#define IS_GREEN  2
#define IS_BLUE   4


// библиотеки
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(DISPLAY_ADDR, 20, 4);

#include "RTClib.h"
RTC_DS3231 rtc;
DateTime now;

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

#include <MHZ19_uart.h>
MHZ19_uart mhz19;

unsigned long sensorsTimer = SENS_TIME;
unsigned long drawSensorsTimer = SENS_TIME;
unsigned long clockTimer = 500;



unsigned long dayPlotTimer = ((long)1.6 * 60 * 60 * 1000);  // 1.6 часа


unsigned long plotTimer = dayPlotTimer;
unsigned long brightTimer = (2000);

unsigned long sensorsTimerD = 0;
unsigned long drawSensorsTimerD = 0;
unsigned long clockTimerD = 0;

unsigned long dayPlotTimerD = 0;
unsigned long plotTimerD = 0;
unsigned long predictTimerD = 0;
unsigned long brightTimerD = 0;

#include "GyverButton.h"
GButton  button(BTN_PIN,  HIGH_PULL, NORM_CLOSE);
GButton button_back(BTN_PIN2, HIGH_PULL, NORM_OPEN);

int8_t hrs, mins, secs, month, day;
int8_t local_hrs, local_mins;

uint16_t year;
byte mode = 0;
/*
  0 часы и данные
  1 график углекислого за сутки
  2 график влажности за сутки
  3 график температуры за сутки
  4 график давления за сутки
*/

byte podMode = 1; // подрежим меню 
byte mode0scr = 0;

/*  
  0 - Крупно время
  1 - Крупно содержание СО2
  2 - Крупно температура
  3 - Крупно давление
  4 - Крупно влажность
*/

// переменные для вывода
float dispTemp;
byte dispHum;
int dispPres;
int dispCO2 = -1;



// массивы графиков
int tempDay[15];
int humDay[15];
int pressDay[15];
int co2Day[15];

int delta;
uint32_t pressure_array[6];
uint32_t sumX, sumY, sumX2, sumXY;
float a, b;

/*
  Характеристики датчика BME:
  Температура: от-40 до + 85 °C
  Влажность: 0-100%
  Давление: 300-1100 hPa (225-825 ммРтСт)
  Разрешение:
  Температура: 0,01 °C
  Влажность: 0.008%
  Давление: 0,18 Pa
  Точность:
  Температура: +-1 °C
  Влажность: +-3%
  Давление: +-1 Па
*/

// символы
// график
byte rowS[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b10001,  0b01010,  0b00100,  0b00000};   // стрелка вниз  
byte row7[8] = {0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row6[8] = {0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row5[8] = {0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};  
byte row4[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111};
byte row3[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};   // в т.ч. для двустрочной цифры 0 
byte row2[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};   // в т.ч. для двустрочной цифры 4  
byte row1[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111};

// цифры //   
uint8_t UB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};   // для двустрочных 7, 0   
uint8_t UMB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};  // для двустрочных 2, 3, 5, 6, 8, 9
uint8_t LMB[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};  // для двустрочных 2, 3, 5, 6, 8, 9
uint8_t LM2[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};  // для двустрочной 4


uint8_t KU[8] = {0b00000,  0b00000,  0b00000,  0b00001,  0b00010,  0b00100,  0b01000,  0b10000};   // для верхней части %
uint8_t KD[8] = {0b00001,  0b00010,  0b00100,  0b01000,  0b10000,  0b00000,  0b00000,  0b00000};   // для нижней части %


void digSeg(byte x, byte y, byte z1, byte z2, byte z3, byte z4, byte z5, byte z6) {   // отображение двух строк по три символа с указанием кодов символов  
  lcd.setCursor(x, y);
  lcd.write(z1); lcd.write(z2); lcd.write(z3);
  if (x <= 11) lcd.print(" ");
  lcd.setCursor(x, y + 1);
  lcd.write(z4); lcd.write(z5); lcd.write(z6);
  if (x <= 11) lcd.print(" ");
}

void drawDig(byte dig, byte x, byte y) {        // рисуем цифры   
  switch (dig) {            // двухстрочные цифры
    case 0:
      digSeg(x, y, 255, 1, 255, 255, 2, 255);
      break;
    case 1:
      digSeg(x, y, 32, 255, 32, 32, 255, 32);
      break;
    case 2:
      digSeg(x, y, 3, 3, 255, 255, 4, 4);
      break;
    case 3:
      digSeg(x, y, 3, 3, 255, 4, 4, 255);
      break;
    case 4:
      digSeg(x, y, 255, 0, 255, 5, 5, 255);
      break;
    case 5:
      digSeg(x, y, 255, 3, 3, 4, 4, 255);
      break;
    case 6:
      digSeg(x, y, 255, 3, 3, 255, 4, 255);
      break;
    case 7:
      digSeg(x, y, 1, 1, 255, 32, 255, 32);
      break;
    case 8:
      digSeg(x, y, 255, 3, 255, 255, 4, 255);
      break;
    case 9:
      digSeg(x, y, 255, 3, 255, 4, 4, 255);
      break;
    case 10:
      digSeg(x, y, 32, 32, 32, 32, 32, 32);
      break;
  } 
}

void drawPPM(int dispCO2, byte x, byte y) {     // Уровень СО2 крупно на главном экране   ----------------------------
  if (dispCO2 / 1000 == 0) drawDig(10, x, y);
  else drawDig(dispCO2 / 1000, x, y);
  drawDig((dispCO2 % 1000) / 100, x + 4, y);
  drawDig((dispCO2 % 100) / 10, x + 8, y);
  drawDig(dispCO2 % 10 , x + 12, y);
  lcd.setCursor(15, 0);

  lcd.print("ppm");
}

void drawPres(int dispPres, byte x, byte y) {   // Давление крупно на главном экране   ----------------------------
  drawDig((dispPres % 1000) / 100, x , y);
  drawDig((dispPres % 100) / 10, x + 4, y);
  drawDig(dispPres % 10 , x + 8, y);
  lcd.setCursor(x + 11, 1);
  
  lcd.print("mm");
}


void drawTemp(float dispTemp, byte x, byte y) { // Температура крупно на главном экране   ----------------------------
  if (dispTemp / 10 == 0) drawDig(10, x, y);
  else drawDig(dispTemp / 10, x, y);
  drawDig(int(dispTemp) % 10, x + 4, y);
  drawDig(int(dispTemp * 10.0) % 10, x + 9, y);

  lcd.setCursor(x + 7, y + 1);
  lcd.write(0B10100001);    // десятичная точка

  lcd.setCursor(x + 13, y);
  lcd.write(223);             // градусы
}

void drawHum(int dispHum, byte x, byte y) {   // Влажность крупно на главном экране   ----------------------------
  if (dispHum / 100 == 0) drawDig(10, x, y);
  else drawDig(dispHum / 100, x, y);
  if ((dispHum % 100) / 10 == 0) drawDig(0, x + 4, y);
  else drawDig(dispHum / 10, x + 4, y);
  drawDig(int(dispHum) % 10, x + 8, y);
  
  lcd.setCursor(x + 12, y + 1);
  lcd.print("%");
}

void drawClock(byte hours, byte minutes, byte x, byte y) {    // рисуем время крупными цифрами -------------------------------------------
  if (hours > 23 || minutes > 59) return;
  if (hours / 10 == 0) drawDig(10, x, y);
  else drawDig(hours / 10, x, y);
  drawDig(hours % 10, x + 4, y);
  drawDig(minutes / 10, x + 8, y);
  drawDig(minutes % 10, x + 12, y);
}


static const char *dayNames[]  = {
  "Su",
  "Mo",
  "Tu",
  "We",
  "Th",
  "Fr",
  "Sa",
};


void drawData() {                     // выводим дату -------------------------------------------------------------
  int Y = 0;
  if (mode0scr == 1) Y = 2;
              
  lcd.setCursor(15, 0 + Y);
  if (now.day() < 10) lcd.print(0);
  lcd.print(now.day());
  lcd.print(".");
  if (now.month() < 10) lcd.print(0);
  lcd.print(now.month());


  loadClock();              // принудительно обновляем знаки, т.к. возможно необновление знаков в днях недели  
  lcd.setCursor(18, 1);
  int dayofweek = now.dayOfTheWeek();
  lcd.print(dayNames[dayofweek]);
}


void drawPlot(byte pos, byte row, byte width, byte height, int min_val, int max_val, int *plot_array, String label1, String label2, int stretch) {  // график ---------------------------------
  int max_value = -32000;
  int min_value = 32000;

  for (byte i = 0; i < 15; i++) {
    max_value = max(plot_array[i] , max_value);
    min_value = min(plot_array[i] , min_value);
  }

  //рисуем указатель пределов (стрелочки вверх-вниз)  
  lcd.setCursor(15, 0);

  lcd.write(0);
  lcd.setCursor(15, 3);
  lcd.write(0b01011110);

  if (min_val >= max_val) max_val = min_val + 1;

  lcd.setCursor(15, 1); lcd.write(0b01111100);
  lcd.setCursor(15, 2); lcd.write(0b01111100);

  lcd.setCursor(16, 0); lcd.print(max_value);
  lcd.setCursor(16, 1); lcd.print(label1); lcd.print(label2);
  lcd.setCursor(16, 2); lcd.print(plot_array[14]);
  lcd.setCursor(16, 3); lcd.print(min_value);

  for (byte i = 0; i < width; i++) {                  // каждый столбец параметров
    int fill_val = plot_array[i];
    fill_val = constrain(fill_val, min_val, max_val);
    byte infill, fract;
    // найти количество целых блоков с учётом минимума и максимума для отображения на графике
    if ((plot_array[i]) > min_val)
      infill = floor((float)(plot_array[i] - min_val) / (max_val - min_val) * height * 10);
    else infill = 0;
    fract = (float)(infill % 10) * 8 / 10;            // найти количество оставшихся полосок
    infill = infill / 10;

    for (byte n = 0; n < height; n++) {     // для всех строк графика
      if (n < infill && infill > 0) {       // пока мы ниже уровня
        lcd.setCursor(i, (row - n));        // заполняем полными ячейками
        lcd.write(255);
      }
      if (n >= infill) {                    // если достигли уровня
        lcd.setCursor(i, (row - n));
        if (n == 0 && fract == 0) fract++;      // если нижний перел графика имеет минимальное значение, то рисуем одну полоску, чтобы не было пропусков  
        if (fract > 0) lcd.write(fract);        // заполняем дробные ячейки
        else lcd.write(16);                     // если дробные == 0, заливаем пустой
        for (byte k = n + 1; k < height; k++) { // всё что сверху заливаем пустыми
          lcd.setCursor(i, (row - k));
          lcd.write(16);
        }
        break;
      }
    }
  }
}

void loadClock() {
  lcd.createChar(0, row2);
  lcd.createChar(1, UB);
  lcd.createChar(2, row3);
  lcd.createChar(3, UMB);
  lcd.createChar(4, LMB);
  lcd.createChar(5, LM2);
}

void loadPlot() {
  lcd.createChar(0, rowS);      // Стрелка вниз для индикатора пределов  
  lcd.createChar(1, row1);
  lcd.createChar(2, row2);
  lcd.createChar(3, row3);
  lcd.createChar(4, row4);
  lcd.createChar(5, row5);
  lcd.createChar(6, row6);
  lcd.createChar(7, row7);
}


byte LED_ON = (LED_BRIGHT_MAX);
byte LED_OFF = (0);


void setLEDcolor(byte color) {                    // цвет индикатора задается двумя битами на каждый цвет  
  analogWrite(LED_R, LED_ON *  (color == IS_RED));
  analogWrite(LED_G, LED_ON *  (color == IS_GREEN));
  analogWrite(LED_B, LED_ON *  (color == IS_BLUE));
}

void setLED() {
  if (LED_BRIGHT_local < 11) {                                     // если ручные установки яркости
    LED_ON = 255 / 100 *  LED_BRIGHT_local * LED_BRIGHT_local;
  } else {
    checkBrightness();
  }
  
  // ниже задается цвет индикатора в зависимости от показаний датчика CO2: красный, зеленый

  if (dispCO2 >= maxCO2) setLEDcolor(IS_RED);   // красный
  else setLEDcolor(IS_GREEN);    // зеленый
}

void setup() {
  Serial.begin(9600);

  pinMode(BACKLIGHT, OUTPUT);
  pinMode(LED_COM, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setLEDcolor(0);

  digitalWrite(LED_COM, 0);
  analogWrite(BACKLIGHT, LCD_BRIGHT_MAX);

  if (EEPROM.read(0) == 122) {      // если было сохранение настроек, то восстанавливаем их  
    mode0scr = EEPROM.read(6);
    LED_BRIGHT = EEPROM.read(8);
    LCD_BRIGHT = EEPROM.read(9);  
    LED_BRIGHT_local = LED_BRIGHT;
    LCD_BRIGHT_local = LCD_BRIGHT;
  }

  lcd.init();
  lcd.backlight();
  lcd.clear();

  mhz19.begin(MHZ_RX, MHZ_TX);
  mhz19.setAutoCalibration(false);

  rtc.begin();
  bme.begin(&Wire);


  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF);

  if (RESET_CLOCK || rtc.lostPower())
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  lcd.clear();
  now = rtc.now();


  secs = now.second();  
  mins = now.minute();
  hrs  = now.hour();

  local_hrs = hrs; 
  local_mins = mins;


  bme.takeForcedMeasurement();
  uint32_t Pressure = bme.readPressure();
  for (byte i = 0; i < 6; i++) {          // счётчик от 0 до 5
    pressure_array[i] = Pressure; // забить весь массив текущим давлением    
  }


  // заполняем графики текущим значением
  readSensors();
  for (byte i = 0; i < 15; i++) {   // счётчик от 0 до 14
    tempDay[i] = dispTemp;
    humDay[i] = dispHum;
    if (PRESSURE) {
      pressDay[i] = 0;
    } else {
      pressDay[i] = dispPres;
    }
  }

  drawData();
  loadClock();
  drawSensors();
}

void loop() {
  if (testTimer(brightTimerD, brightTimer)) checkBrightness();  // яркость
  if (testTimer(sensorsTimerD, sensorsTimer)) readSensors();    // читаем показания датчиков с периодом SENS_TIME

  if (testTimer(clockTimerD, clockTimer)) clockTick();          // два раза в секунду пересчитываем время и мигаем точками
  plotSensorsTick();                                // тут внутри несколько таймеров для пересчёта графиков 
  modesTick();                                      // тут ловим нажатия на кнопку и переключаем режимы
  if (mode == 0) {                                  // в режиме "главного экрана"
    if (testTimer(drawSensorsTimerD, drawSensorsTimer)) drawSensors();  // обновляем показания датчиков на дисплее с периодом SENS_TIME
  } else {                                          // в любом из графиков
    if (testTimer(plotTimerD, plotTimer)) redrawPlot();  // перерисовываем график
  }
}
