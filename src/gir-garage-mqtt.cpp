
#include <AccelStepper.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <ArduinoOTA.h>

#define EN D1


// Update these with values suitable for your network.
const char* ssid = "hermes-outside";//put your wifi ssid here
const char* password = "#teamfuckchad";//put your wifi password here
const char* host = "gir-garage-module";
const char* mqtt_server = "10.0.1.3";
const char* clientId = "girGarageDoor";
const char* userName = "homeassistant";
const char* passWord = "kei0UDah9veideich7eepheep5shah4aexei5paexi7aegh8wot9chohnoasahsh";
const char* willTopic = "home-assistant/girgaragedoor/availability";
const char* willMessage = "offline";
const byte willQoS = 0;
const boolean willRetain = false;

const int stepsToMove = 1900;
// initialize the stepper library on D1,D2,D5,D6
//Stepper myStepper(stepsPerRevolution, D5, D6, D7, D8);

AccelStepper myAStepper(1, D2, D3);
WiFiClient espClient;
PubSubClient client(espClient);

void setupOTA() {
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname(host);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

void setup_wifi() {
   delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void enableMotor() {
    digitalWrite(EN, HIGH);
  }

 void disableMotor() {
    digitalWrite(EN, LOW);
   }

void turnMotorOff() {
  disableMotor();
  myAStepper.disableOutputs();
    Serial.println("Motor off.");
  }

void turnMotorOn() {
  enableMotor();
  myAStepper.enableOutputs();
    Serial.println("Motor on.");
  }

void openDoor() {
    turnMotorOn();
    Serial.println("Opening..." );
    myAStepper.move(-stepsToMove);
    Serial.println("Open.");
    turnMotorOff();
  }

void closeDoor() {
  turnMotorOn();
  Serial.println("Closing...");
  myAStepper.move(stepsToMove);
  Serial.println("Closed.");
  turnMotorOff();
  }

void callback(char* topic, byte* payload, unsigned int length) 
{

  String response;

  for (int i = 0; i < length; i++) {
    response += (char)payload[i];
  }
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  Serial.println((char)payload[0]-'0');
  Serial.print("Response: ");
  Serial.println(response);
  int p =(char)payload[0]-'0';

      Serial.println("Switch Topic");
        if (response == "OPEN") {
          // open
          client.publish("home-assistant/girgaragedoor/state", "opening");
          openDoor();
          client.publish("home-assistant/girgaragedoor/state", "open");
          }
          else if (response == "CLOSE") {
            // close
            client.publish("home-assistant/girgaragedoor/state", "closing");
            closeDoor();
            client.publish("home-assistant/girgaragedoor/state", "closed");
            }
  Serial.println();
   
}
 //end callback

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect(clientId,userName,passWord, willTopic, willQoS, willRetain, willMessage))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
     client.subscribe("home-assistant/girgaragedoor/set");
     client.publish("home-assistant/girgaragedoor/availability", "online");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(6000);
    }
  }
} //end reconnect()

void setup() {
  Serial.begin(9600);
  setupOTA();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //AccelStepper
  myAStepper.setMaxSpeed(6000);
  myAStepper.setAcceleration(2000);

  // Set pin to enable and disable motor controller
  pinMode(EN, OUTPUT);

    myAStepper.run();

}

void loop() {
  myAStepper.run();
  if (!client.connected()) {
    reconnect();
  }
  bool isRunning = myAStepper.run();
  if (!isRunning) {
    client.loop();
    }
  
  ArduinoOTA.handle();

}
