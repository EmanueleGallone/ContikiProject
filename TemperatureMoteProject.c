#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "erbium.h"
#include "dev/leds.h"
#include "er-coap-13.h"
/* Resource definition */

/*NB Apparently Contiki has no support for float */

static short int TEMPERATURE = 22;
static bool COOLER = false;
static bool HEATER = false;
static bool VENTILATION = false;
static short int current_ratio = 0;
static short int old_ratio = 0;

/*HERE WE DEFINE THE METHOD TO RETURN THE STATUS OF THE ACTUATORS*/
RESOURCE(status, METHOD_GET, "actuators/status", "title=\"actuators' status\";rt=\"Text\"");
void
status_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  //INCOMPLETE: doesn not write the status (1 or 0) inside the messages
  char *cooling = "C:" + (short int)COOLER;
  char *heating = "H:" + (short int) HEATER;
  char *ventilating = "V:" + (short int) VENTILATION;
  char msg[] = "";

  strcat(msg, cooling);  
  strcat(msg, heating);  
  strcat(msg, ventilating);
  strcat(msg,"\n");  

  memcpy(buffer,msg,strlen(msg));
  short int length = strlen(cooling) + strlen(heating) + strlen(ventilating);

  //printf("(DEBUG) status length: %d\n", strlen(msg));
  //printf("(DEBUG) msg: %s\n", msg);
  
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Here we say that a packet arrives, start cooling*/
RESOURCE(cooler, METHOD_POST, "actuators/cooler", "title=\"Cooling (Blue LED)\";rt=\"Control\"");
void
cooler_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{

  char const * const ok = "OK";
  char const * const denied = "NOT POSSIBLE";
  short int denied_length = strlen(denied);
  short int ok_length = strlen(ok);

  if(HEATER == true){ // not possible to turn on the cooler
    memcpy(buffer, denied, denied_length); //denied message
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    REST.set_header_etag(response, (uint8_t *) &denied_length, 1);
    REST.set_response_payload(response, buffer, denied_length);
    return;
  }

  if(HEATER == false && COOLER == false){
    COOLER = true;
    //printf("(DEBUG)COOLER: %d\n", COOLER);
    leds_on(LEDS_BLUE);
  } else if (HEATER == false && COOLER == true)
  {
    COOLER = false;
    //printf("(DEBUG)COOLER: %d\n", COOLER);
    leds_off(LEDS_BLUE);
  }

  memcpy(buffer, ok, ok_length); //ok message
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &ok_length, 1);
  REST.set_response_payload(response, buffer, ok_length);  
}

/* Here we say that a packet arrives, start heating*/
RESOURCE(heater, METHOD_POST, "actuators/heater", "title=\"Heater (Red LED)\";rt=\"Control\"");
void
heater_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char const * const ok = "OK";
  char const * const denied = "NOT POSSIBLE";
  short int ok_length = strlen(ok);
  short int denied_length = strlen(denied);
  
  if(COOLER == true){ // not possible to turn on the heater
  
    memcpy(buffer, denied, denied_length); //denied message
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    REST.set_header_etag(response, (uint8_t *) &denied_length, 1);
    REST.set_response_payload(response, buffer, denied_length);
  
    return;
  }
  
  if(COOLER == false && HEATER == false){
    HEATER = true;
    //printf("(DEBUG)HEATER: %d\n", HEATER);
    leds_on(LEDS_RED);
  } else if (COOLER == false && HEATER == true)
  {
    HEATER = false;
    //printf("(DEBUG)HEATER: %d\n", HEATER);
    leds_off(LEDS_RED);
  }

  memcpy(buffer, ok, ok_length); //ok message
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &ok_length, 1);
  REST.set_response_payload(response, buffer, ok_length);
}// end of heater_handler

/* Here we say that a packet arrives, start ventilating*/
RESOURCE(ventilation, METHOD_POST, "actuators/ventilation", "title=\"Ventilation (Green LED)\";rt=\"Control\"");
void
ventilation_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  VENTILATION = !VENTILATION;
  //printf("(DEBUG)VENTILATION: %d\n", VENTILATION);
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

PERIODIC_RESOURCE(pushing, METHOD_GET, "temperature", "title=\"Temperature observer\";obs", 5*CLOCK_SECOND);
//INCOMPLETE
void
pushing_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);

  /* Usually, a CoAP server would response with the resource representation matching the periodic_handler. */
  char msg[] = "";
  sprintf(msg, "%d", TEMPERATURE);
  REST.set_response_payload(response, msg, strlen(msg));
}

/* A post_handler that handles subscriptions will be called for periodic resources by the REST framework. */
void
pushing_periodic_handler(resource_t *r)
{
  static uint16_t obs_counter = 0;
  static char content[11];

  ++obs_counter;

  //printf("TICK %u for /%s\n", obs_counter, r->url);

  /* Build notification. */
  coap_packet_t notification[1]; /* This way the packet can be treated as pointer as usual. */
  coap_init_message(notification, COAP_TYPE_NON, REST.status.OK, 0 );
  coap_set_payload(notification, content, snprintf(content, sizeof(content), "TICK %u", obs_counter));

  /* Notify the registered observers with the given message type, observe option, and payload. */
  REST.notify_subscribers(r, obs_counter, notification);
}


/*PROCESS DEFINITION START*/

PROCESS(server, "CoAP Server");
PROCESS(update_temperature, "Update Temperature Service");
AUTOSTART_PROCESSES(&server);

PROCESS_THREAD(update_temperature, ev, data){
  
  static struct etimer timer;
  static short int temp_update;

  //NB: temperature values must be static otherwise its value will not be refreshed
  etimer_set(&timer, CLOCK_CONF_SECOND * 20);

  PROCESS_BEGIN();

   old_ratio = current_ratio;
   temp_update = current_ratio;

   PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_TIMER) {

      if (VENTILATION == true){
        temp_update = current_ratio * 2;
      }
    
      TEMPERATURE += temp_update;
    
  } 
  PROCESS_END();
}//end of PROCESS_THREAD(update_temperature, ev, data)

PROCESS_THREAD(server, ev, data){

  static struct etimer timer;

  static short int cooler_ratio = -1;
  static short int heater_ratio = 1;

  PROCESS_BEGIN();
  rest_init_engine();
  
  rest_activate_resource(&resource_cooler);
  rest_activate_resource(&resource_heater);
  rest_activate_resource(&resource_ventilation);
  rest_activate_resource(&resource_status);

  //this will be used for informing client of current temperature
  rest_activate_periodic_resource(&periodic_resource_pushing);

  // we set the timer
  etimer_set(&timer, CLOCK_CONF_SECOND);

  while(1) {

    PROCESS_WAIT_EVENT();

    if(ev == PROCESS_EVENT_TIMER){
      printf("(DEBUG)temperatura: %d \n",TEMPERATURE);

      

      //update ratio
      if(COOLER == true){
        current_ratio = cooler_ratio;
        process_start(&update_temperature, NULL);
      } else if (HEATER == true){
        current_ratio = heater_ratio;
        process_start(&update_temperature, NULL);
      } else {
        current_ratio = 0; //reset
      }

      if (old_ratio != current_ratio){
        process_exit(&update_temperature);
      }
 

      if(TEMPERATURE >= 30) //bounding temperature values
        TEMPERATURE = 30;
      if (TEMPERATURE <= 10)
        TEMPERATURE = 10;

      etimer_reset(&timer);
    }

  }//end of while(1)

  PROCESS_END();
}