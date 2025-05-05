void checkBrightness() {
  Serial.println(analogRead(PHOTO));
  if (LCD_BRIGHT_local == 11) {                         // если установлен автоматический режим для экрана  
    if (analogRead(PHOTO) < BRIGHT_THRESHOLD) {   // если темно
      analogWrite(BACKLIGHT, LCD_BRIGHT_MIN);
    } else {                                      // если светло
      analogWrite(BACKLIGHT, LCD_BRIGHT_MAX);
    }
  } else {
    analogWrite(BACKLIGHT, LCD_BRIGHT_local * LCD_BRIGHT_local * 2.5);
  }

  if (LED_BRIGHT_local == 11) {                         // если установлен автоматический режим для индикатора  
    if (analogRead(PHOTO) < BRIGHT_THRESHOLD) {   // если темно
      LED_ON = (LED_BRIGHT_MIN);

    } else {                                      // если светло
      LED_ON = (LED_BRIGHT_MAX);
    }
  }
}

/*
  mode:
  0 - Главный экран (Время, CO2, Влажность, Температура)
  1-3 - Графики: СО2, Влажность, Температура
  252 - настройка времени
  253 - настройка яркости экрана (podMode от 0 до 11: выбор от 0% до 100% или автоматическое регулирование)
  254 - настройка яркости индикатора (podMode от 0 до 11: выбор от 0% до 100% или автоматическое регулирование)
  255 - главное меню (podMode от 1 до 3: 1 - Ярк.индикатора, 2 - Ярк.экрана, 3 - Изменение времени)                              
*/

byte last_mode = 0;
byte last_podMode = 0;
byte last_mode0scr = 0;


void modesTick() {
  // отладка перемещения по окнам 
  if (last_mode != mode || last_podMode != podMode || last_mode0scr != mode0scr) {
    Serial.print("mode ");
    Serial.println(mode);
    
    Serial.print("podMode ");
    Serial.println(podMode);

    Serial.print("mode0scr ");
    Serial.println(mode0scr);

    last_mode = mode;
    last_podMode = podMode;
    last_mode0scr = mode0scr;
  }

  button.tick();
  button_back.tick();
  boolean changeFlag = false;
  if (button.isSingle()) {    // одинарное нажатие на кнопку
    Serial.println("Single button");
    
    if (mode >= 240) {
      podMode++;
      switch (mode) {

        case 252:
          if (podMode > 1) podMode = 0;
          changeFlag = true;
          break;

        case 253:             // Перебираем все варианты яркости LCD экрана  
          if (podMode > 11) podMode = 0;
          LCD_BRIGHT_local = podMode;
          checkBrightness();
          changeFlag = true;
          break;

        case 254:             // Перебираем все варианты яркости LED индикатора  
          if (podMode > 11) podMode = 0;
          LED_BRIGHT_local = podMode;
          changeFlag = true;
          break;

        case 255:             // Перебираем все варианты основных настроек  
          if (podMode > 3) podMode = 1;
          changeFlag = true;
          break;
      }
    } else if (mode == 0)  {
      mode0scr++;
        
      if (mode0scr > 4) mode0scr = 0;         // Переключение режима работы главного экрана  
       
      changeFlag = true;
    } else if (mode > 0 && mode < 100) {
      mode++;

      if (mode > 4) mode = 1;

      changeFlag = true;
    } else if (mode == 100) {    // выход с сохранением
      Serial.println("Saving...");
      mode = 0;
      podMode = 0;
      mode0scr = 0;
      changeFlag = true;

      LED_BRIGHT = LED_BRIGHT_local;
      LCD_BRIGHT = LCD_BRIGHT_local;

      if (EEPROM.read(6) != mode0scr) EEPROM.write(6, mode0scr);
      if (EEPROM.read(8) != LED_BRIGHT) EEPROM.write(8, LED_BRIGHT);
      if (EEPROM.read(9) != LCD_BRIGHT) EEPROM.write(9, LCD_BRIGHT);
      if (EEPROM.read(0) != 122) EEPROM.write(0, 122);

      now   = rtc.now();
      secs  = now.second();
      mins  = local_mins;
      hrs   = local_hrs;
      month = now.month();
      year  = now.year();
      day   = now.day();
      rtc.adjust(DateTime(year, month, day, hrs, mins, secs));
      now = rtc.now();
      secs  = now.second();
      month = now.month();
      year  = now.year();
      day   = now.day();
      hrs   = now.hour();
      mins  = now.minute();
    }  
  }

  if (button_back.isSingle()) {    // одинарное нажатие на кнопку
    Serial.println("Single button_back");
    if (mode >= 253) {

      switch (mode) {
        case 253:             // Перебираем все варианты яркости LCD экрана  
          if (podMode == 0)  podMode = 11;
          else               podMode--;
          LCD_BRIGHT_local = podMode;
          checkBrightness();
          changeFlag = true;
          break;

        case 254:             // Перебираем все варианты яркости LED индикатора  
          if (podMode == 0)  podMode = 11;
          else               podMode--;
          LED_BRIGHT_local = podMode;
          changeFlag = true;
          break;

        case 255:             // Перебираем все варианты основных настроек  
          podMode--;
          if (podMode <= 0) podMode = 3;
          changeFlag = true;    
          break;
      }
    }
    else if (mode == 0)
    {
      if (mode0scr <= 0)  mode0scr = 4;
      else                mode0scr--;
      changeFlag = true;
    }
    else if (mode < 100)
    {
      mode--;
      if (mode == 0) mode = 4;
      changeFlag = true;
    }
    else if (mode == 100)
    {
      mode = 0;
      podMode = 0;
      mode0scr = 0;
      changeFlag = true;
      local_hrs = hrs;
      local_mins = mins;
      LED_BRIGHT_local = LED_BRIGHT;
      LCD_BRIGHT_local = LCD_BRIGHT;
    }
    else if (mode == 252)
    {
      if (podMode == 0) {
        local_hrs++;
        if (hrs >= 24) hrs = 0;     
      } else {
        local_mins++;
        if (mins >= 60) mins = 0;
      }
      
      changeFlag = true;
    }
  }

  if (button.isDouble()) {                    // двойное нажатие   ----------------------------
    Serial.println("Double button");

    switch (mode) {

      case 0:
        mode = mode0scr;
        changeFlag = true;
        break;

      case 255:       // главное меню
        if (podMode <= 3) mode = 255 - podMode;   // если настройки яркостей, то переключаемся в настройки пункта меню   

        if (mode == 254) podMode = LED_BRIGHT_local;  // если выбрана яркость LED - устанавливаем текущее показание  
        if (mode == 253) podMode = LCD_BRIGHT_local;  // если выбрана яркость LCD - устанавливаем текущее показание  
        if (mode == 252) podMode = 0;
        changeFlag = true;
        break;
    }
  }

  if (button_back.isDouble()) // двойное нажатие 
  {
    Serial.println("Double button_back");
    if (mode < 100) 
    {
      mode0scr   = mode;
      mode       = 0;
      changeFlag = true;
    }

    switch (mode)
    {
      case 252:
        mode = 255;
        podMode = 3;
        changeFlag = true;
        break;
      case 253:       // ярк. экрана
        mode = 255;
        podMode = 2;
        changeFlag = true;
        break;
      case 254:       // ярк. индикатора
        mode = 255;
        podMode = 1;
        changeFlag = true;
        break;
    }    
  }

  if ((button.isHolded()) && (button_back.isHolded())) {  // удержание двух кнопок - переход в меню настроек или из меню настроек в окно сохранения
    Serial.println("button_back button holded");
    if (mode < 100)
    {        
      mode = 255;
      podMode = 1;
      changeFlag = true;
    }
    else if (mode >= 240)
    {
      mode       = 100;
      changeFlag = true;
    }
  }


  if (changeFlag) {
    if (mode >= 240) {
      lcd.clear();
      lcd.setCursor(0, 0);
    }
    if (mode == 255) {          // Перебираем варианты в главном меню  

      lcd.print("Setup:");

      lcd.setCursor(0, 1);
      switch (podMode) {
        case 1:
          lcd.print("LED brightness");
          break;
        case 2:
          lcd.print("LCD brightness");
          break;
        case 3:
          lcd.print("Set time");
          break;
      }
    }
    if (mode == 252)                        // --------------------- показать "Время"
    {
      lcd.setCursor(0, 0);
      lcd.print("Set time");
      lcd.setCursor(0, 1);

      if (podMode == 0)   lcd.print("hours");
      else                lcd.print("minutes");

      lcd.setCursor(0, 2);
      lcd.print(local_hrs);
      lcd.print(":");
      lcd.print(local_mins);

    }
    if (mode == 253) {                        // --------------------- показать  "Ярк.экрана"

      lcd.print("LCD Brightness:");

      if (LCD_BRIGHT_local == 11) lcd.print("Auto ");
      else                  lcd.print(String(LCD_BRIGHT_local * 10) + "%");

    }
    if (mode == 254) {                        // --------------------- показать  "Ярк.индикатора"
      lcd.print("LED Brightness:");

      if (LED_BRIGHT_local == 11) lcd.print("Auto ");
      else                  lcd.print(String(LED_BRIGHT_local * 10) + "%");

    }
    if (mode == 100)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("right button - exit");
      lcd.setCursor(0, 1);
      lcd.print("with save,");
      lcd.setCursor(0, 2);
      lcd.print("left button - exit");
      lcd.setCursor(0, 3);
      lcd.print("without save");
      lcd.setCursor(0, 4);
    }

    if (mode == 0) {
      lcd.clear();
      loadClock();
      drawSensors();
      drawData();
    } else if (mode <= 10) {
      loadPlot();
      redrawPlot();
    }
  }
}

void redrawPlot() {
  lcd.clear();
  switch (mode) {             

    case 1: drawPlot(0, 3, 15, 4, CO2_MIN, CO2_MAX, (int*)co2Day, "c ", "da", mode);
      break;
    case 2: drawPlot(0, 3, 15, 4, TEMP_MIN, TEMP_MAX, (int*)tempDay, "t\337", "da", mode);
      break;
    case 3: drawPlot(0, 3, 15, 4, PRESS_MIN, PRESS_MAX, (int*)pressDay, "p ", "da", mode);
      break;
    case 4: drawPlot(0, 3, 15, 4, HUM_MIN, HUM_MAX, (int*)humDay, "h%", "da", mode);
      break;
  }
}

void readSensors() {
  bme.takeForcedMeasurement();
  dispTemp = bme.readTemperature();
  dispHum = bme.readHumidity();
  dispPres = (float)bme.readPressure() * 0.00750062;
  dispCO2 = mhz19.getPPM();
}

void drawSensors() {
  // дисплей 20 x 4

  if (mode0scr != 2) {                        // Температура   ----------------------------
    lcd.setCursor(0, 2);
    lcd.print(String(dispTemp, 1));
    lcd.write(223);

  } else {
    drawTemp(dispTemp, 0, 0);
  }

  if (mode0scr != 4) {                        // Влажность   ----------------------------
    lcd.setCursor(5, 2);
    lcd.print(" " + String(dispHum) + "% ");

  } else {
    drawHum(dispHum, 0, 0);
  }

  if (mode0scr != 1) {                       // СО2   ----------------------------
    lcd.setCursor(11, 2);
    lcd.print(String(dispCO2) + "ppm ");
    
  } else {
    drawPPM(dispCO2, 0, 0);
  }

  if (mode0scr != 3) {                      // Давление   ---------------------------
    lcd.setCursor(0, 3);
    lcd.print(String(dispPres) + "mm");

  } else {
    drawPres(dispPres, 0, 0);
  }

  if (mode0scr != 0) {                      // время   ----------------------------
    lcd.setCursor(15, 3);
    if (hrs / 10 == 0) lcd.print(" ");
    lcd.print(hrs);
    lcd.print(":");
    if (mins / 10 == 0) lcd.print("0");
    lcd.print(mins);

  } else {
    drawClock(hrs, mins, 0, 0);
  }

}

void plotSensorsTick() {
  // таймер
  if (testTimer(dayPlotTimerD, dayPlotTimer)) {
    long averTemp = 0, averHum = 0, averPress = 0, averCO2 = 0; 


    averTemp /= 15;
    averHum /= 15;
    averPress /= 15;
    averCO2 /= 15;

    for (byte i = 0; i < 14; i++) {
      tempDay[i] = tempDay[i + 1];
      humDay[i] = humDay[i + 1];
      pressDay[i] = pressDay[i + 1];
      co2Day[i] = co2Day[i + 1];
    }
    tempDay[14] = averTemp;
    humDay[14] = averHum;
    pressDay[14] = averPress;
    co2Day[14] = averCO2;
  }
}

boolean dotFlag;
void clockTick() {
  dotFlag = !dotFlag;
  if (dotFlag) {            // каждую секунду пересчёт времени
    secs++;
    if (secs > 59) {        // каждую минуту
      secs = 0;
      mins++;
      if (mins <= 59 && mode == 0) {
        drawSensors();      //  
      }
    }
    if (mins > 59) {        // каждый час
      now = rtc.now();
      secs = now.second();
      mins = now.minute();
      hrs = now.hour();
      if (mode == 0) drawSensors();
      if (hrs > 23) hrs = 0;
      if (mode == 0) drawData();
    }
    if (mode == 0) {   // показывать секунды  
      lcd.setCursor(15, 1);
      if (secs < 10) lcd.print(" ");
      lcd.print(secs);
    }
  }

  if (mode == 0) {                // Мигающие точки, разделяющие часы и минуты 

    byte code;
    if (dotFlag) code = 165;
    else         code = 32;
    if (mode0scr == 0) {          // мигание большими точками только в нулевом режиме главного экрана  
      
      lcd.setCursor(7, 0);
      lcd.write(code);
      lcd.setCursor(7, 1);
      lcd.write(code);
    }
    else {
      if (code == 165) code = 58;
      lcd.setCursor(17, 3);
      lcd.write(code);
    }
  }

  if ((dispCO2 >= blinkLEDCO2) && !dotFlag) setLEDcolor(0);     // мигание индикатора в зависимости от значения и привязанного сенсора  
  else setLED();
}

boolean testTimer(unsigned long & dataTimer, unsigned long setTimer) {   // Проверка таймеров  
  if (millis() - dataTimer >= setTimer) {
    dataTimer = millis();
    return true;
  } else {
    return false;
  }
}
