#include <Arduino.h>
#include <PubSubClient.h>


class Mqtt
{
private:
  PubSubClient ps_client;
  int led_pubed[5] = {-1, -1, -1, -1, -1};
  unsigned long last_pubed = 0;
  int32_t reconnect_timeout = 60000*5;
  
  bool reconnect();

public:
  char *name = NULL;
  
  Mqtt(){};
  void setup(Client& client, char *server, int port, char *_name);
  bool loop();

  void setReceived(void) { last_pubed = millis(); }
  void decode_complex_payload(char *payload, int *ch, int *t);
};


extern Mqtt mqtt;
