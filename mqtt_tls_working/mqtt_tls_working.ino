#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>

// Insert your FQDN of your MQTT Broker
#define MQTT_SERVER "37.187.106.16"
const char* mqtt_server = MQTT_SERVER;

// WiFi Credentials
const char* ssid = "INBLRFTWIFI";
const char* password = "Flamenco#abcd)";

// Static IP configuration
#define IPSET_STATIC { 192, 168, 1, 138 }
#define IPSET_GATEWAY { 192, 168, 1, 1 }
#define IPSET_SUBNET { 255, 255, 255, 0 }
//#define IPSET_DNS { 10, 10, 10, 254 }

byte ip_static[] = IPSET_STATIC;
byte ip_gateway[] = IPSET_GATEWAY;
byte ip_subnet[] = IPSET_SUBNET;
//byte ip_dns[] = IPSET_DNS;


// converting PER to binar i.e .crt to .der
// openssl x509 -in cert.crt -outform der -out cert.der
// Fingerprint of the broker CA
// openssl x509 -in  mqttserver.crt -sha1 -noout -fingerprint
const char* fingerprint = "A2:79:92:D3:42:0C:89:F2:93:D3:51:37:8B:A5:F5:67:5F:74:FE:3C";

// Topic
char* topic = "test_vacus";

String clientName;

long lastReconnectAttempt = 0;
long lastMsg = 0;
int test_para = 2000;
unsigned long startMills;

WiFiClientSecure wifiClient;
PubSubClient client(mqtt_server, 8883, wifiClient);


void verifytls() {
  // Use WiFiClientSecure class to create TLS connection
  Serial.print("connecting to ");
  Serial.println(mqtt_server);
  if (!wifiClient.connect(mqtt_server, 8883)) {
    Serial.println("connection failed");
    return;
  }

  if (wifiClient.verify(fingerprint, mqtt_server)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
}

// Load Certificates
void loadcerts() {

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  // Load client certificate file from SPIFFS
  /*File cert = SPIFFS.open("/esp.der", "r"); //replace esp.der with your uploaded file name
  if (!cert) {
    Serial.println("Failed to open cert file");
  }
  else
    Serial.println("Success to open cert file");

  delay(1000);

  // Set client certificate
  if (wifiClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");

  // Load client private key file from SPIFFS
  File private_key = SPIFFS.open("/espkey.der", "r"); //replace espkey.der with your uploaded file name
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  }
  else
    Serial.println("Success to open private cert file");

  delay(1000);

  // Set client private key
  if (wifiClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");
  */

  // Load CA file from SPIFFS
  File ca = SPIFFS.open("/test.der", "r"); //replace ca.der with your uploaded file name
  if (!ca) {
    Serial.println("Failed to open ca ");
  }
  else
    Serial.println("Success to open ca");
  delay(1000);

  // Set server CA file
  if (wifiClient.loadCACert(ca))
    Serial.println("ca loaded");
  else
    Serial.println("ca failed");

}

void getTime() {
  // Synchronize time useing SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "de.pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 1000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

boolean reconnect()
{
  if (!client.connected()) {
    if (client.connect((char*) clientName.c_str())) {
      Serial.println("===> mqtt connected");
    } else {
      Serial.print("---> mqtt failed, rc=");
      Serial.println(client.state());
    }
  }
  return client.connected();
}

void wifi_connect()
{
  if (WiFi.status() != WL_CONNECTED) {
    // WIFI
    Serial.println();
    Serial.print("===> WIFI ---> Connecting to ");
    Serial.println(ssid);

    delay(10);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.config(IPAddress(ip_static), IPAddress(ip_gateway), IPAddress(ip_subnet));

    int Attempt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(". ");
      Serial.print(Attempt);

      delay(100);
      Attempt++;
      if (Attempt == 150)
      {
        Serial.println();
        Serial.println("-----> Could not connect to WIFI");

        ESP.restart();
        delay(200);
      }

    }
    Serial.println();
    Serial.print("===> WiFi connected");
    Serial.print(" ------> IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void setup()
{
  Serial.begin(115200);
  startMills = millis();

  wifi_connect();
  delay(500);
  getTime();
  delay(500);

  loadcerts();
  delay(200);


  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);

}

void loop()
{
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      long now = millis();
      if (now - lastReconnectAttempt > 2000) {
        lastReconnectAttempt = now;
        if (reconnect()) {
          lastReconnectAttempt = 0;
        }
      }
    } else {
      long now = millis();
      if (now - lastMsg > test_para) {
        lastMsg = now;
        String payload = "{\"startMills\":";
        payload += (millis() - startMills);
        payload += "}";
        sendmqttMsg(topic, payload);
      }
      client.loop();
    }
  } else {
    wifi_connect();
  }

}

void sendmqttMsg(char* topictosend, String payload)
{

  if (client.connected()) {
    Serial.print("Sending payload: ");
    Serial.print(payload);

    unsigned int msg_length = payload.length();

    Serial.print(" length: ");
    Serial.println(msg_length);

    byte* p = (byte*)malloc(msg_length);
    memcpy(p, (char*) payload.c_str(), msg_length);

    if ( client.publish(topictosend, p, msg_length)) {
      Serial.println("Publish ok");
      free(p);
      //return 1;
    } else {
      Serial.println("Publish failed");
      free(p);
      //return 0;
    }
  }
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
