#include <ModbusMaster.h>
#include <SPI.h>
#include <Ethernet.h>
#include <VDOS.h>

#define SENSORNUM 9
#define PCPORT Serial
#define DEVICEPORT Serial1

byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};

uint8_t bitsignal, product;
uint16_t data[1], worddata[4];
bool T0 = false, T1 = true;
bool Auto, Stop, Alarm, Completion, Countreset, constate;
uint32_t Poutput;
int MN1, MN2, MN3, MN4;
int OP1, OP2, OP3, OP4, OP5;
int OOP1, OOP2, OOP3;

unsigned long delaytime, delay1, delay2;
uint8_t DataByteArray[200];
char id[13];

bOnSensor autoSensor(false);
bOnSensor stopSensor(false);
bOnSensor alarmSensor_on(true); //실제 알람으로 사용
bOnSensor alarmSensor_off(true); //실제 알람으로 사용
bOnSensor completionSensor(true); //혹시 몰라서 알람으로 설정
bOnSensor countresetSensor(true); //혹시 몰라서 알람으로 설정
lNumberSensor modelNumSensor;
lCountSensor outputNumSensor;
lCountSensor outputSensor;

SensorClass* Sensors[SENSORNUM] =
{
  &alarmSensor_on, //PLC 에러
  &alarmSensor_off, //PLC 에러 정상화
  &autoSensor, //자동 운전 상태
  &stopSensor, //정지 상태
  &completionSensor, //생산 완료 신호
  &countresetSensor, //생산 수량 리셋 신호

  &modelNumSensor, //생산 모델 번호
  &outputNumSensor, //생산 수량
  &outputSensor //1회당 생산 수량
};


DeviceClass device(&DEVICEPORT, Sensors, SENSORNUM);

IPAddress server(192, 168, 0, 9);      // MES 서버 IP
IPAddress ip(192, 168, 0, 203);        // MODLINK IP
IPAddress gateway(192, 168, 0, 1);     // MODLINK 게이트웨이
IPAddress subnet(255, 255, 255, 0);    // MODLINK SUBNET
IPAddress dnsserver(168, 126 , 63, 1);

EthernetClient MES;

ModbusMaster node;

void setup()
{
  Serial2.begin(9600);
  node.begin(1, Serial2);

  PCPORT.begin(115200);
  PCPORT.setTimeout(100);
  DEBUGPORT.begin(115200);
  DEBUGPORT.setTimeout(100);
  DEBUGPORT.println("Debug mode");
  DEVICEPORT.begin(115200);

  Ethernet.init(4);
  Ethernet.begin(mac, ip, dnsserver, gateway, subnet);

  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.dnsServerIP());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.subnetMask());

  device.Init();
  device.SetInterval(1000);
  device.SendSensorList();

  device.SetGatewayReportInterval(60);

  memset(id, 0, sizeof(id));
  device.GetDeviceID(id);
  DEBUGPORT.println(id);

  alarmSensor_on.SetData(0);
  alarmSensor_off.SetData(0);
}


void loop()
{
  PCParsing();
  Dataread();

  if (!constate) MES.connect(server, 7950);
  if (MES.connected()) constate = true;
  if (!MES.connected()) constate = false;

  if (millis() - delaytime > 1000) {
    delaytime = millis();

    Serial.print("Auto=");
    Serial.print(Auto);
    Serial.print("   ");
    Serial.print("Stop=");
    Serial.print(Stop);
    Serial.print("   ");
    Serial.print("Alarm=");
    Serial.print(Alarm);
    Serial.print("   ");
    Serial.print("Completion=");
    Serial.print(Completion);
    Serial.print("   ");
    Serial.print("Count reset=");
    Serial.print(Countreset);
    Serial.print("   ");
    Serial.print("Model Num=");
    Serial.print(worddata[0]);
    Serial.print("   ");
    Serial.print("Production Output=");
    Serial.print(Poutput);
    Serial.print("   ");
    Serial.print("1Cycle Output=");
    Serial.print(worddata[3]);
    Serial.println("   ");

    MESsend();
  }

  //alarmSensor.SetData(Alarm);
  if (Alarm == 1) {
    alarmSensor_on.SetData(true);
    alarmSensor_off.SetData(false);
  }
  else if (Alarm == 0) {
    alarmSensor_off.SetData(true);
    alarmSensor_on.SetData(false);
  }

  completionSensor.SetData(Completion);
  countresetSensor.SetData(Countreset);

  device.Run();
}

void Bitread() {
  bitsignal = node.readDiscreteInputs(0x0000, 5);
  if (bitsignal == node.ku8MBSuccess) data[0] = node.getResponseBuffer(0);
}

void Wordread() {
  product = node.readInputRegisters(0x0000, 4);
  if (product == node.ku8MBSuccess) {
    for (int z = 0; z < 4; z++) worddata[z] = node.getResponseBuffer(z);
  }
}

void Dataread() {

  if (!T0 && millis() - delay1 > 200) {
    delay2 = millis();

    T0 = true;
    T1 = false;
    Bitread();

    Auto = bitRead(data[0], 0);
    Stop = bitRead(data[0], 1);
    Alarm = bitRead(data[0], 2);
    Completion = bitRead(data[0], 3);
    Countreset = bitRead(data[0], 4);

    autoSensor.SetData(Auto);
    stopSensor.SetData(Stop);

    completionSensor.SetData(Completion);
    countresetSensor.SetData(Countreset);
  }

  if (!T1 && millis() - delay2 > 200) {
    delay1 = millis();

    T0 = false;
    T1 = true;
    Wordread();

    Poutput = (int32_t)worddata[1] + ((int32_t)worddata[2] << 16);

    MN1 = worddata[0] % 10;
    MN2 = (worddata[0] / 10) % 10;
    MN3 = (worddata[0] / 100) % 10;
    MN4 = (worddata[0] / 1000) % 10;

    OP1 = Poutput % 10;
    OP2 = (Poutput / 10) % 10;
    OP3 = (Poutput / 100) % 10;
    OP4 = (Poutput / 1000) % 10;
    OP5 = (Poutput / 10000) % 10;

    OOP1 = worddata[3] % 10;
    OOP2 = (worddata[3] / 10) % 10;
    OOP3 = (worddata[3] / 100) % 10;

    modelNumSensor.SetData(worddata[0]);
    outputNumSensor.SetData(Poutput);
    outputSensor.SetData(worddata[3]);
  }
}


void MESsend()
{
  DataByteArray[0] = 'A';
  DataByteArray[1] = '0';
  DataByteArray[2] = '0';
  DataByteArray[3] = '8';
  DataByteArray[4] = Auto + '0';
  DataByteArray[5] = Stop + '0';
  DataByteArray[6] = Completion + '0';
  DataByteArray[7] = Countreset + '0';
  DataByteArray[8] = MN4 + '0';
  DataByteArray[9] = MN3 + '0';
  DataByteArray[10] = MN2 + '0';
  DataByteArray[11] = MN1 + '0';
  DataByteArray[12] = OP5 + '0';
  DataByteArray[13] = OP4 + '0';
  DataByteArray[14] = OP3 + '0';
  DataByteArray[15] = OP2 + '0';
  DataByteArray[16] = OP1 + '0';
  DataByteArray[17] = OOP3 + '0';
  DataByteArray[18] = OOP2 + '0';
  DataByteArray[19] = OOP1 + '0';
  MES.write(DataByteArray, 20);
}

//PC포트 요청데이터 파싱
void PCParsing()
{
  //시리얼로 특정 문자열을 입력받으면 특정 기능을 수행한다.
  //고정IP여부
  //IP
  //Subnet
  //Gateway
  //DNS1
  //DNS2

  if ( PCPORT.available() )
  {
    //String str = PCPORT.readStringUntil('\r');
    String str = PCPORT.readString();
    //PCPORT.println(str);

    if ( str.length() != 0 )
    {
      if ( str.indexOf("+STATIC?") == 0  )
      {
        PCPORT.print("STATIC=");
        //PCPORT.print("\r");
        //        PCPORT.print( (StaticIP.Enable ? "1" : "0"));
        PCPORT.print("\r");
      }
      else if ( str.indexOf("+STATIC=") == 0)
      {
        str.replace("+STATIC=", "");
        int enable = str.toInt();
        if ( enable < 0 || enable > 1 ) PCPORT.println("ERROR");
        else
        {
          PCPORT.print("OK");
          PCPORT.print("\r");
          //          StaticIP.Enable = enable;
        }
      }
      else if ( str.indexOf("+IP?") == 0 )
      {
        PCPORT.print("IP=");
        //PCPORT.print("\r");
        //        PCPORT.print(StaticIP.IP);
        PCPORT.print("\r");
      }
      else if ( str.indexOf("+IP=") == 0 )
      {
        PCPORT.print("OK");
        PCPORT.print("\r");
        str.replace("+IP=", "");
        str.replace("\r", "");
        //        strcpy(StaticIP.IP, str.c_str());
      }
      else if ( str.indexOf("+SUBNET?") == 0 )
      {
        PCPORT.print("SUBNET=");
        //PCPORT.print("\r");
        //        PCPORT.print(StaticIP.Subnet);
        PCPORT.print("\r");
      }
      else if ( str.indexOf("+SUBNET=") == 0)
      {
        PCPORT.print("OK");
        PCPORT.print("\r");
        str.replace("+SUBNET=", "");
        str.replace("\r", "");
        //        strcpy(StaticIP.Subnet, str.c_str());
      }
      else if ( str.indexOf("+GATEWAY?") == 0)
      {
        PCPORT.print("GATEWAY=");
        //PCPORT.print("\r");
        //        PCPORT.print(StaticIP.Gateway);
        PCPORT.print("\r");
      }
      else if ( str.indexOf("+GATEWAY=") == 0 )
      {
        PCPORT.print("OK");
        PCPORT.print("\r");
        str.replace("+GATEWAY=", "");
        str.replace("\r", "");
        //        strcpy(StaticIP.Gateway, str.c_str());
      }
      else if ( str.indexOf("+DNS1?") == 0 )
      {
        PCPORT.print("DNS1=");
        //PCPORT.print("\r");
        //        PCPORT.print(StaticIP.DNS1);
        PCPORT.print("\r");
      }
      else if ( str.indexOf("+DNS1=") == 0 )
      {
        PCPORT.print("OK");
        PCPORT.print("\r");
        str.replace("+DNS1=", "");
        str.replace("\r", "");
        //        strcpy(StaticIP.DNS1, str.c_str());
      }
      else if ( str.indexOf("+DNS2?") == 0 )
      {
        PCPORT.print("DNS2=");
        //PCPORT.print("\r");
        //        PCPORT.print(StaticIP.DNS2);
        PCPORT.print("\r");
      }
      else if ( str.indexOf("+DNS2=") == 0 )
      {
        PCPORT.print("OK");
        PCPORT.print("\r");
        str.replace("+DNS2=", "");
        str.replace("\r", "");
        //        strcpy(StaticIP.DNS2, str.c_str());
      }
      else if ( str.equals("+SAVE") )
      {
        PCPORT.print("OK");
        PCPORT.print("\r");
        //        IO.SaveConfig();
        //        SendEthSetting();
      }
      else if (str.indexOf("+ADDR") >= 0)
      {
        int i = str.substring(5, 6).toInt();
        uint32_t d = str.substring(6).toInt();
        Serial.print("index:");
        Serial.println(i);
        Serial.print("data:");
        Serial.println(d);

        switch (i)
        {
          case 1:
            Auto = d;
            break;
          case 2:
            Stop = d;
            break;
          case 3:
            Alarm = d;
            break;
          case 4:
            Completion = d;
            break;
          case 5:
            Countreset = d;
            break;
          case 6:
            worddata[0] = d;
            break;
          case 7:
            Poutput = d;
            break;
          case 8:
            worddata[3] = d;
            break;
        }

        autoSensor.SetData(Auto);
        stopSensor.SetData(Stop);
        
        //alarmSensor.SetData(Alarm);
        if (Alarm == 1) {
          alarmSensor_on.SetData(true);
          alarmSensor_off.SetData(false);
        }
        else if (Alarm == 0) {
          alarmSensor_off.SetData(true);
          alarmSensor_on.SetData(false);
        }


        modelNumSensor.SetData(worddata[0]);
        outputNumSensor.SetData(Poutput);
        outputSensor.SetData(worddata[3]);
      }
      else
      {
        PCPORT.print("ERROR");
        PCPORT.print("\r");
      }
    }
  }
}
