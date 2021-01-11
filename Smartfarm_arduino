#include <DS1307RTC.h> // RTC-LINK 사용
#include <Wire.h> // 온습도 센서 사용
#include "DFRobot_SHT20.h" // 온습도 센서 사용
#include <VitconBrokerComm.h> // Vitcon IoT 서비스 사용
// #include <SoftPWM.h>
#include <U8g2lib.h> //U8g2 by oliver 라이브러리 설치 필요
#include <Arduino.h>
#include <Print.h>

 U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


using namespace vitcon;

DFRobot_SHT20 sht20; // 온습도 센서 사용

#define PUMP 16 //PUMP와 연결된 릴레이 핀으로 지정
#define LAMP 17 // LAMP와 연결된 릴레이 핀으로 지정

// SOFTPWM_DEFINE_CHANNEL(A3); //Arduino pin A3

uint32_t StartTime = 0; // 타이머 시작 시간
tmElements_t tm; // 시간 가져오기(rtc 예제에서 set예제 설정해줘야함)

uint32_t Time_interval; // ms단위 변환한 간격
uint32_t Time_length; // ms단위 변환한 시간
uint32_t drawlogo_timer = 0; // ms
uint32_t iot_timer = 0;

int nowHour; // 현재 시

float humd; // 습도
float temp; // 온도

bool lampres1 = false;
bool lampres2 = false;

// 각종 변수 선언

bool autoMode = false; // default 수동모드
bool pump_out_status;
bool lamp_out_status;
bool activelamp1_out_status;
int32_t lampstart1_out_status;
int32_t lampend1_out_status;
bool activelamp2_out_status;
int32_t lampstart2_out_status;
int32_t lampend2_out_status;
bool activepump_out_status;

int32_t pumpinterval_out_status;
int32_t pumplength_out_status;

//모드 변경을 위한 함수
void mode_out(bool val) {
  autoMode = val;
}

//manual mode일 때 PUMP를 제어하는 함수
void pump_out(bool val) {
  pump_out_status = val;
}

//manual mode일 때 LAMP를 제어하는 함수
void lamp_out(bool val) {
  lamp_out_status = val;
}

void activelamp1_out(bool val) {
  activelamp1_out_status = val;
}

void lampstart1_out(int32_t val) {
  lampstart1_out_status = val;
}

void lampend1_out(int32_t val) {
  lampend1_out_status = val;
}

void activelamp2_out(bool val) {
  activelamp2_out_status = val;
}

void lampstart2_out(int32_t val) {
  lampstart2_out_status = val;
}

void lampend2_out(int32_t val) {
  lampend2_out_status = val;
}

void activepump_out(bool val) {
  activepump_out_status = val;
}

void pumpinterval_out(int32_t val) {
  pumpinterval_out_status = val;
}

void pumplength_out(int32_t val) {
  pumplength_out_status = val;
}

/* A set of definition for IOT items */
#define ITEM_COUNT 26

/* 위젯 선언 */
/*widget toggle switch*/
IOTItemBin ModeStatus;
IOTItemBin Mode(mode_out);

IOTItemBin PumpStatus;
IOTItemBin Pump(pump_out);

IOTItemBin LampStatus;
IOTItemBin Lamp(lamp_out);

IOTItemFlo Temperature;
IOTItemFlo Humidity;

IOTItemBin Activelamp1_status;
IOTItemBin Activelamp1(activelamp1_out);

IOTItemInt Lampstart1_status;
IOTItemInt Lampstart1(lampstart1_out);

IOTItemInt Lampend1_status;
IOTItemInt Lampend1(lampend1_out);

IOTItemBin Activelamp2_status;
IOTItemBin Activelamp2(activelamp2_out);

IOTItemInt Lampstart2_status;
IOTItemInt Lampstart2(lampstart2_out);

IOTItemInt Lampend2_status;
IOTItemInt Lampend2(lampend2_out);

IOTItemBin Activepump_status;
IOTItemBin Activepump(activepump_out);

IOTItemInt Pumpinterval_status;
IOTItemInt Pumpinterval(pumpinterval_out);

IOTItemInt Pumplength_status;
IOTItemInt Pumplength(pumplength_out);

IOTItem *items[ITEM_COUNT] = { &ModeStatus, &Mode,
                               &LampStatus, &Lamp,
                               &PumpStatus, &Pump,
                               &Temperature, &Humidity,
                               &Activelamp1_status, &Activelamp1,
                               &Lampstart1_status, &Lampstart1,
                               &Lampend1_status, &Lampend1,
                               &Activelamp2_status, &Activelamp2,
                               &Lampstart2_status, &Lampstart2,
                               &Lampend2_status, &Lampend2,
                               //index num : 0 ~ 19
                               &Activepump_status, &Activepump,
                               &Pumpinterval_status, &Pumpinterval,
                               &Pumplength_status, &Pumplength
                               //index num : 20 ~ 25
                             };

/* IOT server communication manager */
const char device_id[] = "685ed28f6bd667c79bdf4e63c04dbdee"; //현재 장비의 ID 찾아 따옴표 사이에 넣기(설정 변경에서 확인할 수 있음)
BrokerComm comm(&Serial, device_id, items, ITEM_COUNT);


void drawLogo()
{  
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_ncenB08_te);
    u8g2.drawStr(1, 15, " SMART FARM");

    u8g2.drawStr(15, 36, "Temp.");
    u8g2.setCursor(85, 36); u8g2.print(temp,1);
    u8g2.drawStr(114, 36, "\xb0" );
    u8g2.drawStr(119, 36, "C");

    u8g2.drawStr(15, 47, "Humidity");
    u8g2.setCursor(85, 47); u8g2.print(humd,1);
    u8g2.drawStr(116, 47, "%");

    u8g2.sendBuffer();      
}

void ManualMode(){ // 수동모드
  digitalWrite(PUMP, pump_out_status);
  digitalWrite(LAMP, lamp_out_status);
}

void AutoMode(){ // 자동모드
  if(activepump_out_status == true){ //펌프제어 활성화일때
    // 펌프 제어
    Time_interval = (uint32_t)(pumpinterval_out_status * 60) * 60 * 1000; //Interval시간단위, ms단위로 변환
    Time_length = (uint32_t)pumplength_out_status * 60 * 1000; //length시간단위, ms단위로 변환

    if((millis() - StartTime) < Time_interval){ // 타이머 < 사용자가 지정한 시간 일때
      digitalWrite(PUMP,LOW);
    }

    if((millis() - StartTime) >= Time_interval){ // 타이머 >= 사용자가 지정한 시간 일때
      digitalWrite(PUMP,HIGH); // 펌프 ON
      if( ((millis()-StartTime) - Time_interval) >= Time_length){ // 설정한 기동시간 지나면    
        digitalWrite(PUMP,LOW); // 펌프 OFF
        StartTime = millis(); // 타이머 초기화
      }
    }
  }

  if(activelamp1_out_status == true){  // 구간1 활성화 on
    if(lampstart1_out_status <= lampend1_out_status){ // 시작시간 =< 종료시간 일때
      if(lampstart1_out_status <= nowHour && nowHour < lampend1_out_status){ // 사이에 현재시간
        lampres1 = true; // LAMP ON
      }
      else if(lampend1_out_status <= nowHour){ // 설정종료시간 이상일때
        lampres1 = false;
      }   
      else
        lampres1 = false;          
    }
    else if(lampstart1_out_status > lampend1_out_status){ // 시작시간 > 종료시간 일때(자정넘길때)
      if( lampstart1_out_status <= nowHour && nowHour < 24){ // 자정 전
        lampres1 = true;
      }
      else if( 0 <= nowHour && nowHour < lampend1_out_status){ // 자정 후
        lampres1 = true;        
      }
      else
        lampres1 = false;       
    }
    else{
        lampres1 = false; 
    }
  }
  if(activelamp1_out_status == false){ // 구간1 활성화 off
    lampres1 = false;
  }

  if(activelamp2_out_status == true){ // 구간2 
    if(lampstart2_out_status <= lampend2_out_status){ // 시작시간 =< 종료시간 일때
      if(lampstart2_out_status <= nowHour && nowHour < lampend2_out_status){ // 사이에 현재시간
        lampres2 = true;
      }
      else if(lampend2_out_status <= nowHour){ // 설정종료시간 이상일때
        lampres2 = false;
      }
      else
        lampres2 = false;  
    }
    else if(lampstart2_out_status > lampend2_out_status){ // 시작시간 > 종료시간 일때(자정넘길때)
      if( lampstart2_out_status <= nowHour && nowHour < 24){ // 자정 전
        lampres2 = true;
      }
      else if( 0 <= nowHour && nowHour < lampend2_out_status){ // 자정 후
        lampres2 = true;        
      }
      else
        lampres2 = false;       
    }
    else
      lampres2 = false; 
  }
  if(activelamp2_out_status == false){ // 구간2 활성화 off
    lampres2 = false;
  }
  
  if(lampres1 == true || lampres2 == true)
    digitalWrite(LAMP,HIGH); //하나라도 참이면 참
  else
    digitalWrite(LAMP,LOW);
}

void setup() {
  Serial.begin(250000);
  comm.SetInterval(200);
  sht20.initSHT20();                                  // Init SHT20 Sensor
  sht20.checkSHT20();                                 // Check SHT20 Sensor

  pinMode(PUMP, OUTPUT);
  pinMode(LAMP, OUTPUT);
  u8g2.begin();
  autoMode = false; // 처음 수동모드
}

void loop() { 
  // 온습도 체크
  humd = sht20.readHumidity();                  // Read Humidity
  humd = float(int(humd * 10.0f)) / 10.0f;
  temp = sht20.readTemperature();               // Read Temperature
  temp = float(int(temp * 10.0f)) / 10.0f;
 
  if( millis() > drawlogo_timer + 4000){ // 4초마다 OLED 출력 , 현재 시간 구하기
    drawLogo();
    // 24시간 중 LAMP 제어(RTC-LINK 필요)
  if (RTC.read(tm)) {
    nowHour = tm.Hour; // 현재 시간 얻기
    Serial.println(nowHour);
  }
    drawlogo_timer = millis();    
  }

    /*Mode change*/
  if (!autoMode) { //수동모드일 때
    ManualMode();
  }
  if (autoMode) { //자동모드일 때
    AutoMode();
  }

  if(activepump_out_status == false || !autoMode){ // 펌프제어 비활성화이거나 수동모드일때
    StartTime = millis(); // 비활성화 시간만큼 스타트시간을 더해준다
  }
  
  // 서버에 데이터 전송
  LampStatus.Set(digitalRead(LAMP));
  PumpStatus.Set(digitalRead(PUMP));
  ModeStatus.Set(autoMode);
  Temperature.Set(temp);
  Humidity.Set(humd);

  if( millis() > iot_timer + 3000){
    Activelamp1_status.Set(activelamp1_out_status);
    Lampstart1_status.Set(lampstart1_out_status);
    Lampend1_status.Set(lampend1_out_status);
    Activelamp2_status.Set(activelamp2_out_status);
    Lampstart2_status.Set(lampstart2_out_status);
    Lampend2_status.Set(lampend2_out_status);
    Activepump_status.Set(activepump_out_status);
    Pumpinterval_status.Set(pumpinterval_out_status);
    Pumplength_status.Set(pumplength_out_status);
    iot_timer = millis();
  }
  comm.Run();
}
