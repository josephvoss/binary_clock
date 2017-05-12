#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char WiFiSSID[] = "2WIRE3442";
const char WiFiPSK[] = "XXXX";

const int LED_PIN = 2;
const int BATT_PIN = A0;

const int hour_pins[4] = {D1, D2, D3, D4};
const int clock_pins[2] = {D5, D7};
const int shift_pins[2] = {D6, D8};

const char serverHost[] = "192.168.1.111";

unsigned long int seconds = 0;

WiFiClient wifi_client;
PubSubClient client(serverHost, 1883, wifi_client);

void callback(char* topic, byte* payload, unsigned int length)
{
    Serial.println("Message Received");
    //Serial.print(topic);
    //Serial.print(payload);

    char* ptr;
    //Convert input to unsigned long int
    seconds = strtoul((char*)payload, &ptr, 10);
    Serial.printf("Seconds received: %d\n\r",seconds);
}

void initHardware()
{
    Serial.begin(9600);
    client.setCallback(callback);
    for (int i=0; i<4; i++)
        pinMode(hour_pins[i], OUTPUT);
    for (int i=0; i<2; i++)
    {
        pinMode(clock_pins[i], OUTPUT);
        pinMode(shift_pins[i], OUTPUT);
    }
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
        wifi_counter += 1;
    }
    Serial.println("Wifi connected successfully");
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
            client.subscribe("binary_clock/time");
            Serial.println("Connected to server");
            digitalWrite(LED_PIN, HIGH);

            // Send request for the current time
            client.publish("binary_clock/request", "1");
            return 1;
        }
        counter += 1;
        delay(5000);
    } 
    // If we fail to connect, return -1.
    Serial.println("Failed to connect to server");

    return -1;
}

void to_bin(int x, int* arr, int length)
/*
 * Convert a decimal number x into an array of length composed of 1s and 0s.
 * Big-endian
 */
{
    //Reset current array to 0
    for (int i = 0; i < length; i++)
        arr[i] = 0;

    //Basic visual bit-shift
    int counter = 0;
    while (x != 0)
    {
        //Take mod 2 of the value and store in array - reverse order
        //(big endian)
        arr[length-counter-1] = x % 2;
        
        //Subtract by value just stored
        x = x/2;

        //Increment index
        counter++;
    }
}

void update_arr(int** tot_arr, int now)
/*
 * Update the binary array with the time value in seconds "now"
 */
{
    int x;
    //Iterate over the 3 places - hour, minute, seconds
    for (int i = 0; i < 3; i++)
    {
        //Break up into 10 digit and 1 digit for each time place
        for (int j = 1; j > -1; j--)
        {
            //Divide now by the place value (no. of 60 seconds) and the digit
            //value (either 0 or 10)
            x =  now / pow(60,2-i) / pow(10,j);

            //Subtract the value abt to be converted from the seconds left in
            //the total second count
            now = now - x*pow(60,2-i)* pow(10,j);

            //Convert the x value into seconds, and store it in the array for
            //the current time place and digit
            to_bin(x, tot_arr[i*2+j], 4);
        }
    }
}

void set_lights(int** tot_arr)
/*
 * Input the array with time data, and use that to drive all the LEDS
 */
{
    for (int i = 0; i<3; i++)
    {
        for (int j = 0; j<4; j++)
            Serial.printf("%d ", tot_arr[i*2+1][j]);
        Serial.printf("\n\r");
        for (int j = 0; j<3; j++)
            Serial.printf("%d ", tot_arr[i*2][j]);
        Serial.printf("\n\r");
    }
    Serial.println();

    //Light setup
    //Hours 10
    // 2 - register 1 pin 0
    // 1 - register 0 pin 0
    //Hours 1
    // 8 - D4 
    // 4 - D3
    // 2 - D2
    // 1 - D1
    //Minutes 10
    // 4 - register 1 pin 7
    // 2 - register 1 pin 6
    // 1 - register 1 pin 5
    //Minutes 1
    // 8 - register 1 pin 4
    // 4 - register 1 pin 3
    // 2 - register 1 pin 2
    // 1 - register 1 pin 1 
    //Seconds 10
    // 4 - register 0 pin 7
    // 2 - register 0 pin 6
    // 1 - register 0 pin 5
    //Seconds 1
    // 8 - register 0 pin 4
    // 4 - register 0 pin 3
    // 2 - register 0 pin 2
    // 1 - register 0 pin 1 

    int shift_array[2] = {0,0};
    int other_pins[4] = {0,0,0,0};

    //Build shift register values
    for (int i=0; i<3; i++)
    {
        //If not in hours place
        if (i != 2)
        {
            //Tens
            for (int j = 0; j<4; j++)
            {
                shift_array[i] += tot_arr[i*2+1][j];
                shift_array[i] <<= 1;
            }
            //Minutes
            for (int j = 0; j<3; j++)
            {
                shift_array[i] += tot_arr[i*2][j];
                shift_array[i] <<= 1;
            }
            Serial.printf("%d\n\r", shift_array[i]);
        }
        //If hours place
        else
        {
            //Tens
            shift_array[0] += tot_arr[2*2+1][0];
            shift_array[1] += tot_arr[2*2+1][1];
            //Minutes
            for (int j=0; j<4; j++)
                other_pins[j] = tot_arr[2*2][j];
        }
    }

    shiftOut(shift_pins[0], clock_pins[0], MSBFIRST, shift_array[0]);
    shiftOut(shift_pins[1], clock_pins[1], MSBFIRST, shift_array[1]);
    for (int i=0; i<3; i++)
        digitalWrite(hour_pins[i], other_pins[i]);
}

int** light_array;

void setup() 
{
    initHardware();
    Serial.println("Hardware init'ed");

    light_array = (int**) malloc(sizeof(int*)*6);
    for (int i = 0; i < 3; i++)
    {
        light_array[i*2] = (int*) malloc(sizeof(int*)*10);
        light_array[i*2+1] = (int*) malloc(sizeof(int*)*10);
        for (int j = 0; j<4; j++)
        {
            light_array[i*2][j] = 0;
            light_array[i*2+1][j] = 0;
        }
    }

    connectWiFi();
    digitalWrite(LED_PIN, HIGH);

//    DDRD = B11111111;
}

//Initial timer values
int time1 = 0;
int time2 = 0;

bool ledStatus = LOW;

void loop()
{
    //Check time
    time1 = millis();
    update_arr(light_array, seconds);
    seconds++;
    set_lights(light_array);
    
    //Reset time to 0 at midnight
    if (seconds == 86400) seconds = 0;
    
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    //Check if still connected to wifi
    //Not really necessary imo
    if (!client.connected())
    {
        connectToServer();
    }

    //Receive and send messages
    //Not really necessary imo
    client.loop();

    //Check how long it's been, and if it hasn't been a full second, wait
    time2 = millis();
    if (time2 - time1 < 1000) delay(1000 - (time2-time1)); 
}

