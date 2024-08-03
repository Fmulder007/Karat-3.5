char ver[ ] = "160x05";
/*
   Для Карат-3,5
   F2 = 13557.6kHz(LSB)-13560kHz(USB)
   F1 = 34440kHz
   F0 = VFO+F2+F1
   ВНИМАНИЕ!!! Применять ядро от AlexGyver: https://github.com/AlexGyver/GyverCore
   Добавлен Канальный режим. В нем недоступны "лишние" настройки
   Настройки второй ПЧ разбиты на 4 для применения разных фильтров RX/TX
   Включение с нажатым кнобом переводит в режим инжменю.
   Добавлен цифровой фильтр для вольтметра
   Тональник на D10
   Добавлен режим импульсного управления для реле с фиксацией

*/

//#define SI_OVERCLOCK 750000000L
#define ENCODER_OPTIMIZE_INTERRUPTS

#define crcmod 5// поправка расчета CRC для НЕ СОВМЕСТИМОСТИ со старыми прошивками

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_I2C_ADRESS 0x3C // Display I2c adress

#define max_number_of_bands	30 // Максимальное количество диапазонов.
#define Si_Xtall_Freq 27000000UL // Частота кварца si5351, Гц.
#define si_cload SI5351_CRYSTAL_LOAD_10PF// 
#define lo_max_freq 13580000UL // Максимальная частота 13560 КГц опоры, Гц.
#define lo_min_freq 13540000UL // Минимальная частота 13560 КГц опоры, Гц.
#define bfo_max_freq 80000000UL // Максимальная частота F2 опоры, Гц.
#define bfo_min_freq 30000000UL // Минимальная частота F2 опоры, Гц.
#define min_hardware_freq 15 // *100KHz Минимальный железный предел частоты диапазона VFO
#define max_hardware_freq 300 // *100KHz Максимальный железный предел частоты диапазона VFO
#define ONE_WIRE_BUS 14 // Порт датчика температуры
#define myEncBtn 4 // Порт нажатия кноба.
#define fwdpin 15 // Порт fwd показометра мощности. А1
#define revpin 16 // Порт rew показометра мощности. А2
#define mybattpin 21 // Порт датчика АКБ
#define txsenspin 17 //Порт датчика ТХ
#define pttpin 6 // PTT
#define dotpin 5 // CW TX dot
#define dashpin 7 // CW TX dash
#define txenpin 9 // TX en out pin
#define rxenpin 8 // RX en out pin
#define tonefreq 500 // Частота тонального сигнала для настройки TX.
#define pttdelay 50 //Задержка выключения PTT
#define HiBandPin 11 //Пин управления ФНЧ передатчика



#include "Adafruit_SSD1306.h" // Use version 1.2.7!!!
#include "si5351a.h"
#include "Wire.h"
#include "Encoder.h"
#include "Eeprom24C32_64.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "DS1307RTC.h"
#include "GyverTimers.h"


//Общие настройки
struct general_set {
  uint8_t stp = 3; //Начальный шаг настройки.
  uint8_t band = 0; // Стартовый диапазон.
  uint8_t number_of_bands = 0; // Количество диапазонов.
  bool cmode = false; // Канальный режим.
  uint32_t bfo_freq = 34430000UL; // Начальная частота BFO при первом включении.
  uint32_t usb_lo_freq = 13561000UL; // Начальная частота опоры USB при первом включении.
  uint32_t lsb_lo_freq = 13558000UL; // Начальная частота опоры LSB при первом включении.
  int16_t Si_Xtall_calFreq = 6550; // Начальная частота калибровки кварца, Гц.
  uint8_t batt_cal = 208; // Начальная калибровка вольтметра.
  bool reverse_encoder = false; //Реверс энкодера.
  uint8_t mem_enc_div = 4; // Делитель импульсов энкодера
  int8_t temp_cal = 0; //Калибровка термометра
  uint8_t HiBandBorder = 40; // Граница переключения ФНЧ передатчика
  //CW Section
  uint8_t cwdelay = 50; // Задержка переключения на прием после CW передачи * 10мсек
  uint8_t cwtone = 70; // Сдвиг частоты CW *10 Гц
  uint8_t cwtype = 0; // 0: BUG Keyer, 1:Paddle Keyer
} general_setting;


// Диапазонные настройки
struct band_set {
  bool mode = 0; // LSB=0, USB=1.
  uint32_t vfo_freq = 7100000UL; // Начальная частота VFO при первом включении.
  uint8_t min_freq = 15; // *100KHz Минимальный предел частоты диапазона VFO.
  uint32_t max_freq = 300; // *100KHz Максимальный предел частоты диапазона VFO.
} band_setting;


//
//
//

uint8_t menu = 0; //Начальное положение меню.
uint16_t arraystp[] = {1, 10, 50, 100, 500, 1000, 10000}; //шаги настройки * 10 герц.

uint8_t enc_div = 4;
//uint8_t mypower = 0;
// float myswr=0;
uint16_t fwdpower = 0;
uint16_t revpower = 0;
uint8_t mybatt = 0;
int8_t temperature = 0;
uint16_t screenstep = 1000;
uint16_t rawbatt = 0;
bool toneen = false;

long oldPosition  = 0;
bool actencf = false;
bool txen = false;
bool knobup = true;
bool exitmenu = false;
bool reqtemp = false;
bool timesetup = false;
bool actfmenuf = false;
bool HiBand = false;

// CW flags
bool cwtxen = false;
bool cwkeydown = false;
bool cwsemitoneen = false;
uint32_t cwkeyreleasetimer = 0;
uint8_t cwkeycount = 0;


// PTT flags
bool ptten = 0;
bool pttdown = 0;
bool state = 0;
uint8_t pttcount = 0;
uint32_t pttreleasetimer = 0;




uint32_t previousdsp = 0;
uint32_t previoustemp = 0;
uint32_t previoustime = 0;
uint32_t knobMillis = 0;
uint32_t actenc = 0;



static Eeprom24C32_64 AT24C32(0x50);
Si5351 si;
Encoder myEnc(2, 3); //порты подключения енкодера.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
tmElements_t tm;


void setup() {
  //PTT control setup
  pinMode (pttpin, INPUT); // PTT pin input
  pinMode (txenpin, OUTPUT); // TX control pin output
  pinMode (rxenpin, OUTPUT); // RX control pin output
  digitalWrite (pttpin, LOW); // PTT pin pullup
  digitalWrite (txenpin, LOW); // TX control pin pullup
  digitalWrite (rxenpin, LOW); // RX control pin pullup

  //CW pin`s setup
  pinMode (dotpin, INPUT);			//CW dotpin input
  pinMode (dashpin, INPUT);         //CW dashpin input
  digitalWrite (dotpin, LOW);      // CW dotpin pin pullup Disable
  digitalWrite (dashpin, LOW);     // CW dashpin pin pullup Disable
  //pinMode (tonepin, OUTPUT); // CW semicontrol tonepin mode
  //digitalWrite(cwsemitonepin, 0);  // CW semicontrol tonepin GND


  // other pin`s setup
  pinMode(myEncBtn, INPUT);
  pinMode(fwdpin, INPUT);
  pinMode(revpin, INPUT);
  pinMode (10, OUTPUT);      // настроить пин как выход
  pinMode (HiBandPin, OUTPUT);
  digitalWrite(myEncBtn, HIGH);
  analogReference(INTERNAL);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADRESS);
  display.clearDisplay();
  display.display();
  sensors.begin();
  memread();
  enc_div = general_setting.mem_enc_div;
  si5351init();
  si5351correction();
  vfosetup();
  battmeter();
  powermeter();
  tempsensor ();
  timenow ();
  if (!digitalRead(myEncBtn)) menu = 100;
  versionprint ();
  mainscreen();

}

void loop() { // Главный цикл
  pttsensor();
  cw();
  rxtxcontrol();
  pushknob();
  if (!general_setting.cmode) readencoder(); //Если не в канальном режиме - считать енкодер
  if (general_setting.cmode && menu) readencoder(); //Если в канальном режиме и не на главном экране - считать енкодер
  txsensor();
  battmeter();
  if (!menu) {
    if (txen) screenstep = 100;
    else screenstep = 1000;
    if (millis() - previousdsp > screenstep) {
      storetomem();
      //battmeter();
      powermeter();
      tempsensor();
      timenow();
      mainscreen();
      previousdsp = millis();
    }
  }
}

void timenow () {
  if (timesetup) {
    tm.Second = 0;
    RTC.write(tm);
    timesetup = false;
  }
  else {
    if (millis() - previoustime > 1000 || !previoustime) {
      previoustime = millis();
      if (!RTC.read(tm)) {
        if (RTC.chipPresent()) {
          tm.Hour = 0;  tm.Minute = 0; tm.Second = 0;
          RTC.write(tm);
        }
      }
    }
  }
}

void tempsensor () {
  if (millis() - previoustemp > 5000 && !reqtemp) {
    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();
    sensors.setWaitForConversion(true);
    reqtemp = true;
  }
  if (millis() - previoustemp > 8000 && reqtemp) {
    temperature = (int8_t)(0.5 + sensors.getTempCByIndex(0));
    temperature = temperature + general_setting.temp_cal;
    previoustemp = millis();
    reqtemp = false;
  }
}

void txsensor () {
  bool txsens = digitalRead(txsenspin);
  //Если радио на приеме и нажали ТХ то txen = true
  if (txsens && !txen) {
    txen = true;
    mainscreen();
  }
  //Если радио на передаче и отпустили ТХ то txen = false
  if (!txsens && txen) {
    txen = false;
    mainscreen();
  }
}

void pushknob () {  // Обработка нажатия на кноб

  bool knobdown = digitalRead(myEncBtn);   //Читаем состояние кноба
  if (!knobdown && knobup) {   //Если кноб был отпущен, но нажат сейчас
    knobup = false;   // отмечаем флаг что нажат
    knobMillis = millis();  // запускаем таймер для антидребезга
  }

  if (knobdown && !knobup) { //Если кноб нажат
    knobup = true; // отмечаем флаг что кноб отпущен
    long knobupmillis = millis();
    if (knobupmillis - knobMillis >= 1000) { //Если длительное нажатие
      if (menu == 0) {
        if (general_setting.cmode) menu = 23;
        else menu = 20;// Если долгое нажатие на главном экране, то перейти в юзерменю
      }
      else if (menu != 0) menu = 0; // Если долгое нажатие не на главном экране, то перейти на главный экран
    }

    if (knobupmillis - knobMillis < 1000 && knobupmillis - knobMillis > 100) { //Если кноб отпущен и был нажат и времени от таймера прошло 100Мс
      if (menu < 4 && menu > 0 && actfmenuf) //Если 0<меню<4 и крутили енкодер в быстром меню, то выйти на главный экран
      {
        actfmenuf = false;
        menu = 0;
      }
      else {
        menu ++; //Переходим на меню дальше
        if (menu == 4) menu = 0; //Если меню 5 выйти на главный экран
        if (menu > 28 && menu < 100) menu = 20; //Если меню > 30 но < 100 перейти на меню 20
        if (menu > 107) menu = 100; //Если меню больше 107 перейти на меню 100
      }
      if (!general_setting.number_of_bands && (menu == 1 || menu == 100)) menu++; // Если каналы не настроены, то нет меню 1 и 100
      if (general_setting.cmode && general_setting.number_of_bands) {                             //Если в канальном режиме, то пропускать меню 2,3,20,21,22,28,29
        switch (menu) {
          case 2:
          case 3: menu = 0;
            break;
          case 20:
          case 21:
          case 22:
          case 27:
          case 28: menu = 23;
            break;
        }
      }
      if (menu == 104) band_setting.mode = false;
      if (menu == 105) band_setting.mode = true;
    }
    mainscreen();
  }
}

void storetomem() { // Если крутили енкодер, то через 10 секунд все сохранить

  if ((millis() - actenc > 10000UL) && actencf) {
    actenc = millis();
    actencf = false;
    memwrite ();
  }
}

void readencoder() { // работа с енкодером
  long newPosition;
  newPosition = myEnc.read() / enc_div;
  if (general_setting.reverse_encoder) newPosition *= (-1);
  if (newPosition != oldPosition && digitalRead(myEncBtn)) { // ЕСЛИ КРУТИЛИ энкодер

    if (menu > 0 && menu < 5) actfmenuf = true; // Если крутили энкодер в быстром меню - флаг вверх!
    switch (menu) {

      case 0: //Основная настройка частоты
        if (newPosition > oldPosition && band_setting.vfo_freq <= band_setting.max_freq * 100000UL) {
          if (band_setting.vfo_freq % (arraystp[general_setting.stp] * 10UL)) {
            band_setting.vfo_freq = band_setting.vfo_freq + (arraystp[general_setting.stp] * 10UL) - (band_setting.vfo_freq % (arraystp[general_setting.stp] * 10UL));
          }
          else {
            band_setting.vfo_freq = band_setting.vfo_freq + (arraystp[general_setting.stp] * 10UL);
          }
        }
        if (newPosition < oldPosition && band_setting.vfo_freq >= band_setting.min_freq * 100000UL) {
          if (band_setting.vfo_freq % (arraystp[general_setting.stp] * 10UL)) {
            band_setting.vfo_freq = band_setting.vfo_freq - (band_setting.vfo_freq % (arraystp[general_setting.stp] * 10UL));
          }
          else {
            band_setting.vfo_freq = band_setting.vfo_freq - (arraystp[general_setting.stp] * 10UL);
          }
        }
        if (band_setting.vfo_freq < band_setting.min_freq * 100000UL) band_setting.vfo_freq = band_setting.min_freq * 100000UL;
        if (band_setting.vfo_freq > band_setting.max_freq * 100000UL) band_setting.vfo_freq = band_setting.max_freq * 100000UL;
        vfosetup();
        break;

      case 1: //Переключение каналов
        if (newPosition > oldPosition && general_setting.band < general_setting.number_of_bands) general_setting.band++;
        if (newPosition < oldPosition && general_setting.band > 0) general_setting.band--;
        general_setting.band = constrain(general_setting.band, 0, general_setting.number_of_bands);
        band_memread();
        vfosetup();
        break;

      case 2: //Настройка ШАГА настройки
        if (newPosition > oldPosition && general_setting.stp < (sizeof(arraystp) / sizeof(arraystp[0]) - 1)) general_setting.stp++;
        if (newPosition < oldPosition && general_setting.stp > 0) general_setting.stp--;
        if (general_setting.stp > (sizeof(arraystp) / sizeof(arraystp[0]) - 1)) general_setting.stp = (sizeof(arraystp) / sizeof(arraystp[0]) - 1);
        break;

      case 3: //Переключение LSB|USB.
        band_setting.mode = !band_setting.mode;
        vfosetup();
        break;

      case 20: //Настройка min_freq
        if (newPosition > oldPosition && band_setting.min_freq <= max_hardware_freq) band_setting.min_freq++;
        if (newPosition < oldPosition && band_setting.min_freq >= min_hardware_freq) band_setting.min_freq--;
        band_setting.min_freq = constrain(band_setting.min_freq, min_hardware_freq, band_setting.max_freq - 1);
        break;

      case 21: //Настройка maxfreq
        if (newPosition > oldPosition && band_setting.max_freq <= max_hardware_freq) band_setting.max_freq++;
        if (newPosition < oldPosition && band_setting.max_freq >= min_hardware_freq) band_setting.max_freq--;
        band_setting.max_freq = constrain(band_setting.max_freq, band_setting.min_freq + 1, max_hardware_freq);
        break;

      case 22: // Настройка количества каналов
        if (newPosition > oldPosition && general_setting.number_of_bands < max_number_of_bands) general_setting.number_of_bands++;
        if (newPosition < oldPosition && general_setting.number_of_bands > 0) general_setting.number_of_bands--;
        if (general_setting.number_of_bands > max_number_of_bands) general_setting.number_of_bands = max_number_of_bands;
        if (general_setting.band > general_setting.number_of_bands) general_setting.band = general_setting.number_of_bands;
        break;

      case 23: //Настройка Часов
        if (newPosition > oldPosition && tm.Hour < 24) tm.Hour++;
        if (newPosition < oldPosition && tm.Hour > 0) tm.Hour--;
        if (tm.Hour > 23) tm.Hour = 23;
        timesetup = true;
        break;

      case 24: //Настройка Минут
        if (newPosition > oldPosition && tm.Minute < 60) tm.Minute++;
        if (newPosition < oldPosition && tm.Minute > 0) tm.Minute--;
        if (tm.Minute > 59) tm.Minute = 59;
        timesetup = true;
        break;

      case 25: // Настройка CW-Delay
        if (newPosition > oldPosition && general_setting.cwdelay < 255) general_setting.cwdelay++;
        if (newPosition < oldPosition && general_setting.cwdelay > 1) general_setting.cwdelay--;
        general_setting.cwdelay = constrain(general_setting.cwdelay, 10, 255);
        break;

      case 26: // Настройка CW-Tone
        if (newPosition > oldPosition && general_setting.cwtone < 255) general_setting.cwtone++;
        if (newPosition < oldPosition && general_setting.cwtone > 1) general_setting.cwtone--;
        general_setting.cwtone = constrain(general_setting.cwtone, 10, 255);
        break;


      case 27: // Настройка делителя импульсов энкодера
        if (newPosition > oldPosition && general_setting.mem_enc_div < 255) general_setting.mem_enc_div++;
        if (newPosition < oldPosition && general_setting.mem_enc_div > 1) general_setting.mem_enc_div--;
        general_setting.mem_enc_div = constrain(general_setting.mem_enc_div, 1, 255);
        break;

      case 28: //Инверсия энкодера.
        if (general_setting.reverse_encoder) {
          general_setting.reverse_encoder = false;
        }
        else {
          general_setting.reverse_encoder = true;
        }
        newPosition *= (-1);
        break;

      case 100: //Канальный режим.
        general_setting.cmode = !general_setting.cmode;
        break;

      case 101: //Настройка калибровки по питанию
        if (newPosition > oldPosition && general_setting.batt_cal <= 254) general_setting.batt_cal++;
        if (newPosition < oldPosition && general_setting.batt_cal >= 100) general_setting.batt_cal--;
        general_setting.batt_cal = constrain(general_setting.batt_cal, 100, 254);
        break;

      case 102: //Калибровка термодатчика
        if (newPosition > oldPosition && general_setting.temp_cal <= 30) general_setting.temp_cal++;
        if (newPosition < oldPosition && general_setting.temp_cal >= - 30) general_setting.temp_cal--;
        general_setting.temp_cal = constrain(general_setting.temp_cal, -30, 30);
        break;

      case 103: //Частота кварца синтезатора
        if (newPosition > oldPosition && general_setting.Si_Xtall_calFreq <= 30000) general_setting.Si_Xtall_calFreq += arraystp[general_setting.stp];
        if (newPosition < oldPosition && general_setting.Si_Xtall_calFreq >= - 30000) general_setting.Si_Xtall_calFreq -= arraystp[general_setting.stp];
        general_setting.Si_Xtall_calFreq = constrain(general_setting.Si_Xtall_calFreq, -30000, 30000);
        si5351correction();
        vfosetup();
        break;

      case 104: //F2 LSB
        if (newPosition > oldPosition && general_setting.lsb_lo_freq <= lo_max_freq) general_setting.lsb_lo_freq += arraystp[general_setting.stp];
        if (newPosition < oldPosition && general_setting.lsb_lo_freq >= lo_min_freq) general_setting.lsb_lo_freq -= arraystp[general_setting.stp];
        general_setting.lsb_lo_freq = constrain(general_setting.lsb_lo_freq, lo_min_freq, lo_max_freq);
        vfosetup();
        break;

      case 105: //F2 USB
        if (newPosition > oldPosition && general_setting.usb_lo_freq <= lo_max_freq) general_setting.usb_lo_freq += arraystp[general_setting.stp];
        if (newPosition < oldPosition && general_setting.usb_lo_freq >= lo_min_freq) general_setting.usb_lo_freq -= arraystp[general_setting.stp];
        general_setting.usb_lo_freq = constrain(general_setting.usb_lo_freq, lo_min_freq, lo_max_freq);
        vfosetup();
        break;

      case 106: //F1
        if (newPosition > oldPosition && general_setting.bfo_freq <= bfo_max_freq) general_setting.bfo_freq += arraystp[general_setting.stp];
        if (newPosition < oldPosition && general_setting.bfo_freq >= bfo_min_freq) general_setting.bfo_freq -= arraystp[general_setting.stp];
        general_setting.bfo_freq = constrain(general_setting.bfo_freq, bfo_min_freq, bfo_max_freq);
        vfosetup();
        break;

      case 110: //Граница переключения ФНЧ пенредатчика
        if (newPosition > oldPosition && general_setting.HiBandBorder <= max_hardware_freq) general_setting.HiBandBorder++;
        if (newPosition < oldPosition && general_setting.HiBandBorder >= min_hardware_freq) general_setting.HiBandBorder--;
        general_setting.HiBandBorder = constrain(general_setting.HiBandBorder, min_hardware_freq, max_hardware_freq);
        HiBandControl();
        break;
    }
    actenc = millis();
    actencf = true;
    mainscreen();
    oldPosition = newPosition;
  }
}

void powermeter () { // Измеритель уровня выхода
  //fwdpower = constrain(analogRead(fwdpin), 1, 1023);
  //revpower = constrain(analogRead(revpin), 1, 1023);
  analogRead(fwdpin);
  fwdpower = (12 * fwdpower + 4 * (analogRead(fwdpin) + 1)) >> 4;
  //analogRead(revpin);
  revpower = (12 * revpower + 4 * (analogRead(revpin) + 1)) >> 4;
}

void battmeter () { // Измеритель напряжения питания
  //rawbatt = analogRead(mybattpin);
  rawbatt = (14 * rawbatt + 2 * (analogRead(mybattpin))) >> 4; // Крутой фильтр для усреднения показаний вольтметра!!!
  mybatt = map(rawbatt, 0, 1023, 0, general_setting.batt_cal);
}

void mainscreen() { //Процедура рисования главного экрана
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.setTextSize(3);
  if (menu) display.dim(false);
  switch (menu) {

    case 0: //Если не в меню, то рисовать главный экран
      //Вывод частоты на дисплей
      if ((band_setting.vfo_freq / 1000000) < 10) display.print(" ");
      display.print(band_setting.vfo_freq / 1000000);//Вывод МГц
      display.setCursor(display.getCursorX() + 5, display.getCursorY()); //Переводим курсор чуть правее текущего положения
      if ((band_setting.vfo_freq % 1000000) / 1000 < 100) display.print("0");
      if ((band_setting.vfo_freq % 1000000) / 1000 < 10) display.print("0");
      display.print((band_setting.vfo_freq % 1000000) / 1000); //Выводим КГц
      display.setTextSize(2); // Для сотен и десятков герц делаем шрифт поменьше
      display.setCursor(display.getCursorX() + 5, 7); //Переводим курсор чуть ниже текущего положения
      if ((band_setting.vfo_freq % 1000) / 10 < 10) display.print("0"); //Если герц <10 то выводим "0" перед ними.
      display.println((band_setting.vfo_freq % 1000) / 10);

      //Выводим вторую строку на дисплей
      display.setTextSize(1);// Ставим маленький шрифт
      if (mybatt - 100 < 0) display.print(" ");
      display.print(mybatt / 10);
      display.print(".");
      display.print(mybatt % 10);
      display.print("v");
      //display.setTextSize(1);
      //if (ptten) display.print("PTT");
      //if (cwtxen) display.print("CWtxen");

      if (txen) {//Если передача, то вывод показометра мощности
        if ((fwdpower - revpower) > 0) {
          if ((fwdpower + revpower) / (fwdpower - revpower) < 10)display.print(" ");
          display.print((fwdpower + revpower) / (fwdpower - revpower));
          display.print(".");
          display.print(((fwdpower + revpower) % (fwdpower - revpower)) % 10);
        }
        display.fillRect(64, 23, (map(fwdpower, 1, 1023, 0, 100)), 4, WHITE);
        display.fillRect(64, 28, (map(revpower, 1, 1023, 0, 100)), 4, WHITE);
      }
      else {// Если прием, то рисовать температуру часы, полосу и диапазон
        //char ddot
        if (temperature >= -50 && temperature <= 50) {
          if (temperature >= 0) display.print(" ");
          display.print(temperature);
          display.print((char)247);
          display.print("C");
        }
        else {
          display.print(" err ");
        }
        if (actencf) {
          display.print(" ");
          display.dim(false);
        }
        else {
          display.print(".");
          display.dim(true);
        }
        if (tm.Hour < 10) display.print(" ");
        display.print(tm.Hour);
        if (tm.Second % 2) {
          display.print(":");
        }
        else {
          display.print(" ");
        }
        if (tm.Minute < 10) display.print("0");
        display.print(tm.Minute);
        if (band_setting.mode) {
          display.print(" U");
        }
        else {
          display.print(" L");
        }
        if (general_setting.band < 10) display.print(" ");
        display.print(general_setting.band);
      }
      break;

    case 1: //Меню 1 - канал
      display.println(general_setting.band);
      display.setTextSize(1);
      display.print(menu);
      display.print("  Ch from 0 to ");
      display.print(general_setting.number_of_bands);
      break;

    case 2: //Меню 2 - шаг настройки
      display.println(arraystp[general_setting.stp] * 10UL);
      display.setTextSize(1);
      display.print(menu);
      display.print("  Step Hz");
      break;

    case 3: //Меню 3 - LSB|USB
      if (band_setting.mode) {
        display.println("USB");
      }
      else
      {
        display.println("LSB");
      }
      display.setTextSize(1);
      display.print(menu);
      display.println("  LSB|USB Switch");
      break;
    //-----------------------------USER MENU Display-------------------------//
    case 20: //Настройка min_freq
      display.println(band_setting.min_freq * 100);
      display.setTextSize(1);
      display.print(menu);
      display.print("  Min Freq ");
      display.print((char)240);
      display.print("kHz");
      break;

    case 21: //Настройка maxfreq
      display.println(band_setting.max_freq * 100);
      display.setTextSize(1);
      display.print(menu);
      display.print("  Max Freq ");
      display.print((char)240);
      display.print("kHz");
      break;

    case 22: //Количество каналов
      display.println(general_setting.number_of_bands);
      display.setTextSize(1);
      display.print(menu);
      display.print("  MAX Num Ch");
      break;


    case 23: //Меню 12 - Настройка Часов
      if (tm.Hour < 10) display.print("0");
      display.println(tm.Hour);
      display.setTextSize(1);
      display.print(menu);
      display.print("  Hour");
      break;

    case 24: //Меню 13 - Настройка Минут
      if (tm.Minute < 10) display.print("0");
      display.println(tm.Minute);
      display.setTextSize(1);
      display.print(menu);
      display.print("  Min");
      break;

    case 25: //Меню 17 - CW-Delay
      display.println(general_setting.cwdelay * 10);
      display.setTextSize(1);
      display.print(menu);
      display.print("  CW Delay msec");
      break;

    case 26: //Меню 18 - CW-Tone
      display.println(general_setting.cwtone * 10);
      display.setTextSize(1);
      display.print(menu);
      display.print("  CW Tone Hz");
      break;


    case 27: //Меню 15 - Encoder Divider
      display.println(general_setting.mem_enc_div);
      display.setTextSize(1);
      display.print(menu);
      display.print(" !Enc Divider!");
      break;

    case 28: //Меню 14 - Reverse Encoder
      if (general_setting.reverse_encoder) {
        display.println("Yes");
      }
      else
      {
        display.println("NO");
      }
      display.setTextSize(1);
      display.print(menu);
      display.println("  Reverse Enc");
      break;

    //-------------------------------------SETUP MENU DISPLAY--------------------------------------//

    case 100: //Channel mode
      if (general_setting.cmode) {
        display.println("Yes");
      }
      else
      {
        display.println("NO");
      }
      display.setTextSize(1);
      display.print(menu);
      display.println("  CHannel MODE");
      break;


    case 101: //Меню 10 - Настройка калибровки по питанию
      display.println(general_setting.batt_cal);
      display.setTextSize(1);
      display.print(menu);
      display.print("  Batt ");
      if (mybatt - 100 < 0) display.print("0");
      display.print(mybatt / 10);
      display.print(".");
      display.print(mybatt % 10);
      display.print("v");
      break;

    case 102: //Калибровка термодатчика
      display.println(general_setting.temp_cal);
      display.setTextSize(1);
      display.print(menu);
      display.print(" Temp CAL ");
      display.print((char)240);
      display.print((char)247);
      display.print("C");
      break;

    case 103: //Настройка калибровки кварца
      display.println(general_setting.Si_Xtall_calFreq);
      display.setTextSize(1);
      display.print(menu);
      display.print("  Xtal Cal ");
      display.print((char)240);
      display.print("Hz");
      break;


    case 104: //Настройка LO LSB
      display.setTextSize(2);
      display.println(general_setting.lsb_lo_freq);
      display.setTextSize(1);
      display.print(menu);
      display.print(" LO LSB ");
      display.print((char)240);
      display.print("Hz");
      break;


    case 105: //Настройка LO USB
      display.setTextSize(2);
      display.println(general_setting.usb_lo_freq);
      display.setTextSize(1);
      display.print(menu);
      display.print(" LO USB ");
      display.print((char)240);
      display.print("Hz");
      break;

    case 106: //Настройка BFO
      display.setTextSize(2);
      display.println(general_setting.bfo_freq);
      display.setTextSize(1);
      display.print(menu);
      display.print(" BFO ");
      display.print((char)240);
      display.print("Hz");
      break;


    case 107: //Настройка переключения ФНЧ передатчика
      display.println(general_setting.HiBandBorder * 100);
      display.setTextSize(1);
      display.print(menu);
      display.print(" LPF Border Freq ");
      display.print((char)240);
      display.print("kHz");
      break;

  }
  display.display();
  //debug();

}


void vfosetup() {
  if (band_setting.mode) {
    si.set_freq((band_setting.vfo_freq + general_setting.bfo_freq + general_setting.usb_lo_freq), general_setting.usb_lo_freq, general_setting.bfo_freq);
  }
  else {
    si.set_freq((band_setting.vfo_freq + general_setting.bfo_freq + general_setting.lsb_lo_freq), general_setting.lsb_lo_freq, general_setting.bfo_freq);
  }
  HiBandControl();
}

void si5351init() {
  si.setup(0, 0, 0);
  si.cload(si_cload);
}

void si5351correction() {
  si.set_xtal_freq(Si_Xtall_Freq + general_setting.Si_Xtall_calFreq);
  si.update_freq(0);
  si.update_freq(1);
  //si.update_freq(2);
}

void memwrite () { 
  //Запись general_setting
  int16_t crc = 0;
  uint8_t i = 0;
  uint8_t * adr;
  adr =  (uint8_t*)(& general_setting);
  while (i < (sizeof(general_setting)))
  {
    crc += *(adr + i);
    i++;
  }
  AT24C32.writeEE(2, general_setting);
  AT24C32.writeEE(0, (crc + crcmod));

  // Запись band_setting
  crc = 0;
  i = 0;
  adr =  (uint8_t*)(& band_setting);
  while (i < (sizeof(band_setting)))
  {
    crc += *(adr + i);
    i++;
  }
  AT24C32.writeEE(sizeof(general_setting) + 2 + ((sizeof(band_setting) + 2)*general_setting.band) + 2, band_setting);
  AT24C32.writeEE(sizeof(general_setting) + 2 + ((sizeof(band_setting) + 2)*general_setting.band), (crc + crcmod));
}

void memread() {
  int16_t crc = 0;
  int16_t crcrom = 0;
  uint8_t i = 0;

  // Чтение general_setting
  AT24C32.readEE (0, crc);
  while (i < (sizeof(general_setting)))
  {
    crcrom += AT24C32.readByte ((i + 2));
    i++;
  }
  if (crc == (crcrom + crcmod)) {
    AT24C32.readEE (2, general_setting);
  }
  else {
    memwrite ();
  }
  band_memread();
}

void band_memread() {

  int16_t crc = 0;
  int16_t crcrom = 0;
  uint8_t i = 0;
  AT24C32.readEE (sizeof(general_setting) + 2 + ((sizeof(band_setting) + 2) * general_setting.band), crc);
  while (i < (sizeof(band_setting)))
  {
    crcrom += AT24C32.readByte ((i + sizeof(general_setting) + 2 + ((sizeof(band_setting) + 2) * general_setting.band)) + 2);
    i++;
  }
  if (crc == (crcrom + crcmod)) {
    AT24C32.readEE (sizeof(general_setting) + 2 + ((sizeof(band_setting) + 2)*general_setting.band) + 2, band_setting);
  }
  else {
    memwrite ();
  }
}

void versionprint() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.setTextSize(3);
  if (menu == 100) {
    display.println("Setup");
    display.display();
    while (!digitalRead(myEncBtn));
  }
  else {
    display.println(ver);
    display.setTextSize(1);
    display.println(" From UD0DAB 2024");
    display.display();
  }
  delay(1000);
}



void cwsemitonegen() {
  if (cwtxen && cwkeydown && !cwsemitoneen) {
    if (!menu) {
      Timer1.setFrequency(general_setting.cwtone * 20);                 // Включаем тональник
      Timer1.outputEnable(CHANNEL_B, TOGGLE_PIN);   // в момент срабатывания таймера пин будет переключаться
      cwsemitoneen = true;
    }
  }
  if (!cwkeydown && cwsemitoneen) {
    Timer1.outputDisable(CHANNEL_B); // Если включен тональник, выключаем его
    cwsemitoneen = false;
  }
}





void cw() { // Процедура работы с ключом
  if (!ptten) {
    if (!digitalRead (dotpin)) {     //Если DOTpin=0, то:
      cwkeycount++;     // счетчик ключа +1
    }
    else {              // Если нет контакта
      cwkeycount = 0;    // Сбросить счетчики
      if (cwtxen) {
        if (cwkeydown) {                 // Если ключ БЫЛ НАЖАТ раньше, то
          cwkeyreleasetimer = millis();                       // Запоминаем время последнего отпускания ключа
          cwkeydown = false;                                  // Запоминаем что ключ был отпущен
          vfosetup();
          cwsemitonegen();
        }
        if (!cwkeydown && ((millis() - cwkeyreleasetimer) >= (general_setting.cwdelay * 10))) {
          cwtxen = false;
          state = false;
          vfosetup();
          cwsemitonegen();
        }
      }
    }

    if (cwkeycount > 10)  {   // Если счетчик непрерывно нажатого ключа больше 10 то:
      cwkeycount = 10;    // поддерживаем счетчик

      if (!cwkeydown) {                   // Если ключ НЕ БЫЛ НАЖАТ, то
        cwkeydown = true;                  // Запоминаем что ключ нажат
        vfosetup();
        cwsemitonegen();
      }

      if (!cwtxen && cwkeydown) {         // Если НЕ на передаче, но ключ нажат:
        cwtxen = true;                    // Переводим трансивер на передачу в CW
        state = true;
        vfosetup();
        cwsemitonegen();
      }
    }
  }
}

void pttsensor() {
  if (!cwtxen) {              // Если не на передаче в CW
    if (!digitalRead (pttpin)) {      // Если PTT на земле, то:
      pttcount++;                     // увеличить счетчик PTT на 1
    }
    else {                            // Если нет контакта PTT
      pttcount = 0;                   // Сбросить счетчик
      if (ptten) {                       // Если на передаче
        if (pttdown) {                   //  и PTT был нажат, то
          pttreleasetimer = millis();         // запоминаем момент отпускания PTT.
          pttdown = false;                    // Запоминаем, что PTT отпустили.
        }

        if (!pttdown && (millis() - pttreleasetimer >= pttdelay)) {  //  Если отпустили давно, то:
          ptten = false;                                                    //  переводим на прием
          if (toneen) {
            Timer1.outputDisable(CHANNEL_B); // Если включен тональник, выключаем его
            delay(100);
            toneen = false;
          }
          vfosetup();
          state = false;;

        }
      }
    }

    if (pttcount > 10)  {   // Если счетчик непрерывно нажатого PTT больше 10 то:
      pttcount = 10;        // 1 - поддерживаем счетчик pttcount на 10
      if (!pttdown) pttdown = true;       // 2 - запоминаем, что PTT нажат
      if (!ptten) {
        ptten = true;         // 3 - переводим на PTT передачу
        vfosetup();
        state = true;


        if (menu > 0 && menu <= 3) {   // Если PTT в быстром меню - дать тон
          menu = 0;
          delay(100);
          Timer1.setFrequency(500 * 2);                 // Включаем тональник на 500 герц
          Timer1.outputEnable(CHANNEL_B, TOGGLE_PIN);   // в момент срабатывания таймера пин будет переключаться
          toneen = true;
        }
      }
    }
  }
}

void rxtxcontrol() {
  if (!txen && state) {
    digitalWrite (rxenpin, LOW);
    digitalWrite (txenpin, HIGH);
  }
  if (txen && !state) {
    digitalWrite (txenpin, LOW);
    digitalWrite (rxenpin, HIGH);
  }
}

void HiBandControl() {
  if (!HiBand && band_setting.vfo_freq > general_setting.HiBandBorder * 100000UL) {
    digitalWrite (HiBandPin, HIGH);
    HiBand = true;
  }
  if (HiBand && band_setting.vfo_freq < general_setting.HiBandBorder * 100000UL) {
    digitalWrite (HiBandPin, LOW);
    HiBand = false;
  }
}