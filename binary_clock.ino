#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char WiFiSSID[] = "2WIRE3442";
const char WiFiPSK[] = "XXXX";

const int LED_PIN = 2;
const int BATT_PIN = A0;

const char serverHost[] = "192.168.1.111";

WiFiClient wifi_client;
PubSubClient client(serverHost, 1883, wifi_client);

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.println("Message Received");
  //Serial.print(topic);
  //Serial.print(payload);

  //Do something

  
}

void initHardware()
{
  Serial.begin(9600);
  client.setCallback(callback);
}

void connectWiFi()
{
  byte ledStatus = LOW;

  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);

  int wifi_counter = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_counter < 12)
  {
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    delay(5000); //wait 5 seconds
    
    Serial.println("Not connected");
    lcd.clear();
    lcd.print("WiFi Not");
    lcd.setCursor(0,1);
    lcd.print("Connected");
    wifi_counter += 1;
  }
  //Serial.println("Wifi connected successfully");
  lcd.clear();
  lcd.print("WiFi Connected");
  digitalWrite(LED_PIN, HIGH);
}

int connectToServer()
{
  // LED turns on when we enter, it'll go off when we 
  // successfully post.
  digitalWrite(LED_PIN, LOW);
  int counter = 0;
  while (counter < 3) 
  {
    if (client.connect("ESP8266 Binary Clock"))
    {
      client.subscribe("XXXXX");
      Serial.println("Connected to server");
      digitalWrite(LED_PIN, HIGH);
      return 1;
    }
    counter += 1;
    delay(5000);
  } 
  // If we fail to connect, return -1.
  Serial.println("Failed to connect to server");
  
  return -1;
}

void setup() 
{
  initHardware();
  Serial.println("Hardware init'ed");
  connectWiFi();
  digitalWrite(LED_PIN, HIGH);
}

void loop() 
{
  if (!client.connected())
  {
    connectToServer();
  }
  client.loop(); 
}

