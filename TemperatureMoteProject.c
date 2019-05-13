#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "erbium.h"
#include "dev/leds.h"
/* Resource definition */

/*NB Apparently Contiki has no support for float */

// float TEMPERATURE; //causes overflow
static bool COOLER = false;
static bool HEATER = false;
static bool VENTILATION = false;

/* Here we say that a packet arrives, start cooling*/
RESOURCE(cooler, METHOD_POST, "actuators/cooler", "title=\"Blue LED\";rt=\"Control\"");
void
cooler_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{

  short int ok_length = 2;
  short int denied_length = 12;
  char const * const ok = "OK";
  char const * const denied = "NOT POSSIBLE";

  if(HEATER == true){ // not possible to turn on the cooler
    memcpy(buffer, denied, denied_length); //denied message
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    REST.set_header_etag(response, (uint8_t *) &denied_length, 1);
    REST.set_response_payload(response, buffer, denied_length);
    return;
  }

  if(HEATER == false && COOLER == false){
    COOLER = true;
    printf("(DEBUG)COOLER:\n");
    leds_on(LEDS_BLUE);
  } else if (HEATER == false && COOLER == true)
  {
    COOLER = false;
    printf("(DEBUG)COOLER:\n");
    leds_off(LEDS_BLUE);
  }

  memcpy(buffer, ok, ok_length); //ok message
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &ok_length, 1);
  REST.set_response_payload(response, buffer, ok_length);  
}

/* Here we say that a packet arrives, start heating*/
RESOURCE(heater, METHOD_POST, "actuators/heater", "title=\"Red LED\";rt=\"Control\"");
void
heater_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  short int ok_length = 2;
  short int denied_length = 12;
  char const * const ok = "OK";
  char const * const denied = "NOT POSSIBLE";
  
  
  if(COOLER == true){ // not possible to turn on the heater
    memcpy(buffer, denied, denied_length); //denied message
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    REST.set_header_etag(response, (uint8_t *) &denied_length, 1);
    REST.set_response_payload(response, buffer, denied_length);
    return;
  }
  
  if(COOLER == false && HEATER == false){
    HEATER = true;
    printf("(DEBUG)HEATER:\n");
    leds_on(LEDS_RED);
  } else if (COOLER == false && HEATER == true)
  {
    HEATER = false;
    printf("(DEBUG)HEATER:\n");
    leds_off(LEDS_RED);
  }

  memcpy(buffer, ok, ok_length); //ok message
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &ok_length, 1);
  REST.set_response_payload(response, buffer, ok_length);
}

/* Here we say that a packet arrives, start ventilating*/
RESOURCE(ventilation, METHOD_POST, "actuators/ventilation", "title=\"Green LED\";rt=\"Control\"");
void
ventilation_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  VENTILATION = !VENTILATION;
  printf("(DEBUG)VENTILATION:\n");
  leds_toggle(LEDS_GREEN);

  /*SETTING RESPONSE*/
  short int length = 2;
  char const * const message = "OK";
  memcpy(buffer, message, length);
  
  
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
  /*END SETTING RESPONSE*/
}

/*PROCESS DEFINITION START*/

PROCESS(server, "CoAP Server");
AUTOSTART_PROCESSES(&server);

PROCESS_THREAD(server, ev, data){
  static short int temperature = 0; //temperature variable
  static struct etimer timer;
  short int current_ratio = 0;
  const short int cooler_ratio = -1;
  const short int heater_ratio = 1;

  PROCESS_BEGIN();
  rest_init_engine();

  temperature  = ((rand() + 10) % 20);
  rest_activate_resource(&resource_cooler);
  rest_activate_resource(&resource_heater);
  rest_activate_resource(&resource_ventilation);

  // we set the timer from here every time
  etimer_set(&timer, CLOCK_CONF_SECOND*5);

  while(1) {

    PROCESS_WAIT_EVENT();

    // // and wait until the vent we receive is the one we're waiting for
    // PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

    if(ev == PROCESS_EVENT_TIMER){
      //update ratio
      if(COOLER == true){
        current_ratio = cooler_ratio;
      } else if (HEATER == true){
        current_ratio = heater_ratio;
      }
      if (VENTILATION == true){
        current_ratio = current_ratio * 2;
      }

      temperature += current_ratio;

      if(temperature >= 30) //bounding temperature values
        temperature = 30;
      if (temperature <= 10)
        temperature = 10;
      

      printf("DEBUG: TEMPERATURE: \n");
      printf("temperatura: %d \n",temperature);

      etimer_reset(&timer);
    }//end of if ev == PROCESS_EVENT_TIMER 

//     //TODO: INFORM SUBSCRIBERS OF TEMPERATURE CHANGE
  }//end of while(1)

  PROCESS_END();
}
