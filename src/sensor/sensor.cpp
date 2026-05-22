#include <Arduino.h>
#include "sensor.h"
#include "frame.h"
#include "protocol.h"

// --- Tipi di anomalia disponibili ----------------------------
const uint8_t anomalie[3] = {ANOMALY_NOISE, ANOMALY_OUT_OF_RANGE, ANOMALY_TIMEOUT}; 

// --- Variabili di stato --------------------------------------
static uint32_t ultimo_invio_anomaly; // timestamp ultimo invio
static uint8_t  intervallo;           // intervallo random tra un invio e l'altro

void sensor_setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));
    ultimo_invio_anomaly = 0;
    intervallo = random(1000, 5000);    // primo intervallo casuale
}

/**
 * Invia un frame ANOMALY casuale a intervalli casuali tra 1000 e 5000ms.
 * Il tipo di anomalia viene scelto casualmente tra NOISE, OUT_OF_RANGE e TIMEOUT.
 */
void sensor_loop() {
    if((millis() - ultimo_invio_anomaly) >= intervallo){
        uint8_t tipo = anomalie[random(0,3)];
        CanFrame frame;

        frame.msg_id = MSG_ID_ANOMALY;
        frame.len = ANOMALY_DATA_LEN;
        frame.data[0] = tipo;
        frame.crc = calcola_crc(&frame);

        uint8_t buf[PROTO_MAX_DATA_LEN + 5];
        uint8_t n = serializza(&frame, buf);

        Serial.write(buf, n);
        ultimo_invio_anomaly = millis();
        intervallo = random(1000,5000);
    }
}