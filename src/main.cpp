#include <Arduino.h>

#if defined(NODE_ENGINE)
  #include "engine/engine.h"
#elif defined(NODE_SENSOR)
  #include "sensor/sensor.h"
#elif defined(NODE_ACTUATOR)
  #include "actuator/actuator.h"
#elif defined(NODE_DASHBOARD)
  #include "dashboard/dashboard.h"
#endif

void setup() {
#if defined(NODE_ENGINE)
  engine_setup();
#elif defined(NODE_SENSOR)
  sensor_setup();
#elif defined(NODE_ACTUATOR)
  actuator_setup();
#elif defined(NODE_DASHBOARD)
  dashboard_setup();
#endif
}

void loop() {
#if defined(NODE_ENGINE)
  engine_loop();
#elif defined(NODE_SENSOR)
  sensor_loop();
#elif defined(NODE_ACTUATOR)
  actuator_loop();
#elif defined(NODE_DASHBOARD)
  dashboard_loop();
#endif
}