#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <U8g2lib.h> //U8g2 by oliver 라이브러리 설치 필요
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#include <VitconBrokerComm.h> // iot service

using namespace vitcon;

//dht
#include "DHT.h"
#define DHTPIN A1 // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321

//SD
#define SET_TIMER 60000 // 데이터 출력 간격 지정(1분)
String file_name; // 온도데이터 저장 파일
const uint8_t SD_CARD = 5; // SD카드 위치
File file;  // file

#define GAS_A A3 // GAS 아날로그값
#define BUZZER 14 // BUZZER
#define RELAY1 16
#define RELAY2 17

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);
uint32_t dht_timer = -2000; //ms
float humi; // 습도 측정
float temp; // 온도 측정
float voc;

// timer
uint32_t _timer = -60000; // SD카드 타이머
uint32_t drawlogo_timer = -4000; // OLED 출력 타이머 ms
uint32_t alarm_timer = 0; // 알람 타이머
uint32_t et_timer = 0; // et 타이머
uint32_t ts_timer = 0; // ts 타이머
uint32_t submode_timer = 0; // 숫자 부저 타이머

// hmi
String main_mode="";
String pw_input = "";
String pw = "0000"; // 초기 비밀번호
String sub_mode = "";
String str_setdata = "";
String str_temp = "4"; // 설정온도 문자열(기본 4도)
String str_alarm = "8"; // 알람온도 문자열(기본 8도)
bool set_update = 0; // 설정값 업데이트 카운터
bool data_update = 1; // 온습,voc 업데이트 카운터

//RTC
String digitalmon; // month 앞에 0써주기 위해 선언
String digitalday; // day 앞에 0써주기 위해 선언
String digitalhou; // hour 앞에 0써주기 위해 선언
String digitalmin; // minute 앞에 0써주기 위해 선언
String digitalsec; // second 앞에 0써주기 위해 선언
tmElements_t tm; // RTC 시간 받아오기

// ETC
bool submode_cnt = 0; // 숫자입력 10초 카운터
bool buzoff_sub = 1; // 숫자입력 off 부저
bool buzoff_sensor = 1; // 센서 off 부저
bool buzoff_alarm = 1; // 알람 off 부저
bool buzoff_pw = 1; // 비밀번호 off 부저

//IOT
#define ITEM_COUNT 5 //IoT 위젯 아이템 개수
IOTItemPsh push;
IOTItemFlo iot_temp;
IOTItemFlo iot_humi;
IOTItemFlo iot_voc;
IOTItemStr iot_time;
char today[50];

IOTItem *items[ITEM_COUNT] = {&push, &iot_temp, &iot_humi, &iot_voc, &iot_time};

const char device_id[] = "abcf91137bb77803be1cd459f197f79d"; // 현재 장비의 ID 찾아 따옴표 사이에 넣기(설정 변경에서 확인할 수 있음)
BrokerComm comm(&Serial, device_id, items, ITEM_COUNT);

void drawLogo()
{
  if (RTC.read(tm)) {    
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_ncenB08_te);
    u8g2.drawStr(1, 15, " GMS");
    u8g2.setCursor(70, 15); u8g2.print(String(tm.Month)+String("/")+String(tm.Day));
    u8g2.setCursor(100, 15); u8g2.print(digitalhou+String(":")+digitalmin);

    u8g2.drawStr(18, 28, "Temp");
    u8g2.drawStr(84, 28, "Set");    

   if (isnan(humi) || isnan(temp)) { // 센서값 못읽는 경우
     u8g2.drawStr(15, 39, "-" );
     u8g2.drawStr(15, 62, "-" );     
   }
   else{
    u8g2.setCursor(15, 39); u8g2.print(temp,1);
    u8g2.setCursor(15, 62); u8g2.print(humi,1);
   }
    u8g2.drawStr(40, 39, "\xb0" );
    u8g2.drawStr(45, 39, "C");
    
    u8g2.setCursor(75, 39); u8g2.print(float(str_temp.toInt()),1); 
    u8g2.drawStr(100, 39, "\xb0" );
    u8g2.drawStr(105, 39, "C");   

    u8g2.drawStr(18, 51, "Humi");
    u8g2.drawStr(80, 51, "VOC");    
    
    u8g2.drawStr(42, 62, "%");
    u8g2.setCursor(75, 62); u8g2.print(voc,2);
    u8g2.drawStr(102, 62, "ppm");       
    u8g2.sendBuffer();      
  }    
  
}

void set_mode(){ // 온도 설정
  data_update = 0;
  sub_mode = Serial1.readString();
  Serial.print("sub mode : ");
  Serial.println(sub_mode);  
  if(set_update == 1){ // 페이지 접속했을때 업데이트
    // 설정 온도
    Serial1.print("t9.txt=\"");
    Serial1.print(str_temp);
    Serial1.print("\"");
    Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
    // 설정 온도 슬라이더
    str_setdata = "h0.val=";
    str_setdata.concat(str_temp.toInt());
    sendString(str_setdata);
    Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);    
    // 알람 온도
    Serial1.print("t10.txt=\"");
    Serial1.print(str_alarm);
    Serial1.print("\"");
    Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
    // 알람 온도 슬라이더
    str_setdata = "h1.val=";
    str_setdata.concat(str_alarm.toInt());
    sendString(str_setdata);
    Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);  
    
    set_update = 0;
  }
  if(sub_mode.startsWith("SAVE") == 1){ // 저장(SAVE로 시작하는 경우)
    str_setdata = sub_mode;
    str_setdata = str_setdata.substring(str_setdata.indexOf("SAVE"),str_setdata.length()); // save앞 부분 제거
    str_setdata.remove(0,4);
    str_temp = str_setdata.substring(0,str_setdata.indexOf(",")); // 설정온도 반환(문자열)
    str_alarm = str_setdata.substring(str_setdata.indexOf(",")+1,str_setdata.length()); // 알람온도 반환(문자열)

    Serial.print("tempdata : ");
    Serial.print(str_temp); 
    Serial.print("\t alarmdata : ");
    Serial.println(str_alarm);
    sub_mode = "";
  }
  if(sub_mode.indexOf("BACK") != -1){ // 초기화
    main_mode = "";
    str_setdata = "";
    set_update = 1; // 설정값 업데이트
    data_update = 1;
  }
}

void door_mode(){ // 비밀번호
  data_update = 0;
  sub_mode = Serial1.readString();
  Serial.print("sub mode : ");
  Serial.println(sub_mode);

  if(isDigit(pw_input.charAt(pw_input.length()-1)) == true || submode_cnt == 1){ // 마지막 입력이 숫자일때, cnt=1 일때
    Serial.println("숫자 입력");
    if(sub_mode == "OPEN"){ // open버튼 누른경우
      Serial.println("open 입력");
      buzoff_sub = 1; // 부저 off      
      submode_cnt = 0;
    }  
    else{
      Serial.println("open안누름");
      submode_cnt = 1; // 타이머 계속 진행
      if(millis()- submode_timer > 10000){ // 10초 경과시        
        digitalWrite(BUZZER,HIGH);
        buzoff_sub = 0;
      }
    }
  }
  else submode_timer = millis();
  if(sub_mode.indexOf("BACK") != -1){ // 초기화
    main_mode = "";
    pw_input = "";
    sub_mode = "";
    data_update = 1;
  }
  else if(sub_mode == "OPEN"){ // 비밀번호 확인 및 변경
    if(pw_input == pw){ // 비밀번호 일치시 릴레이 출력
      digitalWrite(RELAY2,HIGH);
      buzoff_pw = 1;
    }
    else if(pw_input != pw){ // 비밀번호 틀릴 경우
      digitalWrite(RELAY2,LOW);
      digitalWrite(BUZZER,HIGH);
      buzoff_pw = 0;
    }
    pw_input = ""; 
    sub_mode = "";       
  }
  else if(pw_input == "6344"){ // 6344 누를경우 비밀번호 변경
    pw = ""; // 비밀번호 초기화
    while(pw.length() < 4){
      pw += Serial1.readString(); // 비밀번호 입력
      Serial.print("pw : ");    
      Serial.println(pw);      
    }
    pw_input = "";
  }
  else{ // 숫자 버튼 누를경우
    pw_input += sub_mode;
    pw_input.replace("OPEN",""); // 열림 버튼 빨리 누른 경우 비밀번호 문자열에 추가되므로 제거
    if(pw_input.length() > 4){
      pw_input.remove(0,pw_input.length()-4); // 길이가 4자리 초과시 마지막 4자리만 남김
    }
    Serial.print("pw input : ");
    Serial.println(pw_input);        
  }
}

void sd_data(float temp, float humi, float voc){ 
 if (!SD.begin(SD_CARD)) {
   Serial.println("SD.begin failed");
   while(1);
 }  
// file_name = String(tmYearToCalendar(tm.Year))+String(tm.Month)+String(tm.Day)+String(".csv"); // 오늘날짜로 파일 생성
 file_name = String(tmYearToCalendar(tm.Year))+digitalmon+digitalday+String(".csv"); // 오늘날짜로 파일 생성
 file = SD.open(file_name, FILE_WRITE);
 if(file){
  Serial.print("Writing to txt file...");
  file.print(digitalmon+String("/")+digitalday+String(","));
  file.print(digitalhou+String(":")+digitalmin+String(","));
  file.print(temp + String(","));
  file.print(humi + String(","));
  file.println(voc);
  file.close();
  Serial.println("done.");  
 } else{
  Serial.println("file open error");
 }
// open file
  file = SD.open(file_name);
  if(file){
    Serial.println("reading file..."); 
    while(file.available()){
      Serial.write(file.read());
    }
    file.close();
    Serial.println("done");
  } else{
    Serial.println("file open error");
  } 
}

void sd_alarm(bool alarm){  // 온도,센서 이상알람 실시간 저장(0->ET, 1->TS)   
 if (!SD.begin(SD_CARD)) {
   Serial.println("SD.begin failed");
   while(1);
 }  
 file = SD.open("ALARM.csv", FILE_WRITE);
 if(file){
  Serial.println("Writing alarm data...");
  file.print(String(tmYearToCalendar(tm.Year))+String("/")+digitalmon+String("/")+digitalday+String(","));
  file.print(digitalhou+String(":")+digitalmin+String(":")+digitalsec+String(","));
  if(alarm == 0) file.println(String("ET"));
  else if(alarm == 1) file.println(String("TS"));
  file.close();
  Serial.println("done.");  
 } else{
  Serial.println("file open error");
 }
}

void sendString(String sendData) {
  for(int i = 0; i < sendData.length(); i++) {
    Serial1.write(sendData[i]);   // 1 문자씩 보냄
  }
  Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);    // 종료 바이트 3개 보냄
}

void setup() {
  Serial.begin(250000);
  u8g2.begin();
  dht.begin();
  Serial1.begin(9600);
    
  comm.SetInterval(200);
  pinMode(GAS_A,INPUT);
  pinMode(BUZZER,OUTPUT);
  pinMode(RELAY1,OUTPUT);
  pinMode(RELAY2,OUTPUT);
  drawLogo();
}

void loop() { 
    // hmi 함수 실행
    if(main_mode.indexOf("SET") != -1){  // SET 버튼 누를경우       
      set_mode();      
      main_mode == "SET";
    }
    else if(main_mode.indexOf("DOOR") != -1){ // DOOR 버튼 누를경우
      door_mode();
      main_mode == "DOOR";      
    }
    else{
      main_mode = Serial1.readString();
    }
    Serial.print("main mode : ");
    Serial.println(main_mode); 
     
  if (RTC.read(tm)) {  // 날짜,시간이 10이하일 경우 앞에 0으로 표시(3 -> 03)
   if(tm.Month >= 0 && tm.Month < 10){ // 앞에 0 써주기
      digitalmon = String(0) + String(tm.Month);
   }
   else{
     digitalmon = String(tm.Month);
   }     
   if(tm.Day >= 0 && tm.Day < 10){ // 앞에 0 써주기
      digitalday = String(0) + String(tm.Day);
   }    
   else{
     digitalday = String(tm.Day);
   }       
   if(tm.Hour >= 0 && tm.Hour < 10){ // 앞에 0 써주기
      digitalhou = String(0) + String(tm.Hour);
   }  
   else{
     digitalhou = String(tm.Hour);
   }    
   if(tm.Minute >= 0 && tm.Minute < 10){ // 앞에 0 써주기
      digitalmin = String(0) + String(tm.Minute);
   }
   else{
     digitalmin = String(tm.Minute);
   }
   if(tm.Second >= 0 && tm.Second < 10){ // 앞에 0 써주기
     digitalsec = String(0) + String(tm.Second);
   }
   else{
     digitalsec = String(tm.Second);
   } 
  }
 
 if ((millis()) > dht_timer + 2000) { //2초 간격으로 가스, dht 측정 및 화면 출력
  String temp_hmi;
  String humi_hmi;
  String voc_hmi;
 
  voc = analogRead(GAS_A) * 3.83 * 0.001; // 가스 값 읽기    
  humi = dht.readHumidity();  // Read temperature as Celsius (the default)
  temp = dht.readTemperature();

  if(data_update == 1){ // 처음화면인 경우 업데이트
    voc_hmi = "x2.val=";
    voc_hmi.concat(int(voc*100));

    temp_hmi = "x0.val=";
    temp_hmi.concat(int(temp*10));

    humi_hmi = "x1.val=";
    humi_hmi.concat(int(humi*10));  

    sendString(voc_hmi);
    sendString(temp_hmi);
    sendString(humi_hmi);
  }
  dht_timer = millis();
 }
   if (isnan(humi) || isnan(temp)) { // 센서값 못읽는 경우
     digitalWrite(BUZZER, HIGH); // 즉시 부저 발생
     buzoff_sensor = 0;
     if(millis() > ts_timer + 1000){ // 1초마다 실행
       Serial.println(("Failed to read from DHT sensor!"));
       sd_alarm(1); // 센서 이상알람 저장(TS = 1)
       ts_timer = millis();
     }
     return;
   }
   else buzoff_sensor = 1;
   
   if(temp - str_temp.toInt() >= 0.2) digitalWrite(RELAY1,HIGH); // 현재온도가 설정온도보다 0.2이상일때
   if(str_temp.toInt() - temp >= 0.2) digitalWrite(RELAY1,LOW); // 설정온도가 현재온도보다 0.2이상일때
   if(temp > str_alarm.toInt()){ // 알람설정 온도 초과시
    if(millis()-alarm_timer > 600000){ // 온도 초과 후 10분 경과시
      digitalWrite(BUZZER,HIGH); // 부저 발생
      buzoff_alarm = 0;
      push.Set("알람설정 온도 초과!!"); //푸시알람
      if(millis() > et_timer + 1000){  // 1초마다 실행
        sd_alarm(0); // 온도 이상알람 저장(ET = 0)
        et_timer = millis();
      }
    }
   }
   else{
    buzoff_alarm = 1;
    alarm_timer = millis();    
   }
    
  if( millis() > drawlogo_timer + 4000){ // 4초마다 OLED, 날짜 hmi 출력
    Serial1.print("t3.txt=\"");
    Serial1.print(String(tmYearToCalendar(tm.Year))+String(".")+String(tm.Month)+String(".")+String(tm.Day)+String(" ")+digitalhou+String(":")+digitalmin);
    Serial1.print("\"");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
    drawLogo();
    drawlogo_timer = millis();
  }


  if( millis() > _timer + SET_TIMER){ // 1분에 한번 SD카드에 데이터 저장
   sd_data(temp,humi,voc); // sd카드에 온도, 습도 , voc 저장
   _timer = millis();  
  }

  if((buzoff_sub && buzoff_sensor && buzoff_alarm && buzoff_pw) == 1){
      digitalWrite(BUZZER,LOW); // 4개 모두 off상태여야 부저 off -> 하나라도 참이면 부저 on   
  }

  // 서버에 데이터 전송
  iot_temp.Set(temp);
  iot_humi.Set(humi);
  iot_voc.Set(float(int(voc * 100.0f)) / 100.0f); // 소수점 둘째자리까지 출력

  today[0] = 0;
  char tmp[10] = {0};
  // 날짜 iot 출력
  digitalmon.toCharArray(tmp,digitalmon.length()+1); strcat(today,tmp);
  strcat(today,"/");
  digitalday.toCharArray(tmp,digitalday.length()+1); strcat(today,tmp);
  strcat(today,"  ");
  digitalhou.toCharArray(tmp,digitalhou.length()+1); strcat(today,tmp); 
  strcat(today,":");
  digitalmin.toCharArray(tmp,digitalmin.length()+1); strcat(today,tmp);
  iot_time.Set(today);
  
  comm.Run();
}
