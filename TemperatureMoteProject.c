#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "erbium.h"
/* Resource definition */

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#if defined (PLATFORM_HAS_LEDS)
#include "dev/leds.h"
#endif

#define REST_RES_COOLER 1
static int TEMPERATURE;
static bool COOLER = false;
static bool HEATER = false;
static bool VENTILATION = false;

#if defined (PLATFORM_HAS_LEDS)
#if REST_RES_COOLER
/* Here we say that a packet arrives, start cooling*/
RESOURCE(cooler, METHOD_POST, "actuators/cooler", "title=\"Blue LED\";rt=\"Control\"");
void
cooler_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{

  int ok_length = 2;
  int denied_length = 12;
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
    PRINTF("(DEBUG)COOLER:" + COOLER);
    leds_on(LEDS_BLUE);
  } else if (HEATER == false && COOLER == true)
  {
    COOLER = false;
    PRINTF("(DEBUG)COOLER:" + COOLER);
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
  int ok_length = 2;
  int denied_length = 12;
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
    PRINTF("(DEBUG)HEATER:" + HEATER);
    leds_on(LEDS_RED);
  } else if (COOLER == false && HEATER == true)
  {
    HEATER = false;
    PRINTF("(DEBUG)HEATER:" + HEATER);
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
  PRINTF("(DEBUG)VENTILATION:" + VENTILATION);
  leds_toggle(LEDS_GREEN);

  /*SETTING RESPONSE*/
  int length = 2;
  char const * const message = "OK";
  memcpy(buffer, message, length);
  
  
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
  /*END SETTING RESPONSE*/
}
#endif
#endif /* PLATFORM_HAS_LEDS */

PROCESS(server, "CoAP Server");
AUTOSTART_PROCESSES(&server);



PROCESS_THREAD(server, ev, data){

    PROCESS_BEGIN();
    rest_init_engine();

    
#if REST_RES_COOLER
  TEMPERATURE  = (rand() + 10) % 20;
  rest_activate_resource(&resource_cooler);
  rest_activate_resource(&resource_heater);
  rest_activate_resource(&resource_ventilation);
#endif
    while(1) {

    PROCESS_WAIT_EVENT();

    }
    PROCESS_END();

}
