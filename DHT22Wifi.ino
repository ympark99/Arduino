#include <VDOS.h>
#include "DHT.h"

#define DHTPIN A1 // DHT LINK 연결하는 포트
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321

const int TEMP_OVER = 30; // 온도 초과 알람 기준 값 정의
const int HUMI_OVER = 70; // 습도 초과 알람 기준 값 정의

DHT dht(DHTPIN, DHTTYPE);

uint32_t dht_timer = 0; //ms

float temp; // 온도 측정
float humi; // 습도 측정

#define DEVICEPORT Serial1

//사용할 센서 개수를 정의한다.
#define SENSORNUM 4

//사용할 센서 리스트를 정의한다.
TemperatureSensor t1;
HumiditySensor h1;
//bOnoffSensor b1(false);
//bOnoffSensor b2(false);
bOnSensor b1(false);
bOnSensor b2(false);

SensorClass* Sensors[SENSORNUM] = {&t1, &h1, &b1, &b2};

//디바이스 구성
//게이트웨이와의 통신 포트를 설정하고 센서 목록을 넘긴다.
DeviceClass device(&DEVICEPORT, Sensors, SENSORNUM);

bool temp_alarm = false;
bool humi_alarm = false;

void setup() 
{
  DEBUGPORT.begin(115200); //디버그를 위한 포트 오픈
  DEVICEPORT.begin(115200); //통신 포트 오픈. baudrate는 게이트웨이와 맞춘다.(115200 권장)

  dht.begin();
  
  //필요한 경우 센서들 초기값을 넣는다.
  t1.SetData(999);
  h1.SetData(998);

  //디바이스를 초기화 한다.
  device.Init();

  //시리즈 센서 데이터 전송 간격을 설정한다.(ms)
  //최소 간격은 100ms이다.
  device.SetInterval(1000);
  
  //정의한 센서 리스트를 게이트웨이에 알려준다.
  device.SendSensorList();

/*
  //VDOSConnector가 사용할 업체코드명을 전송한다.
  device.SetCompanyCode("vitcon");  

  //이더넷 설정
  device.WifiStop();
  device.EthRefresh();
*/

  //WiFi 설정  
  //연결할 무선 AP의 SSID를 설정한다.
  device.SetWifiSSID("VITCON_3_2.4G", false);

  //연결할 무선 AP의 암호를 설정한다.
  device.SetWifiPassword("vitcon4590", false);

  //WiFi 연결
  device.WifiStart(false);
}
  
void loop()
{
  //필요한 로직 실행
  Process();

  //루프에서 항상 실행되어야 하는 함수.
  //최대한 빠르게 실행되도록 구성해야 한다.
  device.Run();
}

void Process()
{ 
  if ((millis()) > dht_timer + 2000) { //2초 간격으로 dht 측정 및 화면 출력
    humi = dht.readHumidity();  // Read temperature as Celsius (the default)
    temp = dht.readTemperature();
 
    t1.SetData(temp);
    h1.SetData(humi);

    if(temp > TEMP_OVER) temp_alarm = true;
    else temp_alarm = false;

    if(humi > HUMI_OVER) humi_alarm = true;
    else humi_alarm = false;

    b1.SetData(temp_alarm);
    b2.SetData(humi_alarm);
   
    dht_timer = millis();
  }
}
