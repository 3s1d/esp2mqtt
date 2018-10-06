
#include <Arduino.h>

#include "led.h"

Led led = Led();

void ledTask(void *parameter)
{
  unsigned long t = millis();
  
  while(1)
  {
    if(millis() != t)
    {
      /* execute once every ms */
      t = millis();
      led.loop();

      sched_yield();
    }
    else
    {
      vTaskDelay(1);
    }
  }

  vTaskDelete( NULL );
}

void Led::loop(void)
{
  for(int i=0; i<5; i++)
  {
    if(current[i] != target[i])
    {
      portENTER_CRITICAL_ISR(&mux);
      
      if(abs(current[i] - target[i]) <= abs(step[i]/1000))
      {
        current[i] = target[i];
      }
      else if(abs(step[i]) < 1000)
      {
        sub_step[i] += step[i];
        if(abs(sub_step[i]) >= 1000)
        {
          current[i] += (step[i]>0) ? 1 : -1;
          sub_step[i] -= (step[i]>0) ? 1000 : -1000;
        }
      }
      else
      {
        current[i] += (step[i]/1000);
      }
      
      int tlup = constrain(current[i], 0, 255);
      ledcWrite(i, gamma[tlup]);

      portEXIT_CRITICAL_ISR(&mux);
    }
  }  
}

void Led::init(void)
{
  /* attach pwm to pins */
  for(int i=0; i<5; i++)
  {
    ledcSetup(i, 500, 13);
    ledcWrite(i, 0); 
    target[i] = 0;
    current[i] = 0;
    step[i] = 0;
  }

  ledcAttachPin(GPIO_NUM_12, 0);
  ledcAttachPin(GPIO_NUM_14, 1);
  ledcAttachPin(GPIO_NUM_27, 2);
  ledcAttachPin(GPIO_NUM_17, 3);
  ledcAttachPin(GPIO_NUM_16, 4);

  //ledcAttachPin(GPIO_NUM_13, 9);

  /* prepare software */
  float gammaf   = 2.8;                   // Correction factor
  int   max_in  = 255,                    // Top end of INPUT range
        max_out = 8191;                   // Top end of OUTPUT range
  for(int x=0; x<256; x++)
  {
    gamma[x] = round(pow((float)x / (float)max_in, gammaf) * max_out);
    
    //Serial.print(x);
    //Serial.print('>');
    //Serial.println(gamma[x]);
  }

  /* start timer / task */
  xTaskCreatePinnedToCore(ledTask,      /* Task function. */
              "led",                    /* String with name of task. */
              10000,                    /* Stack size in words. */
              NULL,                     /* Parameter passed as input of the task */
              1,                        /* Priority of the task. */
              NULL,1);                  /* Task handle. */  
}

void Led::set(int *channel, int time_ms)
{
  if(channel == NULL)
    return;
  
  portENTER_CRITICAL(&mux);
  for(int i=0; i<5; i++)
  {
    if(channel[i] >= 0)
    {
      target[i] = constrain(channel[i], 0, 255);

      /* delta */
      if(time_ms == 0)
        step[i] = 1000000;
      else
        step[i] = (target[i] - current[i])*1000 / time_ms;

      //todo
      if(step[i] == 0)
        step[i] = ((target[i] - current[i])>=0) ? 1 : -1;
      sub_step[i] = 0;

//      Serial.print("ch");
//      Serial.print(i);
//      Serial.print("steps: ");
//      Serial.println(step[i]);
    }
  }
  portEXIT_CRITICAL(&mux);
}

void Led::get(int *channel)
{
  if(channel == NULL)
    return;
  for(int i=0; i<5; i++)
    channel[i] = current[i];
}
