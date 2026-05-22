#include <Arduino.h>
#include "actuator.h"
#include "frame.h"
#include "protocol.h"

void elabora_frame(CanFrame* frame);

// --- Macchina a stati del parser -----------------------------
enum StatoCorrente {
    ATTESA_START,
    LETTURA_ID,
    LETTURA_LUNGHEZZA,
    LETTURA_DATI,
    LETTURA_CRC,
    VERIFICA_END
};

// --- Variabili di supporto del parser ------------------------
static StatoCorrente stato = ATTESA_START;
static uint8_t buf_frame[PROTO_MAX_DATA_LEN + 5];
static uint8_t buf_index = 0;
static uint8_t dati_attesi = 0;

void actuator_setup() {
    Serial.begin(115200);
}

void actuator_loop() {
    parser_uart();
}

void parser_uart(){
    while(Serial.available()){
        uint8_t byte = Serial.read();
        switch (stato){
            case ATTESA_START:
                if(byte == PROTO_START_BYTE){
                    buf_index = 0;
                    buf_frame[buf_index++] = byte; //Scrive in posizione 0, poi buf_index diventa 1
                    stato = LETTURA_ID; 
                }
                break;
            case LETTURA_ID:
                buf_frame[buf_index++] = byte;
                stato = LETTURA_LUNGHEZZA;
                break;
            case LETTURA_LUNGHEZZA:
                buf_frame[buf_index++] = byte;
                dati_attesi = byte;

                if(dati_attesi > PROTO_MAX_DATA_LEN){
                    stato = ATTESA_START;   // LEN fuori range, frame corrotto
                    buf_index = 0;
                }else{
                    stato = LETTURA_DATI;
                }

                break;
            case LETTURA_DATI:
                buf_frame[buf_index++] = byte;
                dati_attesi--;

                if(dati_attesi == 0)
                    stato = LETTURA_CRC;

                break;
            case LETTURA_CRC:
                buf_frame[buf_index++] = byte;
                stato = VERIFICA_END;
                break;
            case VERIFICA_END:
                CanFrame frame;
                if(byte == PROTO_END_BYTE){
                    buf_frame[buf_index++] = byte;
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


void elabora_frame(CanFrame* frame){
    if(frame->msg_id == MSG_ID_CMD){
        switch (frame->data[0])
        {
        case CMD_ALARM_RPM:
            Serial.println("ALLARME RPM: buzzer attivato");
            break;
        case CMD_ALARM_TEMP:
            Serial.println("ALLARME TEMPERATURA: LED acceso");
            break;
        default:
            break;
        }
    }
}