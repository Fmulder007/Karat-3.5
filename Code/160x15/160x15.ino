#define ver F("160x15")
/*
   Для Карат-3,5
    "GyverOLED.h" и меню на русском языке
    Добавлено управление ДПФ
    Для версии плат синтезатора 150х12
   F2 = 13557.6kHz(LSB)-13560kHz(USB)
   F1 = 34440kHz
   F0 = VFO+F2+F1
   ВНИМАНИЕ!!! Применять ядро от AlexGyver: https://github.com/AlexGyver/GyverCore
   Добавлен Канальный режим. В нем недоступны "лишние" настройки
   Включение с нажатым кнобом переводит в режим инжменю.
   Добавлен цифровой фильтр для вольтметра
   Тональник на D10

*/

//#define SI_OVERCLOCK 750000000L
#define ENCODER_OPTIMIZE_INTERRUPTS

#define crcmod 7// поправка расчета CRC для НЕ СОВМЕСТИМОСТИ со старыми прошивками

//#define SCREEN_WIDTH 128 // OLED display width, in pixels
//#define SCREEN_HEIGHT 32 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
//#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//#define OLED_I2C_ADRESS 0x3C // Display I2c adress

#define FullContrast 255
#define LowContrast 1
#define x00 66

#define max_number_of_bands	30 // Максимальное количество диапазонов.
#define Si_Xtall_MinFreq 22000000UL // Минимальная Частота кварца si5351, Гц.
#define Si_Xtall_MaxFreq 35000000UL // Максимальная Частота кварца si5351, Гц.
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
#define pttpin 6 // PTT.
#define dotpin 5 // CW TX dot.
#define dashpin 7 // CW TX dash.
#define txenpin 9 // TX en out pin.
#define rxenpin 8 // ATU-100 en out pin.
#define tonefreq 500 // Частота тонального сигнала для настройки TX.
#define pttdelay 50 //Задержка выключения PTT.
#define minEncSpeed 1000 // Минимальная скорость вращения энкодера.

// Пины управления ФНЧ передатчика
#define dataPin 11 // DS 74hc595 controller pin
#define latchPin 12 // ST_CP 74hc595 controller pin 
#define clockPin 13 //  SH_CP 74hc595 controller pin

// границы переключения ФНЧ или ДПФ


//#include "Adafruit_SSD1306.h" // Use version 1.2.7!!!
#include "si5351a.h"
#include "Wire.h"
#include "Encoder.h"
#include "Eeprom24C32_64.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "DS1307RTC.h"
#include "GyverTimers.h"
#include "EEPROM.h"
#include "GyverOLED.h"

const static uint8_t icons_8x8[][8] PROGMEM = {
  {0x00, 0x44, 0x44, 0x5f, 0x44, 0x44, 0x00, 0x00}, // 0 - +- symbol
  {0x06, 0x0f, 0x09, 0x0f, 0x06, 0x00, 0x00, 0x00}, // 1 - DEGrees symbol
};

//Общие настройки
struct general_set {
  uint8_t stp = 3; //Начальный шаг настройки.
  uint8_t band = 0; // Стартовый диапазон.
  uint8_t number_of_bands = 0; // Количество диапазонов.
  bool cmode = false; // Канальный режим.
  uint32_t bfo_freq = 34430000UL; // Начальная частота BFO при первом включении.
  uint32_t lsb_lo_freq = 13558400UL; // Начальная частота опоры USB при первом включении.
  uint32_t usb_lo_freq = 13561000UL; // Начальная частота опоры LSB при первом включении.
  uint32_t Si_Xtall_Freq = 32000000UL; // Начальная частота кварца синтезатора, Гц.
  uint8_t batt_cal = 208; // Начальная калибровка вольтметра.
  bool reverse_encoder = false; //Реверс энкодера.
  uint8_t mem_enc_div = 6; // Делитель импульсов энкодера
  int8_t temp_cal = 0; //Калибровка термометра
  uint8_t b4 = 200; // Граница b4 переключения ФНЧ передатчика * 100кГц
  uint8_t b3 = 130; // Граница b3 переключения ФНЧ передатчика * 100кГц
  uint8_t b2 = 80; // Граница b2 переключения ФНЧ передатчика * 100кГц
  uint8_t b1 = 40; // Граница b1 переключения ФНЧ передатчика * 100кГц
  //CW Section
  uint8_t cwdelay = 50; // Задержка переключения на прием после CW передачи * 10мсек
  uint8_t cwtone = 70; // Сдвиг частоты CW *10 Гц
  uint8_t cwtype = 0; // 0: BUG Keyer, 1:Paddle Keyer
  bool PowerDoubler = 1; // ВКЛ-ВЫКЛ удвоителя питания.
  int32_t if_shift = 0; // Значение If-Shift.
  bool ALC_RX = 0; // ВКЛ-ВЫКЛ АРУ Приемника.
  bool ATT_RX = 0; // ВКЛ-ВЫКЛ АТТ Приемника.
} general_setting;


// Диапазонные настройки
struct band_set {
  bool mode = 0; // LSB=0, USB=1.
  uint32_t vfo_freq = 7100000UL; // Начальная частота VFO при первом включении.
  uint32_t min_freq = 15UL; // *100KHz Минимальный предел частоты диапазона VFO.
  uint32_t max_freq = 300UL; // *100KHz Максимальный предел частоты диапазона VFO.
} band_setting;

// Размеры структур являются частью фиксированного формата AT24C32.
static_assert(sizeof(general_set) == 38, "general_set layout changed: update EEPROM map");
static_assert(sizeof(band_set) == 13, "band_set layout changed: update ring record format");

// Текущий рабочий диапазон. Поле general_setting.band оставлено в структуре для старого формата.
uint8_t band = 0;


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
//bool HiBand = false;

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

byte byteToSend = 0;
byte bpf_set = 0;

byte backup_index = 0;
byte restore_index = 0;

#define MEM_READ  0
#define MEM_WRITE 1
#define BAND_RING_COUNT (max_number_of_bands + 1)
#define BAND_RING_SIZE 128
#define BAND_RECORD_SIZE 15
#define BAND_LOG_RING_SIZE 88
#define BAND_LOG_RECORD_SIZE 3

// Единая карта двух ручных слотов полного Backup во внутренней EEPROM МК.
// Все смещения заданы относительно начала соответствующего слота.
namespace BackupMap {
enum : uint16_t {
  SlotSize = 510U,
  SlotCount = 2U,
  TotalSize = SlotSize * SlotCount,

  GeneralOffset = 0U,
  GeneralSize = sizeof(general_set),
  SelectedBandOffset = GeneralOffset + GeneralSize,
  RingDataOffset = SelectedBandOffset + 1U,
  RingRecordSize = BAND_RECORD_SIZE,
  RingCount = BAND_RING_COUNT,
  RingDataSize = RingCount * RingRecordSize,

  MetaOffset = RingDataOffset + RingDataSize,
  SignatureOffset = MetaOffset,
  VersionOffset = SignatureOffset + 1U,
  SlotNumberOffset = VersionOffset + 1U,
  CommitOffset = SlotNumberOffset + 1U,

  // CRC охватывает байты 0..506. Маркер CommitOffset в CRC не входит.
  CrcLength = CommitOffset,
  CrcOffset = CommitOffset + 1U,
  CrcSize = 2U,

  Signature = 0x4B,
  FormatVersion = 1U,
  CommitMarker = 0xA5
};
}

static_assert(BackupMap::GeneralSize == 38U, "Backup general settings layout has changed");
static_assert(BackupMap::RingRecordSize == 15U, "Backup band record size has changed");
static_assert(BackupMap::RingCount == 31U, "Backup band ring count has changed");
static_assert(BackupMap::MetaOffset == 504U, "Backup metadata offset has changed");
static_assert(BackupMap::CommitOffset == 507U, "Backup commit marker offset has changed");
static_assert(BackupMap::CrcLength == 507U, "Backup CRC must cover bytes 0..506");
static_assert(BackupMap::CrcOffset == 508U, "Backup CRC offset has changed");
static_assert(BackupMap::SlotSize == 510U, "Backup slot size has changed");
static_assert(BackupMap::TotalSize == 1020U, "Backup total size has changed");

static_assert(BackupMap::TotalSize <= 1024U, "Backup slots do not fit internal EEPROM");

#if defined(E2END)
static_assert((E2END + 1UL) >= BackupMap::TotalSize, "Internal EEPROM is too small for two backup slots");
#endif

static_assert(BackupMap::SelectedBandOffset == BackupMap::GeneralOffset + BackupMap::GeneralSize,
              "Backup selected band offset is invalid");
static_assert(BackupMap::RingDataOffset == BackupMap::SelectedBandOffset + 1U,
              "Backup ring data offset is invalid");
static_assert(BackupMap::RingDataOffset + BackupMap::RingDataSize == BackupMap::MetaOffset,
              "Backup ring map does not fit the backup slot");
static_assert(BackupMap::VersionOffset == BackupMap::SignatureOffset + 1U &&
              BackupMap::SlotNumberOffset == BackupMap::VersionOffset + 1U &&
              BackupMap::CommitOffset == BackupMap::SlotNumberOffset + 1U,
              "Backup metadata layout is invalid");
static_assert(BackupMap::CrcOffset == BackupMap::CrcLength + 1U,
              "Backup CRC offset does not match its input length");
static_assert(BackupMap::CrcOffset + BackupMap::CrcSize == BackupMap::SlotSize,
              "Backup slot size does not match its CRC position");

// Слоты выбираются вызывающим кодом. Без аргумента используется слот 1.
void backup(uint8_t slot = 1);
void restore(uint8_t slot = 1);

static Eeprom24C32_64 AT24C32(0x50);
Si5351 si;
Encoder myEnc(2, 3); //порты подключения енкодера.
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
GyverOLED<SSD1306_128x32, OLED_BUFFER> display;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
tmElements_t tm;


void setup() {

  Wire.setClock(100000L);
  
  //PTT control setup
  pinMode (pttpin, INPUT); // PTT pin input
  pinMode (txenpin, OUTPUT); // TX control pin output
  pinMode (rxenpin, OUTPUT); // ATU-100 control pin output
  digitalWrite (pttpin, LOW); // PTT pin pullup
  digitalWrite (txenpin, LOW); // TX control pin pullup
  digitalWrite (rxenpin, LOW); // ATU-100 control pin pullup

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
  // настройка пинов 74hc595
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  digitalWrite(latchPin, LOW);
  digitalWrite(clockPin, LOW);
  digitalWrite(dataPin, LOW);
  //========================
  digitalWrite(myEncBtn, HIGH);
  analogReference(INTERNAL);
  sensors.begin();
  memory(MEM_READ);
  enc_div = general_setting.mem_enc_div;
  si5351init();
  si5351correction();
  vfosetup();
  battmeter();
  powermeter();
  tempsensor ();
  timenow ();
  if (!digitalRead(myEncBtn)) menu = 100;
  display.init();
  display.clear();  
  display.update();
  versionprint();
  mainscreen();

}

void loop() { // Главный цикл
  pttsensor();
  cw();
  rxtxcontrol();
  pushknob();
  readencoder(); //считать енкодер
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

/*void tempsensor () {
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
  }*/

void tempsensor()
{
  uint32_t now = millis();

  // Запускаем новое измерение раз в секунду
  if (!reqtemp && now - previoustemp >= 1000UL)
  {
    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();

    reqtemp = true;
    previoustemp = now;
  }

  // Получаем результат через 250 мс
  if (reqtemp && now - previoustemp >= 250UL)
  {
    temperature = (int8_t)(0.5f + sensors.getTempCByIndex(0));
    temperature += general_setting.temp_cal;

    reqtemp = false;
    previoustemp = now;
  }
}

void txsensor () {
  bool txsens = digitalRead(txsenspin);
  //Если радио на приеме и нажали ТХ то txen = true
  if (txsens && !txen) {
    txen = true;
    bpfset();
    vfosetup();
    mainscreen();
  }
  //Если радио на передаче и отпустили ТХ то txen = false
  if (!txsens && txen) {
    txen = false;
    bpfset();
    vfosetup();
    mainscreen();
  }
}

void pushknob () {  // Обработка нажатия на кноб

  bool knobdown = digitalRead(myEncBtn);   //Читаем состояние кноба
  if (!knobdown && knobup) {   //Если кноб был отпущен, но нажат сейчас
    knobup = false;   // отмечаем флаг что нажат
    knobMillis = millis();  // запускаем таймер для антидребезга
  }

  if (knobdown && !knobup) {              //Если кноб нажат
    knobup = true;                        // отмечаем флаг что кноб отпущен
    oldPosition = myEnc.read() / enc_div; //обнуляем положение энкодера, избавляет от случайных поворотов во время нажатия
    if (general_setting.reverse_encoder) oldPosition *= (-1);
    long knobupmillis = millis();
    if (knobupmillis - knobMillis >= 1000) { //Если длительное нажатие
      if (menu == 0) {
        if (general_setting.cmode) menu = 23;
        else menu = 20;// Если долгое нажатие на главном экране, то перейти в юзерменю
      }
      else if (menu != 0) menu = 0;           // Если долгое нажатие не на главном экране, то перейти на главный экран
    }

    if (knobupmillis - knobMillis < 1000 && knobupmillis - knobMillis > 100) { //Если кноб отпущен и был нажат и времени от таймера прошло 100Мс
      if (menu < 9 && menu > 0 && actfmenuf)  //Если 0<меню<9 и крутили енкодер в быстром меню, то выйти на главный экран
      {
        actfmenuf = false;
        menu = 0;
      }
      else {
        menu ++; //Переходим на меню дальше
        if (menu == 7) menu = 0; //Если меню 7 выйти на главный экран
        if (menu > 28 && menu < 100) menu = 20; //Если меню > 30 но < 100 перейти на меню 20
        if (menu > 112) menu = 100; //Если меню больше 112 перейти на меню 100
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
      //if (menu == 104) band_setting.mode = false;
      //if (menu == 105) band_setting.mode = true;
    }
    mainscreen();
  }
}

void storetomem() { // Если крутили енкодер, то через 10 секунд все сохранить

  if ((millis() - actenc > 10000UL) && actencf && !txen) {
    actenc = millis();
    if (memory(MEM_WRITE)) actencf = false;
  }
}

void readencoder() { // работа с енкодером
  long newPosition;
  newPosition = myEnc.read() / enc_div;
  if (general_setting.reverse_encoder) newPosition *= (-1);
  if (newPosition != oldPosition && digitalRead(myEncBtn)) { // ЕСЛИ КРУТИЛИ энкодер
    if (menu > 0 && menu < 9) actfmenuf = true; // Если крутили энкодер в быстром меню - флаг вверх!
    if ((actenc + minEncSpeed) >= millis()) {
      switch (menu) {
        case 0: //Основная настройка частоты
          if (!general_setting.cmode) {
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
          }
          break;

        case 1: //Переключение каналов
          if (newPosition > oldPosition && band < general_setting.number_of_bands) band++;
          if (newPosition < oldPosition && band > 0) band--;
          band = constrain(band, 0, general_setting.number_of_bands);
          memory(MEM_READ);
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

        case 4: //Переключение If-Shift.
          if (newPosition > oldPosition && general_setting.if_shift <= 3000) general_setting.if_shift = general_setting.if_shift + 50;
          if (newPosition < oldPosition && general_setting.if_shift >= -3000) general_setting.if_shift = general_setting.if_shift - 50;
          general_setting.if_shift = constrain(general_setting.if_shift, -3000, 3000);
          vfosetup();
          break;

        case 5: //Переключение PowerDoubler.
          general_setting.PowerDoubler = !general_setting.PowerDoubler;
          bpfset();
          break;

        case 6: //Переключение PowerDoubler.
          general_setting.ALC_RX = !general_setting.ALC_RX;
          bpfset();
          break;

        case 7: //Переключение ATT
          general_setting.ATT_RX = !general_setting.ATT_RX;
          bpfset();
          break;

        case 20: //Настройка min_freq
          if (newPosition > oldPosition && band_setting.min_freq <= max_hardware_freq) band_setting.min_freq++;
          if (newPosition < oldPosition && band_setting.min_freq >= min_hardware_freq) band_setting.min_freq--;
          band_setting.min_freq = constrain(band_setting.min_freq, min_hardware_freq, band_setting.max_freq - 1UL);
          break;

        case 21: //Настройка maxfreq
          if (newPosition > oldPosition && band_setting.max_freq <= max_hardware_freq) band_setting.max_freq++;
          if (newPosition < oldPosition && band_setting.max_freq >= min_hardware_freq) band_setting.max_freq--;
          band_setting.max_freq = constrain(band_setting.max_freq, band_setting.min_freq + 1UL, max_hardware_freq);
          break;

        case 22: // Настройка количества каналов
          if (newPosition > oldPosition && general_setting.number_of_bands < max_number_of_bands) general_setting.number_of_bands++;
          if (newPosition < oldPosition && general_setting.number_of_bands > 0) general_setting.number_of_bands--;
          if (general_setting.number_of_bands > max_number_of_bands) general_setting.number_of_bands = max_number_of_bands;
          if (band > general_setting.number_of_bands) band = general_setting.number_of_bands;
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
          if (newPosition > oldPosition && general_setting.Si_Xtall_Freq <= Si_Xtall_MaxFreq) general_setting.Si_Xtall_Freq += arraystp[general_setting.stp];
          if (newPosition < oldPosition && general_setting.Si_Xtall_Freq >= Si_Xtall_MinFreq) general_setting.Si_Xtall_Freq -= arraystp[general_setting.stp];
          general_setting.Si_Xtall_Freq = constrain(general_setting.Si_Xtall_Freq, Si_Xtall_MinFreq, Si_Xtall_MaxFreq);
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

        case 107: //Настройка границы b1 переключения ФНЧ передатчика * 100кГц
          if (newPosition > oldPosition && general_setting.b1 <= general_setting.b2 - 1) general_setting.b1++;
          if (newPosition < oldPosition && general_setting.b1 >= min_hardware_freq + 1) general_setting.b1--;
          general_setting.b1 = constrain(general_setting.b1, min_hardware_freq + 1, general_setting.b2 - 1);
          bpfset();
          break;

        case 108: //Настройка границы b2 переключения ФНЧ передатчика * 100кГц
          if (newPosition > oldPosition && general_setting.b2 <= general_setting.b3 - 1) general_setting.b2++;
          if (newPosition < oldPosition && general_setting.b2 >= general_setting.b1 + 1) general_setting.b2--;
          general_setting.b2 = constrain(general_setting.b2, general_setting.b1 + 1, general_setting.b3 - 1);
          bpfset();
          break;

        case 109: //Настройка границы b3 переключения ФНЧ передатчика * 100кГц
          if (newPosition > oldPosition && general_setting.b3 <= general_setting.b4 - 1) general_setting.b3++;
          if (newPosition < oldPosition && general_setting.b3 >= general_setting.b2 + 1) general_setting.b3--;
          general_setting.b3 = constrain(general_setting.b3, general_setting.b2 + 1, general_setting.b4 - 1);
          bpfset();
          break;

        case 110: //Настройка границы b4 переключения ФНЧ передатчика * 100кГц
          if (newPosition > oldPosition && general_setting.b4 <= 252) general_setting.b4++;
          if (newPosition < oldPosition && general_setting.b4 >= general_setting.b3 + 1) general_setting.b4--;
          general_setting.b4 = constrain(general_setting.b4, general_setting.b3 + 1, 252);
          bpfset();
          break;

        case 111: // Резервное копирование настроек
          if (newPosition > oldPosition && backup_index < 10) backup_index++;
          if (newPosition < oldPosition && backup_index > 0) backup_index--;
          if (backup_index == 10) {
            //oldPosition = newPosition;
            backup();
          }
          break;

        case 112: // Резервное восстановение настроек
          if (newPosition > oldPosition && restore_index < 10) restore_index++;
          if (newPosition < oldPosition && restore_index > 0) restore_index--;
          if (restore_index == 10) {
            //oldPosition = newPosition;
            restore();
          }
          break;

      }
      if (!general_setting.cmode)actencf = true;
      if (menu && general_setting.cmode)actencf = true;
      mainscreen();
    }
    actenc = millis();
    oldPosition = newPosition;
  }
}

void powermeter () { // Измеритель уровня выхода
  //fwdpower = constrain(analogRead(fwdpin), 1, 1023);
  //revpower = constrain(analogRead(revpin), 1, 1023);
  analogRead(fwdpin);
  fwdpower = (12 * fwdpower + 4 * (analogRead(fwdpin) + 1)) >> 4;
  analogRead(revpin);
  revpower = (12 * revpower + 4 * (analogRead(revpin) + 1)) >> 4;
}

void battmeter () { // Измеритель напряжения питания
  //rawbatt = analogRead(mybattpin);
  rawbatt = (14 * rawbatt + 2 * (analogRead(mybattpin))) >> 4; // Крутой фильтр для усреднения показаний вольтметра!!!
  mybatt = map(rawbatt, 0, 1023, 0, general_setting.batt_cal);
}

void mainscreen() { //Процедура рисования главного экрана
  display.clear();
  display.home();
  //display.setTextColor(WHITE);
  display.setScale(3);
  if (menu) display.setContrast(FullContrast);
  switch (menu) {

    case 0: //Если не в меню, то рисовать главный экран
      //Вывод частоты на дисплей
      if ((band_setting.vfo_freq / 1000000) < 10) display.print(F(" "));
      display.print(band_setting.vfo_freq / 1000000);//Вывод МГц
      display.setCursorXY(41, 0); //Переводим курсор чуть правее текущего положения
      if ((band_setting.vfo_freq % 1000000) / 1000 < 100) display.print(F("0"));
      if ((band_setting.vfo_freq % 1000000) / 1000 < 10) display.print(F("0"));
      display.print((band_setting.vfo_freq % 1000000) / 1000); //Выводим КГц
      display.setScale(2); // Для сотен и десятков герц делаем шрифт поменьше
      display.setCursorXY(100, 7); //Переводим курсор чуть ниже текущего положения
      if ((band_setting.vfo_freq % 1000) / 10 < 10) display.print(F("0")); //Если герц <10 то выводим "0" перед ними.
      display.print((band_setting.vfo_freq % 1000) / 10);

      display.setScale(1);// Ставим маленький шрифт
      display.setCursorXY(0, 24); //Выводим вторую строку на дисплей
      if (mybatt - 100 < 0) display.print(F(" "));
      display.print(mybatt / 10);
      display.print(F("."));
      display.print(mybatt % 10);
      display.print(F(" "));
      //display.setTextSize(1);
      //if (ptten) display.print(F("PTT"));
      //if (cwtxen) display.print(F("CWtxen"));

      if (txen) {//Если передача, то вывод показометра мощности
        if (general_setting.PowerDoubler) {
          display.print(F("В"));
        }
        else {
          display.print(F("Н"));
        }
        // Если оба детектора показывают ноль, числовое значение КСВ не выводим.
        if (fwdpower || revpower) {
          // Сначала сравниваем значения: беззнаковое вычитание при REV > FWD переполняется.
          if (fwdpower <= revpower) {
            display.print(F("---"));
          }
          else {
            // Считаем КСВ в сотых долях, используя 32 бита для суммы и произведения.
            uint32_t swr100 = (((uint32_t)fwdpower + (uint32_t)revpower) * 100UL) /
                              ((uint32_t)fwdpower - (uint32_t)revpower);

            if (swr100 <= 999UL) {
              // До 9.99 сохраняем прежнюю точность — две цифры после точки.
              if ((swr100 / 100UL) < 10UL) display.print(F(" "));
              display.print(swr100 / 100UL);
              display.print(F("."));
              uint8_t swrFraction = (uint8_t)(swr100 % 100UL);
              if (swrFraction < 10) display.print(F("0"));
              display.print(swrFraction);
            }
            else {
              // Выше 9.99 округляем до десятых: прибавляем 5 сотых перед делением на 10.
              uint32_t swr10 = (swr100 + 5UL) / 10UL;

              // Не даём отображаемому значению превысить 99.9.
              if (swr10 > 999UL) swr10 = 999UL;

              display.print(swr10 / 10UL);
              display.print(F("."));
              display.print(swr10 % 10UL);
            }
          }
        }
        display.rect(x00, 23, x00 + (map(fwdpower, 1, 1023, 0, 128)), 26, OLED_FILL);
        display.rect(x00, 28, x00 + (map(revpower, 1, 1023, 0, 128)), 31, OLED_FILL);
      }
      else {// Если прием, то рисовать температуру часы, полосу и диапазон
        //char ddot
        if (temperature >= -50 && temperature <= 50) {
          if (temperature >= 0) display.print(F(" "));
          display.print(temperature);
          drawIcon8x8(1);
          display.print(F("C"));
        }
        else {
          display.print(F(" err "));
        }
        if (actencf) {
          display.print(F(" "));
          display.setContrast(FullContrast);
        }
        else {
          display.print(F("."));
          display.setContrast(LowContrast);
        }
        if (tm.Hour < 10) display.print(F(" "));
        display.print(tm.Hour);
        if (tm.Second % 2) {
          display.print(F(":"));
        }
        else {
          display.print(F(" "));
        }
        if (tm.Minute < 10) display.print(F("0"));
        display.print(tm.Minute);
        if (band_setting.mode) {
          display.print(F(" В"));
        }
        else {
          display.print(F(" Н"));
        }
        if (band < 10) display.print(F(" "));
        display.print(band);
      }
      break;

    case 1: //Меню 1 - канал
      if (band < 10)display.print(F(" ")); // Если номер канала меньше 10 добавить пробел
      display.print(band); // выводим номер канала
      display.setScale(2); // Делаем меньше шрифт
      display.setCursorXY(41, 7);
      if ((band_setting.vfo_freq / 1000000) < 10) display.print(F(" ")); // Если частота меньше 10 МГц, в начале пробел
      display.print(band_setting.vfo_freq / 1000000); // Вывод МГц
      display.setCursorXY(68, 7);
      if ((band_setting.vfo_freq % 1000000) / 1000 < 100) display.print(F("0"));
      if ((band_setting.vfo_freq % 1000000) / 1000 < 10) display.print(F("0"));
      display.print((band_setting.vfo_freq % 1000000) / 1000); //Выводим КГц
      display.setScale(1); // Для сотен и десятков герц делаем шрифт поменьше
      display.setCursor(108, 0); //Переводим курсор чуть выше текущего положения
      if (band_setting.mode) {
        display.print(F("ВБП"));
      }
      else
      {
        display.print(F("НБП"));
      }
      display.setCursorXY(108, 14);
      if ((band_setting.vfo_freq % 1000) / 10 < 10) display.print(F("0")); //Если герц <10 то выводим "0" перед ними.
      display.print((band_setting.vfo_freq % 1000) / 10);
      display.setCursorXY(0, 24); // Переносим курсор на место начала второй строки
      display.print(menu);

      display.print(F(" Выб. КАНАЛ 0 <=> "));
      display.print(general_setting.number_of_bands);
      break;

    case 2: //Меню 2 - шаг настройки
      display.println(arraystp[general_setting.stp] * 10UL);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Шаг Гц"));
      break;

    case 3: //Меню 3 - LSB|USB
      if (band_setting.mode) {
        display.println(F("ВБП"));
      }
      else
      {
        display.println(F("НБП"));
      }
      display.setScale(1);
      display.print(menu);
      display.println(F(" Выбор ВБП/НБП"));
      break;

    case 4: //Настройка IF - Shift
      display.println(general_setting.if_shift);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Сдвиг ПЧ "));
      drawIcon8x8(0);
      display.println(F("Гц"));
      break;

    case 5: // Hi-Lo power
      if (general_setting.PowerDoubler) {
        display.println(F("Высокая"));
      }
      else
      {
        display.println(F("Низкая"));
      }
      display.setScale(1);
      display.print(menu);
      display.println(F(" Мощность УМ"));
      break;

    case 6: // ALC-RX
      if (general_setting.ALC_RX) {
        display.println(F("Выкл"));
      }
      else
      {
        display.println(F("Вкл"));
      }
      display.setScale(1);
      display.print(menu);
      display.println(F(" АРУ ПРМ"));
      break;

    case 7: // ATT_RX
      if (general_setting.ATT_RX) {
        display.println(F("On"));
      }
      else
      {
        display.println(F("Off"));
      }
      display.setScale(1);
      display.print(menu);
      display.println(F(" ATT-RX On/Off"));
      break;

    //-----------------------------USER MENU Display-------------------------//
    case 20: //Настройка min_freq
      display.println(band_setting.min_freq * 100);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Мин. Частота "));
      drawIcon8x8(0);
      display.print(F("кГц"));
      break;

    case 21: //Настройка maxfreq
      display.println(band_setting.max_freq * 100);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Макс. Частота "));
      drawIcon8x8(0);
      display.print(F("кГц"));
      break;

    case 22: //Количество каналов
      display.println(general_setting.number_of_bands);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Всего каналов"));
      break;


    case 23: //Меню 12 - Настройка Часов
      if (tm.Hour < 10) display.print(F("0"));
      display.println(tm.Hour);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Уст. Часов"));
      break;

    case 24: //Меню 13 - Настройка Минут
      if (tm.Minute < 10) display.print(F("0"));
      display.println(tm.Minute);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Уст. Минут"));
      break;

    case 25: //Меню 17 - CW-Delay
      display.println(general_setting.cwdelay * 10);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Задержка ТЛГ"));
      break;

    case 26: //Меню 18 - CW-Tone
      display.println(general_setting.cwtone * 10);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Тон ТЛГ Гц"));
      break;


    case 27: //Меню 15 - Encoder Divider
      display.println(general_setting.mem_enc_div);
      display.setScale(1);
      display.print(menu);
      display.print(F(" !Делитель энкодера!"));
      break;

    case 28: //Меню 14 - Reverse Encoder
      if (general_setting.reverse_encoder) {
        display.println(F("Да"));
      }
      else
      {
        display.println(F("Нет"));
      }
      display.setScale(1);
      display.print(menu);
      display.println(F(" Реверс энкодера"));
      break;

    //-------------------------------------SETUP MENU DISPLAY--------------------------------------//

    case 100: //Channel mode
      if (general_setting.cmode) {
        display.println(F("Да"));
      }
      else
      {
        display.println(F("Нет"));
      }
      display.setScale(1);
      display.print(menu);
      display.println(F(" Вкл кан. режим"));
      break;


    case 101: //Меню 10 - Настройка калибровки по питанию
      display.println(general_setting.batt_cal);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Батарея "));
      if (mybatt - 100 < 0) display.print(F("0"));
      display.print(mybatt / 10);
      display.print(F("."));
      display.print(mybatt % 10);
      display.print(F(" В"));
      break;

    case 102: //Калибровка термодатчика
      display.println(general_setting.temp_cal);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Температура"));
      drawIcon8x8(1);
      drawIcon8x8(0);
      display.print(F("C"));
      break;

    case 103: //Настройка калибровки кварца
      display.setScale(2);
      display.println(general_setting.Si_Xtall_Freq);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Калибровка кварца"));
      drawIcon8x8(0);
      display.print(F("Гц"));
      break;


    case 104: //Настройка LO LSB
      display.setScale(2);
      display.println(general_setting.lsb_lo_freq);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Опора НБП"));
      drawIcon8x8(0);
      display.print(F("Гц"));
      break;


    case 105: //Настройка LO USB
      display.setScale(2);
      display.println(general_setting.usb_lo_freq);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Опора ВБП"));
      drawIcon8x8(0);
      display.print(F("Гц"));
      break;

    case 106: //Настройка BFO
      display.setScale(2);
      display.println(general_setting.bfo_freq);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Опора 2ПЧ "));
      drawIcon8x8(0);
      display.print(F("Гц"));
      break;

    case 107: //Настройка b1 bpf
      display.println(general_setting.b1 * 100);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Граница ДПФ1 "));
      display.print(F("kHz"));
      break;

    case 108: //Настройка b2 bpf
      display.println(general_setting.b2 * 100);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Граница ДПФ2 "));
      display.print(F("kHz"));
      break;

    case 109: //Настройка b3 bpf
      display.println(general_setting.b3 * 100);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Граница ДПФ3 "));
      display.print(F("kHz"));
      break;

    case 110: //Настройка b4 bpf
      display.println(general_setting.b4 * 100);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Граница ДПФ4 "));
      display.print(F("kHz"));
      break;

    case 111: //Backup Setting
      display.println(backup_index);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Резерв => 10"));
      break;

    case 112: //Restore setting
      display.println(restore_index);
      display.setScale(1);
      display.print(menu);
      display.print(F(" Восстанов => 10"));
      break;
  }
  display.update();
  //debug();

}


void vfosetup() {
  if (txen) {
    if (band_setting.mode) {
      si.set_freq((band_setting.vfo_freq + general_setting.bfo_freq + general_setting.usb_lo_freq), general_setting.usb_lo_freq, general_setting.bfo_freq);
    }
    else {
      si.set_freq((band_setting.vfo_freq + general_setting.bfo_freq + general_setting.lsb_lo_freq), general_setting.lsb_lo_freq, general_setting.bfo_freq);
    }
    bpfset();
  }
  else {
    if (band_setting.mode) {
      si.set_freq((band_setting.vfo_freq + general_setting.bfo_freq + general_setting.usb_lo_freq + general_setting.if_shift), (general_setting.usb_lo_freq + general_setting.if_shift), general_setting.bfo_freq);
    }
    else {
      si.set_freq((band_setting.vfo_freq + general_setting.bfo_freq + general_setting.lsb_lo_freq + general_setting.if_shift), (general_setting.lsb_lo_freq + general_setting.if_shift), general_setting.bfo_freq);
    }
    bpfset();
  }
}


void si5351init() {
  si.setup(3, 3, 3);
  si.cload(si_cload);
}

void si5351correction() {
  si.set_xtal_freq(general_setting.Si_Xtall_Freq);
  si.update_freq(0);
  si.update_freq(1);
  //si.update_freq(2);
}


/*
 * Кольцевая память AT24C32.
 * Общие настройки (0..39) сохраняют прежний формат и прежнюю CRC.
 * Диапазоны 0..30: 31 кольцо по 128 байт, запись 15 байт.
 * Выбор диапазона: отдельное кольцо 88 байт, запись 3 байта.
 * Внешний интерфейс: memory(MEM_READ) и memory(MEM_WRITE).
 */

static uint8_t memoryGeneralBand = 0;
static bool memoryInitialized = false;

static bool memoryBandCacheValid = false;
static bool memoryBandHasRecord = false;
static uint8_t memoryCachedBand = 0;
static uint8_t memoryBandLastPos = 0;
static uint8_t memoryBandLastSeq = 0;

static bool memorySelectionHasRecord = false;
static uint8_t memorySelectionLastPos = 0;
static uint8_t memorySelectionLastSeq = 0;

static uint16_t memoryBandBase(uint8_t index) {
  return (uint16_t)(sizeof(general_set) + 2U + (uint16_t)index * BAND_RING_SIZE);
}

static uint16_t memorySelectionBase() {
  return (uint16_t)(sizeof(general_set) + 2U +
                    (uint16_t)BAND_RING_COUNT * BAND_RING_SIZE);
}

static uint8_t memoryRingReadByte(uint16_t base, uint8_t ringSize, uint8_t position) {
  return AT24C32.readByte((word)(base + (position % ringSize)));
}

static void memoryRingWriteByte(uint16_t base, uint8_t ringSize,
                                uint8_t position, uint8_t value) {
  AT24C32.writeEE((word)(base + (position % ringSize)), value);
}

// CRC-8: полином 0x07, init=0, без отражения, xorout=0.
static uint8_t memoryCrc8(const uint8_t *data, uint8_t length) {
  uint8_t crc = 0;
  while (length--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x07);
      else crc <<= 1;
    }
  }
  return crc;
}

// Установка и проверка CRC-8 в последнем байте записи.
static void memorySetRecordCrc(uint8_t *record, uint8_t dataLength) {
  record[dataLength] = memoryCrc8(record, dataLength);
}

static bool memoryCheckRecordCrc(const uint8_t *record, uint8_t dataLength) {
  return memoryCrc8(record, dataLength) == record[dataLength];
}

// Общий формат диапазонной записи: сигнатура, band_set, CRC-8.
static void memoryMakeBandRecord(uint8_t *record, uint8_t sequence,
                                 const band_set &settings) {
  record[0] = (uint8_t)(0xA0 | (sequence & 0x0F));

  const uint8_t *source = (const uint8_t *)&settings;
  for (uint8_t i = 0; i < sizeof(settings); i++) {
    record[i + 1] = source[i];
  }

  memorySetRecordCrc(record, BAND_RECORD_SIZE - 1);
}

// Общий формат записи выбора диапазона: сигнатура+band, sequence, CRC-8.
static void memoryMakeSelectionRecord(uint8_t *record, uint8_t selectedBand,
                                      uint8_t sequence) {
  record[0] = (uint8_t)(0xA0 | (selectedBand & 0x1F));
  record[1] = sequence;
  memorySetRecordCrc(record, BAND_LOG_RECORD_SIZE - 1);
}

static bool memorySequenceNewer4(uint8_t candidate, uint8_t current) {
  uint8_t difference = (uint8_t)((candidate - current) & 0x0F);
  return difference != 0 && difference < 8;
}

static bool memorySequenceNewer8(uint8_t candidate, uint8_t current) {
  uint8_t difference = (uint8_t)(candidate - current);
  return difference != 0 && difference < 128;
}

// Старое поле general_setting.band остаётся неизменным в основной области EEPROM.
static void memorySaveGeneralSettings() {
  uint8_t bandField = general_setting.band;
  general_setting.band = memoryGeneralBand;

  int16_t crc = 0;
  const uint8_t *address = (const uint8_t *)&general_setting;
  for (uint8_t i = 0; i < sizeof(general_setting); i++) crc += address[i];

  AT24C32.writeEE(2, general_setting);
  AT24C32.writeEE(0, (int16_t)(crc + crcmod));

  general_setting.band = bandField;
}

static void memoryLoadGeneralSettings() {
  int16_t storedCrc = 0;
  int16_t calculatedCrc = 0;

  AT24C32.readEE(0, storedCrc);
  for (uint8_t i = 0; i < sizeof(general_setting); i++) {
    calculatedCrc += AT24C32.readByte((word)(i + 2U));
  }

  if (storedCrc == (int16_t)(calculatedCrc + crcmod)) {
    AT24C32.readEE(2, general_setting);
  }

  // При пустой или повреждённой EEPROM остаются начальные значения структуры.
  memoryGeneralBand = general_setting.band;
  if (storedCrc != (int16_t)(calculatedCrc + crcmod)) memorySaveGeneralSettings();
}

// Один проход по всем возможным начальным адресам кольца текущего диапазона.
static bool memoryFindBandRecord(uint8_t targetBand, bool loadSettings,
                                  uint8_t *latestRecord) {
  const uint16_t base = memoryBandBase(targetBand);
  uint8_t record[BAND_RECORD_SIZE];
  bool found = false;
  uint8_t bestSequence = 0;
  uint8_t bestPosition = 0;

  for (uint8_t position = 0; position < BAND_RING_SIZE; position++) {
    record[0] = memoryRingReadByte(base, BAND_RING_SIZE, position);
    if ((record[0] & 0xF0) != 0xA0) continue;

    for (uint8_t i = 1; i < BAND_RECORD_SIZE; i++) {
      record[i] = memoryRingReadByte(base, BAND_RING_SIZE,
                                     (uint8_t)((position + i) % BAND_RING_SIZE));
    }

    if (!memoryCheckRecordCrc(record, BAND_RECORD_SIZE - 1)) continue;

    uint8_t sequence = (uint8_t)(record[0] & 0x0F);
    if (!found || memorySequenceNewer4(sequence, bestSequence)) {
      found = true;
      bestSequence = sequence;
      bestPosition = position;

      // При необходимости отдаём наружу полную запись в исходном формате.
      if (latestRecord) {
        for (uint8_t i = 0; i < BAND_RECORD_SIZE; i++) {
          latestRecord[i] = record[i];
        }
      }

      // В рабочую структуру переносим только запись с верной CRC.
      if (loadSettings) {
        uint8_t *destination = (uint8_t *)&band_setting;
        for (uint8_t i = 0; i < sizeof(band_setting); i++) {
          destination[i] = record[i + 1];
        }
      }
    }
  }

  memoryBandCacheValid = true;
  memoryCachedBand = targetBand;
  memoryBandHasRecord = found;
  memoryBandLastPos = bestPosition;
  memoryBandLastSeq = bestSequence;
  return found;
}

// Сначала пишутся 14 информационных байтов, CRC — последней.
// Затем запись перечитывается для проверки результата.
static bool memoryStoreBandRecord(uint8_t targetBand,
                                  uint8_t position,
                                  uint8_t sequence) {
  const uint16_t base = memoryBandBase(targetBand);
  uint8_t record[BAND_RECORD_SIZE];
  uint8_t verify[BAND_RECORD_SIZE];

  memoryMakeBandRecord(record, sequence, band_setting);

  for (uint8_t i = 0; i < BAND_RECORD_SIZE - 1; i++) {
    memoryRingWriteByte(base, BAND_RING_SIZE,
                        (uint8_t)((position + i) % BAND_RING_SIZE), record[i]);
  }
  memoryRingWriteByte(base, BAND_RING_SIZE,
                      (uint8_t)((position + BAND_RECORD_SIZE - 1) % BAND_RING_SIZE),
                      record[BAND_RECORD_SIZE - 1]);

  for (uint8_t i = 0; i < BAND_RECORD_SIZE; i++) {
    verify[i] = memoryRingReadByte(base, BAND_RING_SIZE,
                                   (uint8_t)((position + i) % BAND_RING_SIZE));
    if (verify[i] != record[i]) return false;
  }
  if (!memoryCheckRecordCrc(verify, BAND_RECORD_SIZE - 1)) return false;

  memoryBandCacheValid = true;
  memoryBandHasRecord = true;
  memoryCachedBand = targetBand;
  memoryBandLastPos = position;
  memoryBandLastSeq = (uint8_t)(sequence & 0x0F);
  return true;
}

// Однопроходный поиск последнего выбора диапазона в кольце №31.
static bool memoryFindSelectionRecord() {
  const uint16_t base = memorySelectionBase();
  uint8_t record[BAND_LOG_RECORD_SIZE];
  bool found = false;
  uint8_t bestSequence = 0;
  uint8_t bestPosition = 0;
  uint8_t bestBand = band;

  for (uint8_t position = 0; position < BAND_LOG_RING_SIZE; position++) {
    record[0] = memoryRingReadByte(base, BAND_LOG_RING_SIZE, position);
    if ((record[0] & 0xE0) != 0xA0) continue;

    uint8_t candidateBand = (uint8_t)(record[0] & 0x1F);
    if (candidateBand > max_number_of_bands) continue;

    record[1] = memoryRingReadByte(base, BAND_LOG_RING_SIZE,
                                   (uint8_t)((position + 1) % BAND_LOG_RING_SIZE));
    record[2] = memoryRingReadByte(base, BAND_LOG_RING_SIZE,
                                   (uint8_t)((position + 2) % BAND_LOG_RING_SIZE));
    if (!memoryCheckRecordCrc(record, BAND_LOG_RECORD_SIZE - 1)) continue;

    if (!found || memorySequenceNewer8(record[1], bestSequence)) {
      found = true;
      bestSequence = record[1];
      bestPosition = position;
      bestBand = candidateBand;
    }
  }

  memorySelectionHasRecord = found;
  memorySelectionLastPos = bestPosition;
  memorySelectionLastSeq = bestSequence;
  if (found) band = bestBand;
  return found;
}

// Служебная запись: сигнатура+Band, номер события, CRC последним байтом.
static bool memoryStoreSelectionRecord(uint8_t position, uint8_t sequence) {
  const uint16_t base = memorySelectionBase();
  uint8_t record[BAND_LOG_RECORD_SIZE];
  uint8_t verify[BAND_LOG_RECORD_SIZE];

  memoryMakeSelectionRecord(record, band, sequence);

  memoryRingWriteByte(base, BAND_LOG_RING_SIZE, position, record[0]);
  memoryRingWriteByte(base, BAND_LOG_RING_SIZE,
                      (uint8_t)((position + 1) % BAND_LOG_RING_SIZE), record[1]);
  memoryRingWriteByte(base, BAND_LOG_RING_SIZE,
                      (uint8_t)((position + 2) % BAND_LOG_RING_SIZE), record[2]);

  for (uint8_t i = 0; i < BAND_LOG_RECORD_SIZE; i++) {
    verify[i] = memoryRingReadByte(base, BAND_LOG_RING_SIZE,
                                   (uint8_t)((position + i) % BAND_LOG_RING_SIZE));
    if (verify[i] != record[i]) return false;
  }
  if (!memoryCheckRecordCrc(verify, BAND_LOG_RECORD_SIZE - 1)) return false;

  memorySelectionHasRecord = true;
  memorySelectionLastPos = position;
  memorySelectionLastSeq = sequence;
  return true;
}

// Одна универсальная процедура: чтение или запись выбранного набора данных.
bool memory(uint8_t command) {
  if (command == MEM_READ) {
    if (!memoryInitialized) {
      memoryLoadGeneralSettings();
      band = general_setting.band;
      if (band > max_number_of_bands) band = 0;

      // Если журнал выбора пуст, сохраняем исходный band в позиции 0.
      if (!memoryFindSelectionRecord()) memoryStoreSelectionRecord(0, 0);
      memoryInitialized = true;
    }

    if (band > max_number_of_bands) band = 0;

    // Если кольцо диапазона пустое, записываем туда текущее band_setting:
    // повторно структуру не инициализируем, наследуем прежнее поведение.
    if (!memoryFindBandRecord(band, true, NULL)) {
      return memoryStoreBandRecord(band, 0, 0);
    }
    return true;
  }

  if (command == MEM_WRITE) {
    if (!memoryInitialized) return false;

    memorySaveGeneralSettings();

    // Для Restore или смены band вне MEM_READ ищем позицию без загрузки данных.
    if (!memoryBandCacheValid || memoryCachedBand != band) {
      memoryFindBandRecord(band, false, NULL);
    }

    uint8_t bandPosition = memoryBandHasRecord
        ? (uint8_t)((memoryBandLastPos + BAND_RECORD_SIZE) % BAND_RING_SIZE) : 0;
    uint8_t bandSequence = memoryBandHasRecord
        ? (uint8_t)((memoryBandLastSeq + 1) & 0x0F) : 0;

    if (!memoryStoreBandRecord(band, bandPosition, bandSequence)) return false;

    uint8_t selectionPosition = memorySelectionHasRecord
        ? (uint8_t)((memorySelectionLastPos + BAND_LOG_RECORD_SIZE) % BAND_LOG_RING_SIZE) : 0;
    uint8_t selectionSequence = memorySelectionHasRecord
        ? (uint8_t)(memorySelectionLastSeq + 1) : 0;

    return memoryStoreSelectionRecord(selectionPosition, selectionSequence);
  }

  return false;
}


void versionprint() {
  display.clear();
  display.home();
  //display.setTextColor(WHITE);
  display.setScale(3);
  if (menu == 100) {
    display.println(F("Setup"));
    display.update();
    while (!digitalRead(myEncBtn));
  }
  else {
    display.println(ver);
    display.setScale(1);
    display.println(F(" UD0DAB 2026 год."));
    display.update();
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
            bpfset();
          }
          vfosetup();
          state = false;

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
          digitalWrite (rxenpin, HIGH);
          bpfset();
        }
      }
    }
  }
}

void rxtxcontrol() {
  if (!txen && state) {
    //digitalWrite (rxenpin, LOW);
    digitalWrite (txenpin, HIGH);
  }
  if (txen && !state) {
    digitalWrite (txenpin, LOW);
    digitalWrite (rxenpin, LOW);
  }
}

void bpfset() {
  //if (!bpfUpdate) {
  byteToSend = 0;
  bpf_set = 0;
  unsigned int bpf_freq = band_setting.vfo_freq / 100000;
  if (bpf_freq >= general_setting.b1) bpf_set = 1;
  if (bpf_freq >= general_setting.b2) bpf_set = 2;
  if (bpf_freq >= general_setting.b3) bpf_set = 3;
  if (bpf_freq >= general_setting.b4) bpf_set = 4;
  bitSet(byteToSend, bpf_set); // Установка нужного бита управления ДПФ
  if (txen && general_setting.PowerDoubler && !toneen) bitSet(byteToSend, 5); //Установка разрешения работы удвоителя
  if (general_setting.ALC_RX)bitSet(byteToSend, 6); //Установка разрешения работы АРУ
  if (general_setting.ATT_RX)bitSet(byteToSend, 7); //Установка разрешения работы ATT
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, byteToSend);
  digitalWrite(latchPin, HIGH);
  //bpfUpdate = true;
  //}
}

// Адрес начала выбранного слота внутренней EEPROM.
static uint16_t backupSlotBase(uint8_t slot) {
  return (uint16_t)((slot - 1U) * BackupMap::SlotSize);
}

static bool backupSlotNumberValid(uint8_t slot) {
  return slot == 1 || slot == 2;
}

static bool backupInternalEepromAvailable() {
  return EEPROM.length() >= BackupMap::TotalSize;
}

static void backupWriteByte(uint16_t address, uint8_t value) {
  EEPROM.update(address, value);
}

static uint8_t backupReadByte(uint16_t address) {
  return EEPROM.read(address);
}

// CRC-16/CCITT-FALSE: poly=0x1021, init=0xFFFF, refin/refout=false, xorout=0.
static uint16_t backupCalculateCrc16(uint16_t base, uint16_t length) {
  uint16_t crc = 0xFFFF;

  for (uint16_t i = 0; i < length; i++) {
    crc ^= (uint16_t)backupReadByte((uint16_t)(base + i)) << 8;

    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x8000) crc = (uint16_t)((crc << 1) ^ 0x1021);
      else crc <<= 1;
    }
  }

  return crc;
}

static bool backupRecordIsEmpty(const uint8_t *record) {
  for (uint8_t i = 0; i < BAND_RECORD_SIZE; i++) {
    if (record[i] != 0xFF) return false;
  }
  return true;
}

static void backupReadBandRecord(uint16_t slotBase, uint8_t index,
                                 uint8_t *record) {
  const uint16_t address = (uint16_t)(slotBase + BackupMap::RingDataOffset +
                                      (uint16_t)index * BAND_RECORD_SIZE);
  for (uint8_t i = 0; i < BAND_RECORD_SIZE; i++) {
    record[i] = backupReadByte((uint16_t)(address + i));
  }
}

static void backupWriteBandRecord(uint16_t slotBase, uint8_t index,
                                  const uint8_t *record) {
  const uint16_t address = (uint16_t)(slotBase + BackupMap::RingDataOffset +
                                      (uint16_t)index * BAND_RECORD_SIZE);
  for (uint8_t i = 0; i < BAND_RECORD_SIZE; i++) {
    backupWriteByte((uint16_t)(address + i), record[i]);
  }
}

// Проверяем, что всё кольцо действительно пустое, а не просто не содержит
// ни одной записи с правильной CRC.
static bool backupBandRingIsErased(uint8_t targetBand) {
  const uint16_t base = memoryBandBase(targetBand);

  for (uint8_t position = 0; position < BAND_RING_SIZE; position++) {
    if (memoryRingReadByte(base, BAND_RING_SIZE, position) != 0xFF) return false;
  }
  return true;
}

static void backupRestoreActiveBandCache(uint8_t activeBand, bool hasRecord,
                                         uint8_t lastPosition,
                                         uint8_t lastSequence) {
  memoryBandCacheValid = true;
  memoryBandHasRecord = hasRecord;
  memoryCachedBand = activeBand;
  memoryBandLastPos = lastPosition;
  memoryBandLastSeq = lastSequence;
}

static uint16_t backupGeneralChecksum(const general_set &settings) {
  int16_t sum = 0;
  const uint8_t *data = (const uint8_t *)&settings;

  for (uint8_t i = 0; i < sizeof(settings); i++) sum += data[i];

  return (uint16_t)(sum + crcmod);
}

// Проверка только выбранного слота. Внешняя AT24C32 здесь не затрагивается.
static bool backupValidateSlot(uint8_t slot) {
  if (!backupSlotNumberValid(slot) || !backupInternalEepromAvailable()) return false;

  const uint16_t base = backupSlotBase(slot);

  if (backupReadByte((uint16_t)(base + BackupMap::MetaOffset)) != BackupMap::Signature) return false;
  if (backupReadByte((uint16_t)(base + BackupMap::MetaOffset + 1U)) != BackupMap::FormatVersion) return false;
  if (backupReadByte((uint16_t)(base + BackupMap::MetaOffset + 2U)) != slot) return false;
  if (backupReadByte((uint16_t)(base + BackupMap::CommitOffset)) != BackupMap::CommitMarker) return false;

  const uint16_t storedCrc =
      (uint16_t)backupReadByte((uint16_t)(base + BackupMap::CrcOffset)) |
      ((uint16_t)backupReadByte((uint16_t)(base + BackupMap::CrcOffset + 1U)) << 8);

  if (backupCalculateCrc16(base, BackupMap::CrcLength) != storedCrc) return false;

  const uint8_t numberOfBands = backupReadByte((uint16_t)(base + BackupMap::GeneralOffset + 2U));
  const uint8_t selectedBand = backupReadByte((uint16_t)(base + BackupMap::SelectedBandOffset));

  if (numberOfBands > max_number_of_bands) return false;
  if (selectedBand > numberOfBands) return false;

  uint8_t record[BAND_RECORD_SIZE];

  for (uint8_t index = 0; index < BAND_RING_COUNT; index++) {
    backupReadBandRecord(base, index, record);
    if (backupRecordIsEmpty(record)) continue;

    if ((record[0] & 0xF0) != 0xA0) return false;
    if (!memoryCheckRecordCrc(record, BAND_RECORD_SIZE - 1)) return false;
  }

  return true;
}

// Единый четырёхстрочный журнал для Backup и Restore.
// Храним только указатели на строки в PROGMEM (F()), сами тексты не копируем в RAM.
static void backupRestoreLogLine(const __FlashStringHelper *message,
                                 bool resetLog = false) {
  static const __FlashStringHelper *lines[4] = {NULL, NULL, NULL, NULL};
  static uint8_t lineCount = 0;

  if (resetLog) {
    for (uint8_t i = 0; i < 4; i++) lines[i] = NULL;
    lineCount = 0;
  }

  if (lineCount < 4) {
    lines[lineCount++] = message;
  } else {
    for (uint8_t i = 0; i < 3; i++) lines[i] = lines[i + 1];
    lines[3] = message;
  }

  display.setScale(1);
  display.clear();

  for (uint8_t i = 0; i < lineCount; i++) {
    display.setCursorXY(0, (int)i * 8);
    display.print(lines[i]);
  }

  display.update();
}

// Создаёт полный образ в указанном слоте. В AT24C32 выполняются только чтения.
void backup(uint8_t slot) {
  backupRestoreLogLine(F("Резерв.."), true);

  if (!backupSlotNumberValid(slot) || !backupInternalEepromAvailable()) {
    backupRestoreLogLine(F("Ошибка слота"));
    delay(1000);
    menu = 0;
    return;
  }

  // Фиксируем RAM-состояние до сканирования остальных диапазонов.
  const general_set savedGeneral = general_setting;
  const band_set savedActiveSettings = band_setting;
  const uint8_t savedBand = band;

  if (savedGeneral.number_of_bands > max_number_of_bands ||
      savedBand > savedGeneral.number_of_bands) {
    backupRestoreLogLine(F("Ошибка настроек"));
    delay(1000);
    menu = 0;
    return;
  }

  // Запоминаем последнюю запись активного кольца. Для снимка из RAM
  // номер последовательности будет следующим, но само кольцо не меняем.
  const bool activeHasRecord = memoryFindBandRecord(savedBand, false, NULL);
  const uint8_t activeLastPosition = memoryBandLastPos;
  const uint8_t activeLastSequence = memoryBandLastSeq;

  bool ringsValid = true;

  if (!activeHasRecord && !backupBandRingIsErased(savedBand)) ringsValid = false;

  // Предварительно убеждаемся, что остальные кольца либо содержат
  // корректную запись, либо действительно полностью пусты.
  for (uint8_t index = 0; index < BAND_RING_COUNT; index++) {
    if (index == savedBand) continue;

    if (!memoryFindBandRecord(index, false, NULL) && !backupBandRingIsErased(index)) {
      ringsValid = false;
      break;
    }
  }

  backupRestoreActiveBandCache(savedBand, activeHasRecord,
                               activeLastPosition, activeLastSequence);

  if (!ringsValid) {
    backupRestoreLogLine(F("Ошибка памяти AT24C32"));
    delay(1000);
    menu = 0;
    return;
  }

  const uint16_t base = backupSlotBase(slot);

  // Сначала делаем выбранный слот недействительным. Маркер 0xA5
  // будет записан последним, после данных и CRC16.
  backupWriteByte((uint16_t)(base + BackupMap::CommitOffset), 0x00);

  // Общие настройки копируются прямо из RAM без изменения поля band.
  const uint8_t *generalBytes = (const uint8_t *)&savedGeneral;
  for (uint8_t i = 0; i < sizeof(savedGeneral); i++) {
    backupWriteByte((uint16_t)(base + i), generalBytes[i]);
  }

  backupWriteByte((uint16_t)(base + BackupMap::SelectedBandOffset), savedBand);

  uint8_t record[BAND_RECORD_SIZE];

  for (uint8_t index = 0; index < BAND_RING_COUNT; index++) {
    if (index == savedBand) {
      const uint8_t sequence = activeHasRecord
          ? (uint8_t)((activeLastSequence + 1U) & 0x0F)
          : 0;
      memoryMakeBandRecord(record, sequence, savedActiveSettings);
    }
    else if (!memoryFindBandRecord(index, false, record)) {
      // Повторно убеждаемся, что кольцо не повреждено.
      if (!backupBandRingIsErased(index)) {
        backupRestoreActiveBandCache(savedBand, activeHasRecord,
                                     activeLastPosition, activeLastSequence);
        backupRestoreLogLine(F("Ошибка памяти AT24C32"));
        delay(1000);
        backupWriteByte((uint16_t)(base + BackupMap::CommitOffset), 0x00);
        menu = 0;
        return;
      }
      for (uint8_t i = 0; i < BAND_RECORD_SIZE; i++) record[i] = 0xFF;
    }

    backupWriteBandRecord(base, index, record);
  }

  backupRestoreActiveBandCache(savedBand, activeHasRecord,
                               activeLastPosition, activeLastSequence);

  // Служебные поля образа. Маркер завершения пока не записываем.
  backupWriteByte((uint16_t)(base + BackupMap::MetaOffset), BackupMap::Signature);
  backupWriteByte((uint16_t)(base + BackupMap::MetaOffset + 1U), BackupMap::FormatVersion);
  backupWriteByte((uint16_t)(base + BackupMap::MetaOffset + 2U), slot);

  const uint16_t crc = backupCalculateCrc16(base, BackupMap::CrcLength);
  backupWriteByte((uint16_t)(base + BackupMap::CrcOffset), (uint8_t)(crc & 0xFF));
  backupWriteByte((uint16_t)(base + BackupMap::CrcOffset + 1U), (uint8_t)(crc >> 8));

  // Последняя операция формирования слота — маркер готовности.
  backupWriteByte((uint16_t)(base + BackupMap::CommitOffset), BackupMap::CommitMarker);

  if (!backupValidateSlot(slot)) {
    backupWriteByte((uint16_t)(base + BackupMap::CommitOffset), 0x00);
    backupRestoreLogLine(F("Ошибка бекапа"));
    delay(1000);
    menu = 0;
    return;
  }

  backupRestoreLogLine(F("Готово"));
  delay(1000);

  // Возвращаемся на главный экран и запускаем штатное отложенное сохранение.
  menu = 0;
  actencf = true;
  actenc = millis();
}

static bool backupVerifyRestoredImage(uint16_t slotBase,
                                      const general_set &restoredGeneral,
                                      uint8_t restoredBand) {
  const uint16_t generalChecksum = backupGeneralChecksum(restoredGeneral);

  if (AT24C32.readByte(0) != (uint8_t)(generalChecksum & 0xFF)) return false;
  if (AT24C32.readByte(1) != (uint8_t)(generalChecksum >> 8)) return false;

  const uint8_t *generalBytes = (const uint8_t *)&restoredGeneral;
  for (uint8_t i = 0; i < sizeof(restoredGeneral); i++) {
    if (AT24C32.readByte((word)(i + 2U)) != generalBytes[i]) return false;
  }

  uint8_t expectedRecord[BAND_RECORD_SIZE];
  uint8_t verifyRecord[BAND_RECORD_SIZE];

  // Проверяем все 128 байт каждого кольца, а не только его запись.
  for (uint8_t index = 0; index < BAND_RING_COUNT; index++) {
    backupReadBandRecord(slotBase, index, expectedRecord);
    const bool empty = backupRecordIsEmpty(expectedRecord);
    const uint16_t ringBase = memoryBandBase(index);

    for (uint8_t position = 0; position < BAND_RING_SIZE; position++) {
      const uint8_t expected = (!empty && position < BAND_RECORD_SIZE)
          ? expectedRecord[position]
          : 0xFF;
      const uint8_t actual = AT24C32.readByte((word)(ringBase + position));

      if (actual != expected) return false;
      if (position < BAND_RECORD_SIZE) verifyRecord[position] = actual;
    }

    if (!empty &&
        ((verifyRecord[0] & 0xF0) != 0xA0 ||
         !memoryCheckRecordCrc(verifyRecord, BAND_RECORD_SIZE - 1))) {
      return false;
    }
  }

  // В служебном кольце должна остаться только одна запись выбора диапазона.
  uint8_t selectionRecord[BAND_LOG_RECORD_SIZE];
  memoryMakeSelectionRecord(selectionRecord, restoredBand, 0);

  const uint16_t selectionBase = memorySelectionBase();
  for (uint8_t position = 0; position < BAND_LOG_RING_SIZE; position++) {
    const uint8_t expected = (position < BAND_LOG_RECORD_SIZE)
        ? selectionRecord[position]
        : 0xFF;
    const uint8_t actual = AT24C32.readByte((word)(selectionBase + position));
    if (actual != expected) return false;
  }

  return memoryCheckRecordCrc(selectionRecord, BAND_LOG_RECORD_SIZE - 1);
}

static void restoreHalt(const __FlashStringHelper *message) {
  backupRestoreLogLine(message);
  while (1) {
    // Не запускаем трансивер с непроверенными или неполностью восстановленными данными.
  }
}

// Восстанавливает только явно выбранный слот. При любой ошибке проверки образа
// внешняя AT24C32 остаётся нетронутой.
void restore(uint8_t slot) {
  backupRestoreLogLine(F("Проверка бекапа"), true);

  if (!backupSlotNumberValid(slot)) restoreHalt(F("Неверный номер слота"));
  if (!backupValidateSlot(slot)) restoreHalt(F("Бекап повреждён"));

  const uint16_t base = backupSlotBase(slot);

  // Только после полной проверки выбранного образа разрешается стирание.
  backupRestoreLogLine(F("Стирание AT24C32"));

  for (uint16_t address = 0; address < 4096U; address++) {
    AT24C32.writeEE((word)address, (byte)0xFF);

    // Читаем каждый байт сразу после его записи.
    if (AT24C32.readByte((word)address) != 0xFF) {
      restoreHalt(F("Замените AT24C32"));
    }
  }

  backupRestoreLogLine(F("Запись настроек"));

  general_set restoredGeneral;
  EEPROM.get((uint16_t)(base + BackupMap::GeneralOffset), restoredGeneral);

  const uint8_t restoredBand = backupReadByte((uint16_t)(base + BackupMap::SelectedBandOffset));
  const uint16_t generalChecksum = backupGeneralChecksum(restoredGeneral);

  // Сохраняем прежнюю раскладку: CRC в 0..1, структура в 2..39.
  AT24C32.writeEE(2, restoredGeneral);
  AT24C32.writeEE(0, (int16_t)generalChecksum);

  // В каждое непустое кольцо переносим единственную сохранённую 15-байтовую
  // запись с позиции 0, не меняя её сигнатуру, последовательность и CRC-8.
  uint8_t record[BAND_RECORD_SIZE];

  for (uint8_t index = 0; index < BAND_RING_COUNT; index++) {
    backupReadBandRecord(base, index, record);
    if (!backupRecordIsEmpty(record)) {
      const uint16_t ringBase = memoryBandBase(index);
      for (uint8_t i = 0; i < BAND_RECORD_SIZE; i++) {
        AT24C32.writeEE((word)(ringBase + i), record[i]);
      }
    }
  }

  // Восстанавливаем выбранный диапазон одной штатной записью журнала.
  uint8_t selectionRecord[BAND_LOG_RECORD_SIZE];
  memoryMakeSelectionRecord(selectionRecord, restoredBand, 0);

  const uint16_t selectionBase = memorySelectionBase();
  for (uint8_t i = 0; i < BAND_LOG_RECORD_SIZE; i++) {
    AT24C32.writeEE((word)(selectionBase + i), selectionRecord[i]);
  }

  backupRestoreLogLine(F("Проверка записи"));

  if (!backupVerifyRestoredImage(base, restoredGeneral, restoredBand)) {
    restoreHalt(F("Ошибка проверки"));
  }

  backupRestoreLogLine(F("Восстановлено"));
  delay(1000);

  // После успешной проверки — обязательная перезагрузка, как в исходной версии.
  asm volatile("jmp 0x00");
}

void drawIcon8x8(byte index) {
  size_t s = sizeof icons_8x8[index];//можна так, а можна просто 8
  for (unsigned int i = 0; i < s; i++) {
    display.drawByte(pgm_read_byte(&(icons_8x8[index][i])));
  }
}
