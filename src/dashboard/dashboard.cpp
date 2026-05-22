#include <Arduino.h>
#include "protocol.h"
#include "dashboard.h"
#include "frame.h"


void elabora_frame(CanFrame* frame);
static bool safe_write(uint8_t byte);

// --- Macchina a stati del parser -----------------------------
enum StatoCorrente {
    ATTESA_START,
    LETTURA_ID,
    LETTURA_LUNGHEZZA,
    LETTURA_DATI,
    LETTURA_CRC,
    VERIFICA_END
};

// --- Stato globale della Dashboard ---------------------------
static uint32_t timestamp_ultimo_byte = 0;
static uint16_t rpm_attuale;
static uint8_t  temp_attuale;
static bool allarme_rpm;
static bool allarme_temp;
static bool allarme_rpm_precedente;
static bool allarme_temp_precedente;

// --- Variabili di supporto del parser ------------------------
static StatoCorrente stato = ATTESA_START;
static uint8_t buf_frame[PROTO_MAX_DATA_LEN + 5];
static uint8_t buf_index = 0;
static uint8_t dati_attesi = 0;

/**
 * Inizializza la UART e lo stato interno del Dashboard.
 */
void dashboard_setup() {
    Serial.begin(115200);
    rpm_attuale = 0;
    temp_attuale = 0;
    allarme_rpm = false;
    allarme_temp = false;
    allarme_rpm_precedente = false;
    allarme_temp_precedente = false;
}


void dashboard_loop() {
    parser_uart();
}

/**
 * Parser UART a macchina a stati.
 * Legge il flusso di byte dalla UART e ricostruisce i frame protocan.
 * Ogni byte ricevuto avanza la macchina di uno stato.
 * Frame corrotti o malformati vengono scartati silenziosamente.
 * I frame validi vengono passati a elabora_frame().
 */
void parser_uart(){
    while(Serial.available()){
        uint8_t byte = Serial.read();
        
        if (millis() - timestamp_ultimo_byte > 100) {  // 100ms senza byte = timeout
            stato = ATTESA_START;
            buf_index = 0;
        }
        timestamp_ultimo_byte = millis();

        switch (stato){
            case ATTESA_START:
                if(byte == PROTO_START_BYTE){
                    buf_index = 0;
                    if(!safe_write(byte)) break; //Scrive in posizione 0, poi buf_index diventa 1
                    stato = LETTURA_ID; 
                }
                break;
            case LETTURA_ID:
                if(!safe_write(byte)) break;
                stato = LETTURA_LUNGHEZZA;
                break;
            case LETTURA_LUNGHEZZA:
                if(!safe_write(byte)) break;
                dati_attesi = byte;

                if(dati_attesi == 0 || dati_attesi > PROTO_MAX_DATA_LEN){
                    stato = ATTESA_START;   // LEN fuori range, frame corrotto
                    buf_index = 0;
                }else{
                    stato = LETTURA_DATI;
                }

                break;
            case LETTURA_DATI:
                if(!safe_write(byte)) break;
                dati_attesi--;

                if(dati_attesi == 0)
                    stato = LETTURA_CRC;

                break;
            case LETTURA_CRC:
                if(!safe_write(byte)) break;
                stato = VERIFICA_END;
                break;
            case VERIFICA_END:
                CanFrame frame;
                if(byte == PROTO_END_BYTE){
                    if(!safe_write(byte)) break;
                    if(deserializza(buf_frame, buf_index, &frame))
                        elabora_frame(&frame);  // frame valido, passa all'elaborazione
                }

                stato = ATTESA_START;
                buf_index = 0;
                break;
            default:
                stato = ATTESA_START;   // stato non previsto, reset
                buf_index = 0;
                break;
        }
    }
}

/**
 * Elabora un frame valido ricevuto dal parser.
 * Aggiorna lo stato globale del Dashboard e genera eventuali comandi.
 */
void elabora_frame(CanFrame* frame){
    switch (frame->msg_id)
    {
    case MSG_ID_RPM:
        rpm_attuale = (frame->data[0] << 8) | frame->data[1];
        Serial.print("RPM: ");
        Serial.println(rpm_attuale);
        if(rpm_attuale > SOGLIA_RPM_ALLARME){
            allarme_rpm = true;
            Serial.println("ATTENZIONE: SOGLIA RPM SUPERATA");
        }else{
            allarme_rpm = false;
        }

        break;
    case MSG_ID_TEMP:
        temp_attuale = frame->data[0] & 0xFF;
        Serial.print("TEMP: ");
        Serial.println(temp_attuale);

        if(temp_attuale > SOGLIA_TEMP_ALLARME){
            allarme_temp = true;
            Serial.println("ATTENZIONE: SOGLIA TEMPERATURA SUPERATA");
        }else{
            allarme_temp = false;
        }

        break;
    case MSG_ID_ANOMALY:
        /* code */
        break;
    case MSG_ID_CMD:
        /* code */
        break;
    
    default:
        break;
    }

    // In caso di allarme attivo invio il frame al nodo Actuator
    if(allarme_rpm && !allarme_rpm_precedente){
        CanFrame frameActuatorRpm;
        frameActuatorRpm.msg_id = MSG_ID_CMD;
        frameActuatorRpm.len = CMD_DATA_LEN;
        frameActuatorRpm.data[0] = CMD_ALARM_RPM;

        uint8_t buf[PROTO_MAX_DATA_LEN + 5];
        uint8_t n = serializza(&frameActuatorRpm, buf);

        Serial.write(buf, n);
    }

    if(allarme_temp && ! allarme_temp_precedente){
        CanFrame frameActuatorTemp;
        frameActuatorTemp.msg_id = MSG_ID_CMD;
        frameActuatorTemp.len = CMD_DATA_LEN;
        frameActuatorTemp.data[0] = CMD_ALARM_TEMP;

        uint8_t buf[PROTO_MAX_DATA_LEN + 5];
        uint8_t n = serializza(&frameActuatorTemp, buf);

        Serial.write(buf, n);
    }

    allarme_rpm_precedente = allarme_rpm;
    allarme_temp_precedente = allarme_temp;

}

/**
 * Scrive un byte nel buffer del frame in costruzione.
 * Controlla che buf_index non superi la dimensione massima del buffer
 * prima di scrivere — protegge da overflow in caso di frame malformati
 * o rumore sulla linea UART.
 * In caso di overflow resetta il parser ad ATTESA_START.
 *
 * @param byte  byte da scrivere nel buffer
 * @return      true se la scrittura è avvenuta, false se overflow rilevato
 */
static bool safe_write(uint8_t byte) {
    if (buf_index >= sizeof(buf_frame)) {
        stato     = ATTESA_START;
        buf_index = 0;
        return false;
    }
    buf_frame[buf_index++] = byte;
    return true;
}