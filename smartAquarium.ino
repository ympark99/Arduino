#include <VitconBrokerComm.h> // Vitcon IoT 서비스 사용
#include <DS1307RTC.h> // RTC LINK 사용
#include <Stepper.h> // 스텝모터 사용
#include <VitconNTC.h> // NTC-V2-LINK 사용
#include <U8g2lib.h> //U8g2 by oliver 라이브러리 설치 필요

using namespace vitcon;
VitconNTC ntc;

// 센서 위치 선언
#define TdsSensorPin A3
#define TempSensor A6
#define LED 14 // LED와 연결된 릴레이 핀으로 지정
#define PUMP 15 //PUMP와 연결된 릴레이 핀으로 지정

// 시간, 기준값 정의
#define PUSH_INTERVAL 3600000 // 푸시 알림 간격(ms단위)
#define ONEHOUR 3600000 // 한시간 ms단위
#define ONEMINUTE 60000 // 1분 ms단위
#define OVERTDS 170 // TDS 초과 알림 기준값

tmElements_t tm; // 현재 시각 객체 선언

// TDS 선언
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;

// OLED 선언
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// LED
int nowHour; // 현재 시
bool onLamp = false; // LED on off

// PUMP
uint32_t Time_length; // ms단위 변환한 시간

// 스텝모터
#define STEPS 2048
#define SPEED 3
const int stepsPerRevolution = 256; // 2048:한바퀴(360도), 1024:반바퀴(180도)
Stepper myStepper(STEPS,19,17,18,16); // 모터 드라이브에 연결된 핀 IN4, IN2, IN3, IN1

// timer
uint32_t DataCaptureDelay = 2000; // OLED 및 위젯 업데이트 딜레이
uint32_t DataCapture_ST = -3000; //Start Time
uint32_t StartTime = 0; // 타이머 시작 시간
uint32_t StartTime_feed = 0; // 타이머 시작 시간(급여)
uint32_t Feed_interval; // ms단위 변환한 급여 간격
uint32_t rtc_timer = -4000; // rtc 타이머
uint32_t push_timer = -1 * PUSH_INTERVAL; // 푸시 간격 타이머

// 변수 선언
float temp = 999; // 수온
float averageVoltage = 0,tdsValue = 0,temperature = 25;

// Iot 관련 변수
bool isAutoMode = false; // 수동 / 자동 모드
bool pump_out_status; // 수동모드 펌프 on/off
int32_t pumpinterval_out_status; // 자동모드 펌프 작동 시간

bool led_out_status; // 수동모드 LED on/off
int32_t ledstart_out_status; // 자동모드 LED 시작 시간
int32_t ledend_out_status; // 자동모드 LED 종료 시간

bool step_out_status; // 수동모트 스텝모터 동작
int32_t stepinterval_out_status = 8; // 스텝모터 급여 간격

void mode_out(bool val) {
  isAutoMode = val;
}

void pump_out(bool val) {
  pump_out_status = val;
}

void pumpinterval_out(int32_t val) {
  pumpinterval_out_status = val;
}

void led_out(bool val) {
  led_out_status = val;
}

void ledstart_out(int32_t val) {
  ledstart_out_status = val;
}

void ledend_out(int32_t val) {
  ledend_out_status = val;
}

void step_out(bool val) {
  step_out_status = val;
}

void stepinterval_out(int32_t val) {
  stepinterval_out_status = val;
}
/* A set of definition for IOT items */
#define ITEM_COUNT 19

// 위젯 선언
IOTItemBin ModeStatus;
IOTItemBin Mode(mode_out);

IOTItemBin PumpStatus;
IOTItemBin Pump(pump_out);

IOTItemInt Pumpinterval_status;
IOTItemInt Pumpinterval(pumpinterval_out);

IOTItemBin LEDStatus;
IOTItemBin Led(led_out);

IOTItemInt LEDstart_status;
IOTItemInt LEDstart(ledstart_out);

IOTItemInt LEDend_status;
IOTItemInt LEDend(ledend_out);

IOTItemBin Step(step_out);

IOTItemInt Stepinterval_status;
IOTItemInt Stepinterval(stepinterval_out);

IOTItemFlo Temperature;
IOTItemInt TDS;
IOTItemBin TDS_alarm;
IOTItemPsh push;

IOTItem *items[ITEM_COUNT] = { &ModeStatus, &Mode, // 0 ~ 1
                               &PumpStatus, &Pump, &Pumpinterval_status, &Pumpinterval, // 2 ~ 5
                               &LEDStatus, &Led, &LEDstart_status, &LEDstart, &LEDend_status, &LEDend, // 6 ~ 11
                               &Step, &Stepinterval_status, &Stepinterval, // 12 ~ 14
                               &Temperature, &TDS, &TDS_alarm, &push // 15 ~ 18
                             };
/* IOT server communication manager */
const char device_id[] = "894e923054d0a06f0ae719f1d336eb33"; //현재 장비의 ID 찾아 따옴표 사이에 넣기(설정 변경에서 확인할 수 있음)
BrokerComm comm(&Serial, device_id, items, ITEM_COUNT);


void drawOLED()
{  
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_ncenB08_te);
    u8g2.drawStr(1, 15, " SMART AQUARIUM");

    u8g2.drawStr(11, 36, "Temp.");
    u8g2.setCursor(75, 36); u8g2.print(temp,1);
    u8g2.drawStr(107, 36, "\xb0" );
    u8g2.drawStr(112, 36, "C");

    u8g2.drawStr(11, 49, "TDS.");
    u8g2.setCursor(75, 49); u8g2.print(tdsValue,0);
    u8g2.drawStr(95, 49, "ppm");

    u8g2.drawStr(11, 62, "Mode.");
    isAutoMode? u8g2.drawStr(75, 62, "AUTO") : u8g2.drawStr(75, 62, "MANUAL");
    
    u8g2.sendBuffer();      
}

// TDS 값 측정
int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
        for (i = 0; i < iFilterLen - j - 1; i++) 
            {
          if (bTab[i] > bTab[i + 1]) 
              {
          bTemp = bTab[i];
              bTab[i] = bTab[i + 1];
          bTab[i + 1] = bTemp;
           }
        }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}

void ManualMode(){ // 수동모드
  digitalWrite(LED, led_out_status);
  digitalWrite(PUMP, pump_out_status);
  if(step_out_status){
    myStepper.step(stepsPerRevolution); // 시계 반대 방향으로 회전
    step_out_status = false; // 버튼 누를때 한번만 동작하므로 다시 false 처리
  }
  else{ // 스텝모터 비활성화(발열 억제)
    digitalWrite(16, LOW);
    digitalWrite(17, LOW);
    digitalWrite(18, LOW);
    digitalWrite(19, LOW);    
  }
}

void AutoMode(){ // 자동모드 
    // 펌프 제어
    Time_length = pumpinterval_out_status * ONEMINUTE; // length 분 단위, ms단위로 변환

    if((millis() - StartTime) > ONEHOUR - Time_length){ // 타이머 >= 사용자가 지정한 시간 일때
      digitalWrite(PUMP,HIGH); // 펌프 ON
      if((millis() - StartTime) > ONEHOUR){ // 설정한 기동시간 지나면    
        StartTime = millis(); // 타이머 초기화
      }
    }    
    else if((millis() - StartTime) < ONEHOUR - (int)Time_length){ // 타이머 < 사용자가 지정한 시간 일때
      digitalWrite(PUMP,LOW);
    }

    // LED 제어
    if(ledstart_out_status <= ledend_out_status){ // 시작시간 =< 종료시간 일때
      if(ledstart_out_status <= nowHour && nowHour < ledend_out_status){ // 사이에 현재시간
        onLamp = true; // LED ON
      } 
      else onLamp = false;
    }
    else if(ledstart_out_status > ledend_out_status){ // 시작시간 > 종료시간 일때(자정넘길때)
      if(ledstart_out_status <= nowHour && nowHour < 24) onLamp = true; // 자정 전
      else if( 0 <= nowHour && nowHour < ledend_out_status) onLamp = true; // 자정 후
      else onLamp = false;       
    }
    else onLamp = false;           
    if(onLamp == true) digitalWrite(LED,HIGH);
    else digitalWrite(LED,LOW); 
    
    // 스텝모터 제어
    Feed_interval = stepinterval_out_status * ONEHOUR; // length시간단위, ms단위로 변환
    if((millis() - StartTime_feed) > Feed_interval){ // 설정한 급여 주기 지나면
      myStepper.step(stepsPerRevolution); // 시계 반대 방향으로 회전
      StartTime_feed = millis(); // 타이머 초기화
    }
    else{
      digitalWrite(16, LOW);
      digitalWrite(17, LOW);
      digitalWrite(18, LOW);
      digitalWrite(19, LOW);
    }
}

void setup(void)
{
  Serial.begin(250000);
  u8g2.begin();
  comm.SetInterval(200);
  myStepper.setSpeed(SPEED); // 회전 속도 설정  
  pinMode(LED, OUTPUT); 
  pinMode(PUMP, OUTPUT);  
  pinMode(TdsSensorPin,INPUT);  
  pinMode(TempSensor,INPUT);
  isAutoMode = false; // 처음 수동모드
}


void loop(void){ 
   // 수온 측정
   temp = ntc.GetTemperature(analogRead(TempSensor));
   temp = float(int(temp * 10.0f)) / 10.0f; // 소수점 첫째자리로 변환
   // TDS값 측정 부분
   static unsigned long analogSampleTimepoint = millis();
   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }   
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:"); Serial.print(averageVoltage,2); Serial.print("V   "); Serial.print("TDS Value:"); Serial.print(tdsValue,0); Serial.println("ppm");
   }

  if(millis() > rtc_timer + 4000){ // 4초마다 현재 시간 구하기
    if(RTC.read(tm)){
      nowHour = tm.Hour; // 현재 시간 얻기
    }
    rtc_timer = millis();
  }
    /*Mode change*/
  if (!isAutoMode) { //수동모드일 때
    ManualMode();
    StartTime = millis(); // 수동모드 시간만큼 스타트시간을 더해준다
    StartTime_feed = millis(); // 수동모드 시간만큼 스타트시간을 더해준다
  }
  if (isAutoMode) { //자동모드일 때
    AutoMode();
  }

  // TDS 값 일정 기준 이상 시 경보 작동 및 알람
  if(tdsValue > OVERTDS){
    TDS_alarm.Set(true);
    if(millis() > push_timer + PUSH_INTERVAL){ 
      push.Set("TDS 값이 높습니다!(설정시간 후 알림)");  // 스마트폰 알람
      push_timer = millis();
    }
  }
  else{
    TDS_alarm.Set(false);
    push_timer = -1 * PUSH_INTERVAL;
  }
  
  // 3초마다 온도, TDS 값 OLED 및 서버에 전송
  if ((millis() - DataCapture_ST) > DataCaptureDelay){
    drawOLED(); //OLED에 데이터 출력
        
    Temperature.Set(temp);
    TDS.Set(tdsValue);    
    Pumpinterval_status.Set(pumpinterval_out_status);
    LEDstart_status.Set(ledstart_out_status);
    LEDend_status.Set(ledend_out_status);
    Stepinterval_status.Set(stepinterval_out_status);
        
    DataCapture_ST = millis();
  }
  // 서버에 실시간으로 데이터 전송
  ModeStatus.Set(isAutoMode);
  PumpStatus.Set(digitalRead(PUMP));
  LEDStatus.Set(digitalRead(LED));
  comm.Run();  
}
