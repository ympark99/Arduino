/*
   IoT SMART KIT
*/

#include <DHT_U.h>
#include <VitconBrokerComm.h>
#include <SoftPWM.h>
#include <U8g2lib.h> //U8g2 by oliver 라이브러리 설치 필요
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

using namespace vitcon;

#define DHTPIN    A1
#define DHTTYPE   DHT22
#define LAMP      15
#define PUMP      14
#define PTC       16
#define UVB       17
#define SOILHUMI  A6

DHT_Unified dht(DHTPIN, DHTTYPE);
SOFTPWM_DEFINE_CHANNEL(A3);  //Arduino pin A3


uint32_t DataCaptureDelay = 4000; //ms
uint32_t StartTime1;
uint32_t StartTime2;
uint32_t StartTime3;
uint32_t StartTime4;
uint32_t StartTime5;

int Soilhumi = 0;
float Temp = 0;
float Humi = 0;
int fanVal = 0;

bool timeset = false;
bool timeset2 = false;
bool autoMode = false;
bool soilstatus = true;
bool tempstatus = false;
bool lampflag = true;
bool auto_1_execute = false;
bool manu_1_execute = false;

bool fan_out_status;
bool pump_out_status;
bool lamp_out_status;
bool uvb_out_status;
bool ptc_out_status;
bool Interval_Minute_Up_status;
bool Interval_Hour_Up_status;
bool Interval_Minute_Up_status2;
bool Interval_Hour_Up_status2;

uint32_t Hour = 0;
uint32_t Minute = 1;
uint32_t TimeSum = 0;
uint32_t TimeStatus;

uint32_t Hour2 = 0;
uint32_t Minute2 = 1;
uint32_t TimeSum2 = 0;
uint32_t TimeStatus2;

/* A set of definition for IOT items */
#define ITEM_COUNT 29

//모드 변경을 위한 함수
void mode_out(bool val) {
  autoMode = val;
}

//Interval 설정 모드로 들어가기 위한 함수
void timeset_out(bool val) {
  timeset = val;
}

void timeset_out2(bool val) {
  timeset2 = val;
}

//Interval 시간 단위를 설정하는 함수(조명)
void Interval_Hup(bool val) {
  Interval_Hour_Up_status = val;
}

//Interval 분 단위를 설정하는 함수(조명)
void Interval_Mup(bool val) {
  Interval_Minute_Up_status = val;
}

//Interval 시간 단위를 설정하는 함수(uvb)
void Interval_Hup2(bool val) {
  Interval_Hour_Up_status2 = val;
}

//Interval 분 단위를 설정하는 함수(uvb)
void Interval_Mup2(bool val) {
  Interval_Minute_Up_status2 = val;
}

//manual mode일 때 FAN을 제어하는 함수
void fan_out(bool val) {
  fan_out_status = val;
}

//manual mode일 때 PUMP를 제어하는 함수
void pump_out(bool val) {
  pump_out_status = val;
}

//manual mode일 때 LAMP를 제어하는 함수
void lamp_out(bool val) {
  lamp_out_status = val;
}

//manual mode일 때 UVB를 제어하는 함수
void uvb_out(bool val) {
  uvb_out_status = val;
}

//manual mode일 때 PTC를 제어하는 함수
void ptc_out(bool val) {
  ptc_out_status = val;
}

//Interval을 0시 0분으로 리셋하는 함수(조명)
void IntervalReset(bool val) {
  if (!timeset && val) {
    Hour = 0;
    Minute = 0;
  }
}

//Interval을 0시 0분으로 리셋하는 함수(uvb)
void IntervalReset2(bool val) {
  if (!timeset2 && val) {
    Hour2 = 0;
    Minute2 = 0;
  }
}

/*widget toggle switch*/
IOTItemBin ModeStatus;
IOTItemBin Mode(mode_out);

IOTItemBin StopStatus;
IOTItemBin Stop(timeset_out);

IOTItemBin StopStatus2;
IOTItemBin Stop2(timeset_out2);

IOTItemBin FanStatus;
IOTItemBin Fan(fan_out);

IOTItemBin PumpStatus;
IOTItemBin Pump(pump_out);

IOTItemBin LampStatus;
IOTItemBin Lamp(lamp_out);

IOTItemBin UvbStatus;
IOTItemBin Uvb(uvb_out);

IOTItemBin PtcStatus;
IOTItemBin Ptc(ptc_out);

/*widget push button*/
IOTItemBin IntervalHUP(Interval_Hup);
IOTItemBin IntervalMUP(Interval_Mup);
IOTItemBin IntervalRST(IntervalReset);
IOTItemBin IntervalHUP2(Interval_Hup2);
IOTItemBin IntervalMUP2(Interval_Mup2);
IOTItemBin IntervalRST2(IntervalReset2);

/*widget label*/
IOTItemInt label_Hinterval;
IOTItemInt label_Minterval;
IOTItemInt label_Hinterval2;
IOTItemInt label_Minterval2;
IOTItemFlo dht22_temp;
IOTItemFlo dht22_humi;
IOTItemInt soilhumi;

IOTItem *items[ITEM_COUNT] = { &ModeStatus, &Mode,
                               &StopStatus, &Stop,
                               &FanStatus, &Fan,
                               &PumpStatus, &Pump,
                               &LampStatus, &Lamp,
                               //index num : 0 ~ 9

                               &IntervalHUP, &IntervalMUP, &IntervalRST,
                               &label_Hinterval, &label_Minterval,
                               //index num : 10 ~ 14

                               &UvbStatus, &Uvb,
                               &StopStatus2, &Stop2,                               
                               &IntervalHUP2, &IntervalMUP2, &IntervalRST2,
                               &label_Hinterval2, &label_Minterval2,
                               //index num : 15 ~ 23
                               
                               &dht22_temp, &dht22_humi, &soilhumi,
                               //index num : 24 ~ 26

                               &PtcStatus, &Ptc
                               //index num : 27 ~ 28
                             };

/* IOT server communication manager */
const char device_id[] = "088da73b42968dc18f0f155a26150d1c"; // Change device_id to yours
BrokerComm comm(&Serial, device_id, items, ITEM_COUNT);

void drawLogo()
{
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_ncenB08_te);
    u8g2.drawStr(1, 15, " SMART KIT");

    u8g2.drawStr(15, 36, "Temp.");
    u8g2.setCursor(85, 36); u8g2.print(Temp);
    u8g2.drawStr(114, 36, "\xb0" );
    u8g2.drawStr(119, 36, "C");

    u8g2.drawStr(15, 47, "Humidity");
    u8g2.setCursor(85, 47); u8g2.print(Humi);
    u8g2.drawStr(116, 47, "%");

    u8g2.drawStr(15, 58, "Soil Humi.");
    u8g2.setCursor(85, 58); u8g2.print(Soilhumi);
    u8g2.drawStr(116, 58, "%");

    u8g2.sendBuffer();
}

void setup() {
  Serial.begin(250000);
  comm.SetInterval(200);

  pinMode(LAMP, OUTPUT);
  pinMode(PUMP, OUTPUT);
  pinMode(PTC, OUTPUT);
  pinMode(UVB, OUTPUT);
  pinMode(SOILHUMI, INPUT);

  //초기설정
  digitalWrite(LAMP, LOW);
  digitalWrite(PUMP, LOW);
  digitalWrite(PTC, LOW);
  digitalWrite(UVB, LOW);

  u8g2.begin();
  dht.begin();
  StartTime1 = millis();

  // begin with 60hz pwm frequency
  SoftPWM.begin(490);
}


void loop() {

  /* DHT22 data acquisition */
  if ((millis() - StartTime1) > DataCaptureDelay) { //4초 간격으로 실행
    sensors_event_t event1;
    sensors_event_t event2;

    dht.temperature().getEvent(&event1); //DHT22_Temperature
    Temp = event1.temperature;

    dht.humidity().getEvent(&event2); //DHT22_Humidity
    Humi = event2.relative_humidity;

    Soilhumi = map(analogRead(SOILHUMI), 0, 1023, 100, 0); //soil humiditiy

    drawLogo();
    StartTime1 = millis();
  }

  /*Mode change*/
  if (!autoMode) { //수동모드일 때
    auto_1_execute = false;
    ManualMode();
  }
  if (autoMode) { //자동모드일 때
    manu_1_execute = false;
    AutoMode();
  }


  /* Interval time set lamp */
  if (timeset && autoMode) //Auto모드에서 시간설정 스위치가 ON일 때
  {
    TimeStatus = (millis() - StartTime2) / TimeSum ;
  }
  else
  {
    TimeSum = (Hour * 60 + Minute) * 60 * 1000; //ms단위로 변환
    StartTime2 = millis(); //지속적으로 타임 초기화

    if (millis() > StartTime3 + 500) //위젯 버튼 누르는 시간 딜레이주기
    {
      Hour += Interval_Hour_Up_status;
      if (Hour >= 24) Hour = 0;
      Minute += Interval_Minute_Up_status;
      if (Minute >= 60) Minute = 0;
      StartTime3 = millis();
    }
  }

    /* Interval time set uvb */
  if (timeset2 && autoMode) //Auto모드에서 시간설정 스위치가 ON일 때
  {
    TimeStatus2 = (millis() - StartTime4) / TimeSum2 ;
  }
  else
  {
    TimeSum2 = (Hour2 * 60 + Minute2) * 60 * 1000; //ms단위로 변환
    StartTime4 = millis(); //지속적으로 타임 초기화

    if (millis() > StartTime5 + 500) //위젯 버튼 누르는 시간 딜레이주기
    {
      Hour2 += Interval_Hour_Up_status2;
      if (Hour2 >= 24) Hour2 = 0;
      Minute2 += Interval_Minute_Up_status2;
      if (Minute2 >= 60) Minute2 = 0;
      StartTime5 = millis();
    }
  }

  SoftPWM.set(fanVal);

  PumpStatus.Set(digitalRead(PUMP));
  LampStatus.Set(digitalRead(LAMP));
  UvbStatus.Set(digitalRead(UVB));
  if (fanVal > 60) FanStatus.Set(true);
  else FanStatus.Set(false);

  ModeStatus.Set(autoMode);
  StopStatus.Set(timeset);
  StopStatus2.Set(timeset2);
  label_Hinterval.Set(Hour);
  label_Minterval.Set(Minute);
  label_Hinterval2.Set(Hour2);
  label_Minterval2.Set(Minute2);

  dht22_temp.Set(Temp);
  dht22_humi.Set(Humi);
  soilhumi.Set(Soilhumi);

  comm.Run();
}

void ManualMode(){
  if (fan_out_status == true)
  {
    fanVal = 65;
  }
  else
  {
    fanVal = 0;
  }
  digitalWrite(PUMP, pump_out_status);
  digitalWrite(LAMP, lamp_out_status);
  digitalWrite(UVB, uvb_out_status);
  digitalWrite(PTC, ptc_out_status);
}

void AutoMode() {
  /* a LAMP auto control */
  if (timeset) {
    if (TimeStatus % 2 == 1) {
      digitalWrite(LAMP, LOW);
    }
    else {
      digitalWrite(LAMP, HIGH);
    }
  }
  else if (!timeset) {
    digitalWrite(LAMP, LOW);
  }

  if (timeset2) {
    if (TimeStatus2 % 2 == 1) {
      digitalWrite(UVB, LOW);
    }
    else {
      digitalWrite(UVB, HIGH);
    }
  }
  else if (!timeset2) {
    digitalWrite(UVB, LOW);
  }  

  /* a pump auto control */
  if (Soilhumi <= 30) { //토양 습도값이 30%이하일 때
    digitalWrite(PUMP, HIGH);
  }
  else if (Soilhumi >= 60) { //토양 습도값이 60%이상일 때
    digitalWrite(PUMP, LOW);
  }

  /* a FAN PTC auto control */
  if (Temp >= 27) { // 화분 주변 온도가 27*C이상일 때
    fanVal = 65;
    digitalWrite(PTC, LOW);
  }
  else if (Temp <= 23) { // 화분 주변 온도가 23*C이하일 때
    fanVal = 0;
    digitalWrite(PTC, HIGH);
  }
}
