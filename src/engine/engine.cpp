#include <stdint.h>
#include <Arduino.h>
#include "frame.h"


#define MAX_RPM 6000
#define MIN_RPM 800
#define MAX_TEMP 110
#define MIN_TEMP 60

// Stato interno del nodo
static uint16_t rpm_corrente;
static uint8_t  temp_corrente;
static uint32_t ultimo_invio_rpm;   // timestamp ultimo frame RPM inviato
static uint32_t ultimo_invio_temp;  // timestamp ultimo frame TEMP inviato


// Inizializzazione della UART e del nodo Engine
void engine_setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));
    rpm_corrente = MIN_RPM;
    temp_corrente = MIN_TEMP;
    ultimo_invio_rpm = 0;
    ultimo_invio_temp = 0;
}

/**
 * Aggiorna il valore simulato di RPM applicando un incremento casuale.
 * Il valore rimane nel range [MIN_RPM, MAX_RPM].
 */
void aggiorna_rpm() {
    int incremento = random(-200, 201);  // nota 201 per includere +200
    int nuovo = (int)rpm_corrente + incremento;
    if (nuovo > MAX_RPM) nuovo = MAX_RPM;
    if (nuovo < MIN_RPM) nuovo = MIN_RPM;
    rpm_corrente = (uint16_t)nuovo;
}


/**
 * Aggiorna la temperatura simulata in base al regime motore.
 * Sopra 3000 RPM la temperatura sale, sotto scende.
 * Il valore rimane nel range [MIN_TEMP, MAX_TEMP].
 * 
 * @param rpm  valore RPM corrente
 */
void aggiorna_temperatura(uint16_t rpm){
    if(rpm >= 3000){
        if (temp_corrente < MAX_TEMP) temp_corrente++;
    } else  {
        if (temp_corrente > MIN_TEMP) temp_corrente--;
    }
}

/**
 * Loop principale del nodo Engine.
 * Trasmette RPM ogni 100ms e temperatura ogni 500ms sulla UART.
 * I frame seguono il formato protocan: [START][MSG_ID][LEN][DATA][CRC][END]
 */
void engine_loop() {

    // Invio RPM ognni 100ms
    if(millis() - ultimo_invio_rpm >= 100){
        aggiorna_rpm();

        CanFrame frame;
        frame.msg_id = MSG_ID_RPM;
        frame.len = RPM_DATA_LEN;
        frame.data[0] = (rpm_corrente >> 8) & 0xFF;
        frame.data[1] = rpm_corrente & 0xFF;
        frame.crc = calcola_crc(&frame);

        uint8_t buf[PROTO_MAX_DATA_LEN + 5];
        uint8_t n = serializza(&frame, buf);

        Serial.write(buf, n);
        ultimo_invio_rpm = millis();
    }

    // Invio Temperatura ogni 500ms
    if(millis() - ultimo_invio_temp >= 500){
        aggiorna_temperatura(rpm_corrente);

        CanFrame frame;
        frame.msg_id = MSG_ID_TEMP;
        frame.len = TEMP_DATA_LEN;
        frame.data[0] = temp_corrente;
        frame.crc = calcola_crc(&frame);

        uint8_t buf[PROTO_MAX_DATA_LEN + 5];
        uint8_t n = serializza(&frame, buf);

        Serial.write(buf, n);
        ultimo_invio_temp = millis();
    }

}
