#include <Ethernet.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include "DHT.h"
#define DHTPIN A3
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321

byte mac_addr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress server_addr(192,168,1,43);  // IP of the MySQL *server* here, cmd에서 ipconfig 확인 후 IPv4 주소 입력
char user[] = "testid1";              // MySQL user login username
char password[] = "test12345";        // MySQL user login password

DHT dht(DHTPIN, DHTTYPE);

uint32_t dht_timer = 0; //ms

float temp; // 온도 측정
float humi; // 습도 측정

EthernetClient client;
MySQL_Connection conn((Client *)&client);

// char INSERT_SQL[] = "INSERT INTO test_db1.test_table1 (message) VALUES ('15', '20')";

void setup() 
{
  Serial.begin(115200);
  dht.begin();
  Ethernet.init(6);
  Ethernet.begin(mac_addr);
  if (conn.connect(server_addr, 3306, user, password)) {
    Serial.println("Connected");
    delay(1000);
  }
  else
    Serial.println("Connection failed.");  
}

void loop()
{
  if (millis() > dht_timer + 4000) { //4초 간격으로 dht 측정 및 화면 출력  
    humi = dht.readHumidity();  // Read temperature as Celsius (the default)
    temp = dht.readTemperature();
/*
    Serial.print("temp : "); 
    Serial.print(temp);
    Serial.print("\t humi : "); 
    Serial.println(humi);
*/
    insertSql();
       
    dht_timer = millis();
  }
}

void insertSql(){
  String INSERT_SQL = "INSERT INTO test_db1.test_table1 VALUES ('" + String(temp) + "', '" + String(humi) + "')";
  //String INSERT_SQL = "INSERT INTO test_db1.test_table1 VALUES('15', '25')";
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  
  cur_mem->execute(INSERT_SQL.c_str());
  delete cur_mem;
}
