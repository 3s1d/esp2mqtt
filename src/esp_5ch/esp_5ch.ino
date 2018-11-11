#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <SPIFFS.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#include "led.h"
#include "mqtt.h"


//WiFi Manager:
//https://github.com/bbx10/WiFiManager

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "apollo.fritz.box";
char mqtt_port[6] = "1883";
char name[40] = "esp_5ch";

WiFiClient espClient;

//flag for saving data
bool shouldSaveConfig = false;

static volatile bool wifi_connected = false;


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

void wifiCfg(bool autoconnect)
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
  wifiManager.setTimeout(600);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.setBreakAfterConfig(true);
  if ((autoconnect && !wifiManager.autoConnect("ESP Cfg AP", "espcfg")) ||
      (!autoconnect && !wifiManager.startConfigPortal("ESP Cfg AP", "espcfg")))
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

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("boot...");

  //rtc_clk_freq_set(RTC_CPU_FREQ_80M);

  led.init();

  //read configuration from FS json
  Serial.println("mounting FS...");
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

    Serial.println("mounted file system");

    if (SPIFFS.exists("/init.json"))
    {
      Serial.println("reading init file");
      File initFile = SPIFFS.open("/init.json", "r");
      if (initFile)
      {
        Serial.println("opened init file");
        size_t size = initFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        initFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          int val[5] = {0, 0, 0, 0, 0};
          int t = 500;

          Serial.println("\nparsed json");
          if (json["ch0"].as<const char*>())
            val[0] = json["ch0"];
          if (json["ch1"].as<const char*>())
            val[1] = json["ch1"];
          if (json["ch2"].as<const char*>())
            val[2] = json["ch2"];
          if (json["ch3"].as<const char*>())
            val[3] = json["ch3"];
          if (json["ch4"].as<const char*>())
            val[4] = json["ch4"];
          if (json["t"].as<const char*>())
            t = json["t"];

          led.set(val, t);
        }
      }
      //else default values
    }

    if (SPIFFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          Serial.println("\nparsed json");
          if (json["name"].as<const char*>())
            strcpy(name, json["name"]);
          if (json["mqtt_server"].as<const char*>())
            strcpy(mqtt_server, json["mqtt_server"]);
          if (json["mqtt_port"].as<const char*>())
            strcpy(mqtt_port, json["mqtt_port"]);

          Serial.print("Name: ");
          Serial.println(name);
          Serial.print("MQTT: ");
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
  //end read

  /* conntect to wifi */
  WiFi.onEvent(WiFiEvent);
  wifiCfg(true);

  //Erases SSID/password, for testing next time
  //WiFi.disconnect(true);

  /* MQTT */
  for(int i=0; i<50 && wifi_connected==false; i++)
    delay(100);    //let ips settle
  mqtt.setup(espClient, mqtt_server, atoi(mqtt_port), name);

  /* OTA */
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
  
  Serial.println("Ready");
}


void loop()
{
  if(millis() < 1000*60*5)   //5min
    ArduinoOTA.handle();
  
  /* MQTT */
  if (!mqtt.loop())
    wifiCfg(false);

  vTaskDelay(10);
}
