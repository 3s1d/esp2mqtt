#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <cstring>
#include <math.h>
#include <ArduinoOTA.h>

#include "mqtt.h"
#include "led.h"

inline int min(int a,int b) { return ((a)>(b)?(b):(a)); }

Mqtt mqtt = Mqtt();

/*
 * MQTT:
 * sub:
 * /name/set,/name/init -> chx=0..255[,chy=0..255,...,t=[ms]]
 * /name/set/0 -> 0..255
 * /name/set/1 -> 0..255
 * /name/set/2 -> 0..255
 * /name/set/3 -> 0..255
 * /name/set/4 -> 0..255
 * pub
 * /name/status/0 -> 0..255
 * /name/status/1 -> 0..255
 * /name/status/2 -> 0..255
 * /name/status/3 -> 0..255
 * /name/status/4 -> 0..255
 * /name/status ????
 */

void ps_callback(char* topic, byte* payload, unsigned int length) 
{
  if(mqtt.name == NULL || strlen(mqtt.name) == 0)
  {
    Serial.println("mqtt name not set");
    return;
  }
  
  Serial.print("Rx Msg [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) 
    Serial.print((char)payload[i]);
  Serial.println();
 
  /* set */
  char msg[200];
  if(snprintf(msg,  sizeof(msg), "/%s/set", mqtt.name) && strncmp(topic, msg, strlen(msg)) == 0)
  {
    int val[5] = {-1, -1, -1, -1, -1};
    int t = 600;
    if(strlen(topic) == strlen(msg)+2)
    {
      int ch = atoi(&topic[strlen(topic)-1]);
      ch = constrain(ch, 0, 4);
      snprintf(msg, min((int)sizeof(msg), (int)length+1), "%s", payload);
      val[ch] = atoi(msg);
    }
    else
    {
      snprintf(msg, min((int)sizeof(msg), (int)length+1), "%s", payload);
      mqtt.decode_complex_payload(msg, val, &t);
    }

    mqtt.setReceived();    //prevent slider to jump around
    led.set(val, t);
  }
  else if(snprintf(msg,  sizeof(msg), "/%s/init", mqtt.name) && strncmp(topic, msg, strlen(msg)) == 0)
  {
    int val[5] = {0, 0, 0, 0, 0};
    int t = 500;
    snprintf(msg, min((int)sizeof(msg), (int)length+1), "%s", payload);
    mqtt.decode_complex_payload(msg, val, &t);

    /* store to spiffs */
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    
    json["ch0"] = val[0];
    json["ch1"] = val[1];
    json["ch2"] = val[2];
    json["ch3"] = val[3];
    json["ch4"] = val[4];
    json["t"] = t;

    File initFile = SPIFFS.open("/init.json", "w");
    if (!initFile) 
    {
      Serial.println("failed to open init file for writing");
    }
    else
    {
      json.printTo(Serial);
      json.printTo(initFile);
      initFile.close();
      Serial.println(" saved to /init.json");
    }
  }
  else
  {
    Serial.print("Unknown topic: ");
    Serial.println(topic);
  }
}


void Mqtt::decode_complex_payload(char *payload, int *ch, int *t)
{
  if(payload == NULL || ch == NULL || t == NULL)
    return;

    char *token = std::strtok(payload, ",");
    while (token != NULL) 
    {
        if(strlen(token)>=5 && token[0] == 'c' && token[1] == 'h' && token[3] == '=')
        {
          int c = atoi(&token[2]);
          int value = atoi(&token[4]);

          if(c >= 0 && c <= 4)
            ch[c] = constrain(value, -1, 255);
        }
        else if(strlen(token)>=3 && token[0] == 't' && token[1] == '=')
        {
          *t = atoi(&token[2]);
        }
        token = std::strtok(NULL, ",");
    }
}

void Mqtt::setup(Client& client, char *server, int port, char *_name)
{
  if(server == NULL || strlen(server) == 0 || port == 0 || _name == NULL)
    return;

  name = _name;
  ps_client.setClient(client);
  ps_client.setServer(server, port);
  ps_client.setCallback(ps_callback);
}

bool Mqtt::reconnect() 
{
  if(name == NULL || strlen(name) == 0)
  {
    Serial.println("mqtt name not set");
    return false;
  }
  
  // Loop until we're reconnected
  int32_t t = 0;
  while (!ps_client.connected()) 
  {
    ArduinoOTA.handle();
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (ps_client.connect(name)) 
    {
      Serial.println("MQTT connected");
      // Once connected, publish an announcement...
      
      /* subscribe to all */
      char msg[200];
      snprintf(msg,  sizeof(msg), "/%s/set/#", name);
      Serial.print("Subscribing to: ");
      Serial.println(msg);
      ps_client.subscribe(msg);

      snprintf(msg,  sizeof(msg), "/%s/set", name);
      Serial.print("Subscribing to: ");
      Serial.println(msg);
      ps_client.subscribe(msg);

      snprintf(msg,  sizeof(msg), "/%s/init", name);
      Serial.print("Subscribing to: ");
      Serial.println(msg);
      ps_client.subscribe(msg);

      /* debug */
      snprintf(msg, sizeof(msg), "%s connected from %s (%s)", name, WiFi.localIP().toString().c_str(), WiFi.localIPv6().toString().c_str());
      ps_client.publish("/debug", msg);
    }
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(ps_client.state());
      
      if(t >= reconnect_timeout)
      {
        Serial.println();
        return false;
      }
      
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for(int i=0; i<500; i++)
      {
        ArduinoOTA.handle();
        delay(10);
      }
      t += 5000;
    }
  }

  return true;
}

bool Mqtt::loop()
{
  if (!ps_client.connected() && !reconnect())
    return false;
  ps_client.loop();

  if(millis() > last_pubed + 3000 || millis() < last_pubed)
  {
    last_pubed = millis();
    
    int ch[5];
    led.get(ch);
    for(int i=0; i<5; i++)
    {
      if(ch[i] == led_pubed[i])
        continue;
  
      char topic[32];
      char payload[32];
  
      snprintf(topic, sizeof(topic), "/%s/status/%d", name, i);
      snprintf(payload, sizeof(payload), "%d", ch[i]);
      ps_client.publish(topic, payload);
      led_pubed[i] = ch[i];
    }
  }

  return true;
}
