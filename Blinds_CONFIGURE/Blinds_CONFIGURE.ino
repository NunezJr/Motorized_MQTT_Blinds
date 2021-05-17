#include <SimpleTimer.h>    //https://github.com/marcelloromani/Arduino-SimpleTimer/tree/master/SimpleTimer
#include <ESP8266WiFi.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <ESP8266mDNS.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <PubSubClient.h>   //https://github.com/knolleary/pubsubclient
#include <ArduinoOTA.h>     //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
#include <AH_EasyDriver.h>  //http://www.alhin.de/arduino/downloads/AH_EasyDriver_20120512.zip
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


/*****************  START USER CONFIG SECTION *********************************/
/*****************  START USER CONFIG SECTION *********************************/
/*****************  START USER CONFIG SECTION *********************************/
/*****************  START USER CONFIG SECTION *********************************/

#define USER_SSID                 "ATT6Jch3yN"
#define USER_PASSWORD             "5=7x99cmh5mr"
#define USER_MQTT_SERVER          "192.168.1.92"
#define USER_MQTT_PORT            1883
#define USER_MQTT_USERNAME        "mqttuser"
#define USER_MQTT_PASSWORD        "Betapass1!"
#define USER_MQTT_CLIENT_NAME     "BlindsMCU"         // Used to define MQTT topics, MQTT Client ID, and ArduinoOTA

#define STEPPER_SPEED             35                  //Defines the speed in RPM for your stepper motor
#define STEPPER_STEPS_PER_REV     1028                //Defines the number of pulses that is required for the stepper to rotate 360 degrees
#define STEPPER_MICROSTEPPING     0                   //Defines microstepping 0 = no microstepping, 1 = 1/2 stepping, 2 = 1/4 stepping 
#define DRIVER_INVERTED_SLEEP     1                   //Defines sleep while pin high.  If your motor will not rotate freely when on boot, comment this line out.

#define STEPS_TO_CLOSE            10                  //Defines the number of steps needed to open or close fully

#define STEPPER_DIR_PIN           D6
#define STEPPER_STEP_PIN          D7
#define STEPPER_SLEEP_PIN         D5
#define STEPPER_MICROSTEP_1_PIN   14
#define STEPPER_MICROSTEP_2_PIN   12
 
/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/

WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
AH_EasyDriver shadeStepper(STEPPER_STEPS_PER_REV, STEPPER_DIR_PIN ,STEPPER_STEP_PIN,STEPPER_MICROSTEP_1_PIN,STEPPER_MICROSTEP_2_PIN,STEPPER_SLEEP_PIN);

//Global Variables
bool boot = true;
int currentPosition = 0;
int newPosition = 0;
char positionPublish[50];
bool moving = false;
char charPayload[50];

const char* ssid = USER_SSID ; 
const char* password = USER_PASSWORD ;
const char* mqtt_server = USER_MQTT_SERVER ;
const int mqtt_port = USER_MQTT_PORT ;
const char *mqtt_user = USER_MQTT_USERNAME ;
const char *mqtt_pass = USER_MQTT_PASSWORD ;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME ; 




//Functions
void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(WiFi.status());
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.print("Connected to: ");  
  display.print(WiFi.localIP());
  display.display(); 
  delay(100);
}

void reconnect() 
{
  int retries = 0;
  while (!client.connected()) {
    if(retries < 150)
    {
      // Display static text
  display.print("Attempting MQTT connection...");
  display.display(); 
  delay(100);
      Serial.print("Attempting MQTT connection...");
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) 
      {
        display.println("connected");
        display.display(); 
        delay(100);
        Serial.println("connected");
        if(boot == false)
        {
          client.publish(USER_MQTT_CLIENT_NAME"/checkIn","Reconnected"); 
        }
        if(boot == true)
        {
          client.publish(USER_MQTT_CLIENT_NAME"/checkIn","Rebooted");
        }
        // ... and resubscribe
        client.subscribe(USER_MQTT_CLIENT_NAME"/blindsCommand");
        client.subscribe(USER_MQTT_CLIENT_NAME"/positionCommand");
      } 
      else 
      {
        display.println("failed, rc=");
        display.println(client.state());
        display.println(" try again in 5 seconds");
        display.display(); 
        delay(100);
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if(retries > 149)
    {
    ESP.restart();
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  String newTopic = topic;
  display.print("Message arrived [");
  display.println(topic);
  display.println("]");
  display.display(); 
  delay(100);
  
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  int intPayload = newPayload.toInt();
  Serial.println(newPayload);
  Serial.println();
  newPayload.toCharArray(charPayload, newPayload.length() + 1);
  if (newTopic == USER_MQTT_CLIENT_NAME"/blindsCommand") 
  {
    if (newPayload == "OPEN")
    {
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand", "0", true);
    }
    else if (newPayload == "CLOSE")
    {   
      int stepsToClose = STEPS_TO_CLOSE;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      String temp_str = String(currentPosition);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand", positionPublish, true); 
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/positionCommand")
  {
    if(boot == true)
    {
      newPosition = intPayload;
      currentPosition = intPayload;
      boot = false;
    }
    if(boot == false)
    {
      newPosition = intPayload;
    }
  }
  
}

void processStepper()
{
  if (newPosition > currentPosition)
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepON();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepOFF();
    #endif
    shadeStepper.move(80, FORWARD);
    currentPosition++;
    moving = true;
  }
  if (newPosition < currentPosition)
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepON();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepOFF();
    #endif
    shadeStepper.move(80, BACKWARD);
    currentPosition--;
    moving = true;
  }
  if (newPosition == currentPosition && moving == true)
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepOFF();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepON();
    #endif
    String temp_str = String(currentPosition);
    temp_str.toCharArray(positionPublish, temp_str.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/positionState", positionPublish); 
    moving = false;
  }
  Serial.println(currentPosition);
  Serial.println(newPosition);
}

void checkIn()
{
  client.publish(USER_MQTT_CLIENT_NAME"/checkIn","OK"); 
}


//Run once setup
void setup() {
  Serial.begin(9600);


  //setup display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  


  
  shadeStepper.setMicrostepping(STEPPER_MICROSTEPPING);            // 0 -> Full Step                                
  shadeStepper.setSpeedRPM(STEPPER_SPEED);     // set speed in RPM, rotations per minute
  #if DRIVER_INVERTED_SLEEP == 1
  shadeStepper.sleepOFF();
  #endif
  #if DRIVER_INVERTED_SLEEP == 0
  shadeStepper.sleepON();
  #endif
  WiFi.mode(WIFI_STA); 
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  ArduinoOTA.setHostname(USER_MQTT_CLIENT_NAME);
  ArduinoOTA.begin(); 
  delay(10);
  timer.setInterval(((1 << STEPPER_MICROSTEPPING)*5800)/STEPPER_SPEED, processStepper);   
  timer.setInterval(90000, checkIn);
}

void loop() 
{
  if (!client.connected()) 
  {
    //Serial.print("Not connected");
    reconnect();
  }
  //Serial.print("still connected");
  client.loop();
  ArduinoOTA.handle();
  timer.run();
}