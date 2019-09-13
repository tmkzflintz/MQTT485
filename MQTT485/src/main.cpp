#include <WiFi.h>
#include <PubSubClient.h>
#include <ModbusMaster.h>

#define MAX485_DE         15
#define MAX485_RE_NEG     15
#define LENGTH_ADDR       0x00
#define MILLI_AMP_ADDR    0x01
#define TEMPERATURE_ADDR  0x02

const char* ssid = "TMKZ";
const char* password = "1212312121";
const char* mqtt_server = "203.172.40.152";

ModbusMaster ultraSonic;
WiFiClient espClient;
PubSubClient client(espClient);
HardwareSerial SerialOne(1);

long lastMsg = 0;
char msg[50];
char deviceAddr[100];

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

void setup_RS485(){

  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);

  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  Serial.begin(9600);
  SerialOne.begin(9600, SERIAL_8N1, 4, 2);

  ultraSonic.begin(1, SerialOne);
  ultraSonic.preTransmission(preTransmission);
  ultraSonic.postTransmission(postTransmission);

}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  
  setup_RS485();    
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    uint8_t result = ultraSonic.readHoldingRegisters(0x00, 3);  
    if (result == ultraSonic.ku8MBSuccess)
    {
        float L     = ultraSonic.getResponseBuffer(LENGTH_ADDR)       / 100.0f;
        float mA    = ultraSonic.getResponseBuffer(MILLI_AMP_ADDR)    / 100.0f;
        float TempC = ultraSonic.getResponseBuffer(TEMPERATURE_ADDR)  / 100.0f;
        snprintf (msg, 50, "%s,%.02f,%.02f,%.02f",deviceAddr,L,mA,TempC);        
    }
    else
    {
        snprintf (msg, 50, "%s,none,none,none",deviceAddr);   
    }

    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("ultraSonic", msg);
  }
}