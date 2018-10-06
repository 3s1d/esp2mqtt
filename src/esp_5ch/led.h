#include <Arduino.h>

/*
 * Channels RGBWY-> 0..4: 12, 14, 27, 17, 16
 */

class Led
{
  private:
 //   hw_timer_t *timer = NULL;
    int gamma[256];

    int target[5];
    int current[5];
    int step[5];
    int sub_step[5];  //circumventing float...

    
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; 
    
  public:
    void loop(void);
    
    Led(void){};

    void init(void);
    void set(int *channel, int time_ms);
    void get(int *channel);
  
};

extern Led led;
