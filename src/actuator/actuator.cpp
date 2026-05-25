#include <Arduino.h>
#include "actuator.h"
#include "frame.h"
#include "protocol.h"

void elabora_frame(CanFrame* frame);
static bool safe_write(uint8_t byte);

// --- Macchina a stati del parser -----------------------------
enum StatoCorrente {
    ATTESA_START,
    LETTURA_ID,
    LETTURA_SEQ,
    LETTURA_LUNGHEZZA,
    LETTURA_DATI,
    LETTURA_CRC,
    VERIFICA_END
};

// --- Variabili di supporto del parser ------------------------
static uint32_t timestamp_ultimo_byte = 0;
static StatoCorrente stato = ATTESA_START;
static uint8_t buf_frame[PROTO_MAX_DATA_LEN + 6];
static uint8_t buf_index = 0;
static uint8_t dati_attesi = 0;

void actuator_setup() {
    Serial.begin(115200);
}

void actuator_loop() {
    parser_uart();
}

/**
 * Parser UART a macchina a stati.
 * Identico al Dashboard — ricostruisce i frame protocan dal flusso UART.
 * Frame corrotti o malformati vengono scartati silenziosamente.
 * I frame validi vengono passati a elabora_frame().
 */
void parser_uart(){
    while(Serial.available()){
        if (millis() - timestamp_ultimo_byte > 100) {
            stato     = ATTESA_START;
            buf_index = 0;
        }
        timestamp_ultimo_byte = millis();

        uint8_t byte = Serial.read();
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
                stato = LETTURA_SEQ;
                break;
            case LETTURA_SEQ:
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
 * Elabora i frame CMD ricevuti dal Dashboard.
 * Attiva il buzzer per allarme RPM, il LED per allarme temperatura.
 * Tutti gli altri MSG_ID vengono ignorati.
 */
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