#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

#include "esp_sleep.h"

#define BUTTON_PIN_BITMASK  0x0F00000000 // lower 4 bit
#define BUTTON0             34
#define BUTTON1             32
#define BUTTON2             35
#define BUTTON3             33
#define VBAT_SENS           13

#define LED_RED             12
#define LED_GREEN           14
#define LED_BLUE            27

RTC_DATA_ATTR int bootCount = 0;

struct Button {
    const uint8_t PIN;
    uint8_t numberKeyPresses;
};
Button btn[4] = {{BUTTON0}, {BUTTON1}, {BUTTON2}, {BUTTON3}};

char mqtt_server[40] = "orthos.fritz.box";
char mqtt_port[6] = "1883";
char name[40] = "espSwitch";

static volatile bool wifi_connected = false;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback ()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void saveConfig()
{
  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  json["name"] = name;
  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
  }
  else
  {
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    Serial.println(" saved to /config.json");
  }
}

void WiFiEvent(WiFiEvent_t event) 
{
  switch (event)
  {
    case SYSTEM_EVENT_AP_START:
      //can set ap hostname here
      //            WiFi.softAPsetHostname(AP_SSID);
      //enable ap ipv6 here
      //            WiFi.softAPenableIpV6();
      break;

    case SYSTEM_EVENT_STA_START:
      //set sta hostname here
      WiFi.setHostname(name);
      Serial.print("set hostname ");
      Serial.println(name);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      //enable sta ipv6 here
      WiFi.enableIpV6();
      break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      //both interfaces get the same event
      Serial.print("STA IPv6: ");
      Serial.println(WiFi.localIPv6());
      //           Serial.print("AP IPv6: ");
      //           Serial.println(WiFi.softAPIPv6());
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      wifi_connected = true;
      Serial.println("STA Connected");
      Serial.print("STA IPv4: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      wifi_connected = false;
      Serial.println("STA Disconnected");
      break;
    default:
      break;
  }
}

void wifiCfg(void)
{
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_name("Name", "name", name, 40);
  WiFiManagerParameter custom_mqtt_server("Server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("Port", "mqtt port", mqtt_port, 6);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_name);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(240);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.setBreakAfterConfig(true);
  //if(!wifiManager.startConfigPortal("ESP Cfg AP", "espcfg"))
  if(!wifiManager.autoConnect("ESP Cfg AP", "espcfg"))
  {
    Serial.println("Evaluating config");

    strcpy(name, custom_name.getValue());
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    if (shouldSaveConfig)
      saveConfig();

    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected");

  //read updated parameters
  strcpy(name, custom_name.getValue());
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig)
    saveConfig();
}


void wakeup_status(void)
{
  uint64_t wakeup_status = esp_sleep_get_ext1_wakeup_status();
  if(wakeup_status & 0x0400000000)
    btn[0].numberKeyPresses=1;
  if(wakeup_status & 0x0100000000)
    btn[1].numberKeyPresses=1;
  if(wakeup_status & 0x0800000000)
    btn[2].numberKeyPresses=1;
  if(wakeup_status & 0x0200000000)
    btn[3].numberKeyPresses=1;
}

void sleep(void)
{
  pinMode(LED_RED, INPUT);
  pinMode(LED_GREEN, INPUT);
  pinMode(LED_BLUE, INPUT);
  pinMode(VBAT_SENS, INPUT);
  
    /*
  First we configure the wake up source. We set our ESP32 to wake up for an external trigger. There are two types for ESP32, ext0 and ext1.
  ext0 uses RTC_IO to wakeup thus requires RTC peripherals to be on while ext1 uses RTC Controller so doesnt need peripherals to be powered on.
  Note that using internal pullups/pulldowns also requires RTC peripherals to be turned on.
  */
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

  //Go to sleep now
  Serial.println("Sleeping");
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void IRAM_ATTR isr0(void)
{
  btn[0].numberKeyPresses = 2;
  detachInterrupt(btn[0].PIN);
}

void IRAM_ATTR isr1(void)
{
  btn[1].numberKeyPresses = 2;
  detachInterrupt(btn[1].PIN);
}

void IRAM_ATTR isr2(void)
{
  btn[2].numberKeyPresses = 2;
  detachInterrupt(btn[2].PIN);
}

void IRAM_ATTR isr3(void)
{
  btn[3].numberKeyPresses = 2;
  detachInterrupt(btn[3].PIN);
}

void setup(){
  Serial.begin(115200);

  /* buttons */
  pinMode(BUTTON0, INPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("#boot=" + String(bootCount));
//todo input for led!
  if(esp_sleep_get_wakeup_cause() == 2)     //Wakeup caused by external signal using RTC_CNTL
  {
    wakeup_status();

    Serial.print("Btn:");
    Serial.print(btn[0].numberKeyPresses);
    Serial.print(btn[1].numberKeyPresses);
    Serial.print(btn[2].numberKeyPresses);
    Serial.println(btn[3].numberKeyPresses);
  }
  else
  {
    sleep();
  }

  /* led */
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, HIGH);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, HIGH);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_BLUE, HIGH);

  /* ADC sens */
  pinMode(VBAT_SENS, OUTPUT);
  digitalWrite(VBAT_SENS, HIGH);

  //read configuration from FS json
  //Serial.println("mounting FS...");
  bool mounted = SPIFFS.begin();
  if (!mounted)
  {
    /* second try, fresh esp?? */
    SPIFFS.format();
    mounted = SPIFFS.begin();
  }

  if (mounted)
  {
    //clean FS, for testing
    //SPIFFS.format();

    Serial.println("mnt fs");

    if (SPIFFS.exists("/config.json"))
    {
      //file exists, reading and loading
      //Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        //Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          Serial.println("");
          if (json["name"].as<const char*>())
            strcpy(name, json["name"]);
          if (json["mqtt_server"].as<const char*>())
            strcpy(mqtt_server, json["mqtt_server"]);
          if (json["mqtt_port"].as<const char*>())
            strcpy(mqtt_port, json["mqtt_port"]);

          Serial.print("Name:");
          Serial.println(name);
          Serial.print("MQTT:");
          Serial.print(mqtt_server);
          Serial.print(":");
          Serial.println(mqtt_port);
        }
        else
        {
          Serial.println("failed to load json config");
        }
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }

  /* buttons */
//  attachInterruptArg(btn[0].PIN, btnIsr, &btn[0], RISING);      //currently broken in 1.0.0
//  attachInterruptArg(btn[1].PIN, isr, &btn[1], RISING);
//  attachInterruptArg(btn[2].PIN, isr, &btn[2], RISING);
//  attachInterruptArg(btn[3].PIN, isr, &btn[3], RISING);
  if(digitalRead(btn[0].PIN) == HIGH)
    btn[0].numberKeyPresses = 3;
  else
    attachInterrupt(btn[0].PIN, isr0, RISING);
  if(digitalRead(btn[1].PIN) == HIGH)
    btn[1].numberKeyPresses = 3;
  else
    attachInterrupt(btn[1].PIN, isr1, RISING);
  if(digitalRead(btn[2].PIN) == HIGH)
    btn[2].numberKeyPresses = 3;
  else
    attachInterrupt(btn[2].PIN, isr2, RISING);
  if(digitalRead(btn[3].PIN) == HIGH)
    btn[3].numberKeyPresses = 3;
  else
    attachInterrupt(btn[3].PIN, isr3, RISING);

  /* vbat */
  int adc = analogRead(36);    //ADC1_CH0
  float volt = ((120.0+47.0)/47.0)*adc / 1024.0;
  Serial.print("Vbat=");
  Serial.println(volt);
  digitalWrite(VBAT_SENS, LOW);

  if(volt >= 3.2)
  {
    digitalWrite(LED_GREEN, LOW);
  }
  else if(volt >= 3.1)
  {
    /* yellow */
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
  }
  else
  {
    digitalWrite(LED_RED, LOW);
  }

  /* conntect to wifi */
  WiFi.onEvent(WiFiEvent);
  wifiCfg();

  /* led blue */
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, LOW);

  /* OTA */
  if(digitalRead(BUTTON0) && digitalRead(BUTTON3) && !digitalRead(BUTTON1) && !digitalRead(BUTTON2))
  {
    ArduinoOTA
    .onStart([]() 
    {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() { Serial.println("\nEnd"); })
    .onProgress([](unsigned int progress, unsigned int total) { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.setHostname(name);
    ArduinoOTA.begin();       
    
    Serial.println("OTA Ready");
  
    for(int i=0; i<6000; i++)     //10min
    {
      digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
      digitalWrite(LED_RED, !digitalRead(LED_BLUE));
      
      ArduinoOTA.handle();
      delay(100);    
    }

    esp_wifi_stop();
    sleep();
  }

    /* wait for multi press */
  while(digitalRead(btn[0].PIN) == HIGH || digitalRead(btn[1].PIN) == HIGH || digitalRead(btn[2].PIN) == HIGH || digitalRead(btn[3].PIN) == HIGH)
    delay(50);

  /* remove int */
  detachInterrupt(btn[0].PIN);
  detachInterrupt(btn[1].PIN);
  detachInterrupt(btn[2].PIN);
  detachInterrupt(btn[3].PIN);

  /* MQTT */
  WiFiClient espClient;
  PubSubClient ps_client(espClient);
  ps_client.setServer(mqtt_server, atoi(mqtt_port));
  Serial.println("Attempting MQTT connection...");
  if (ps_client.connect(name)) 
  {
    Serial.println("MQTT connected");
    // Once connected, publish an announcement...

    char topic[128];
    char msg[256];

    /* buttons */
    for(int i=0; i<4; i++)
    {
      if(btn[i].numberKeyPresses == 0)
        continue;

      snprintf(topic, sizeof(topic), "/%s/btn%d", name, i);
      snprintf(msg, sizeof(msg), "%d", btn[i].numberKeyPresses);
      ps_client.publish(topic, msg);
    }
    
    /* voltage */
    snprintf(topic, sizeof(topic), "/%s/voltage", name);
    snprintf(msg, sizeof(msg), "%.2f", volt);
    ps_client.publish(topic, msg);

    /* debug */
    //snprintf(msg, sizeof(msg), "%s connected from %s (%s)", name, WiFi.localIP().toString().c_str(), WiFi.localIPv6().toString().c_str());
    //ps_client.publish("/debug", msg);

    ps_client.disconnect();
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_GREEN, LOW);
  }
  else
  {
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_RED, LOW);
    Serial.print("failed, rc=");
    Serial.println(ps_client.state());
  }


  esp_wifi_stop();
  delay(200);   //to show return status
 
  sleep();
}

void loop(){
  //This is not going to be called
}
