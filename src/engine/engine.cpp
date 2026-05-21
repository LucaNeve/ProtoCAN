#include <stdint.h>
#include <Arduino.h>
#include "frame.h"


#define MAX_RPM 6000
#define MIN_RPM 800
#define MAX_TEMP 110
#define MIN_TEMP 60

static uint16_t rpm_corrente;
static uint8_t  temp_corrente;
static uint32_t ultimo_invio_rpm;
static uint32_t ultimo_invio_temp;

void engine_setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));
    rpm_corrente = MIN_RPM;
    temp_corrente = MIN_TEMP;
    ultimo_invio_rpm = 0;
    ultimo_invio_temp = 0;
}

void engine_loop() {
    if(millis() - ultimo_invio_rpm >= 100){
        aggiorna_rpm();
        CanFrame frame;
        frame->
    }
}

void aggiorna_rpm() {
    int incremento = random(-200, 201);  // nota 201 per includere +200
    int nuovo = (int)rpm_corrente + incremento;
    if (nuovo > MAX_RPM) nuovo = MAX_RPM;
    if (nuovo < MIN_RPM) nuovo = MIN_RPM;
    rpm_corrente = (uint16_t)nuovo;
}

void aggiorna_temperatura(uint16_t rpm){
    if(rpm >= 3000){
        if (temp_corrente < MAX_TEMP) temp_corrente++;
    } else  {
        if (temp_corrente > MIN_TEMP) temp_corrente--;
    }
}