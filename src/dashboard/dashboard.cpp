#include <Arduino.h>
#include "protocol.h"
#include "dashboard.h"
#include "frame.h"

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

// --- Stato globale della Dashboard ---------------------------
static uint16_t rpm_attuale;
static uint8_t  temp_attuale;
static bool     allarme_rpm;
static bool     allarme_temp;

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

/**
 * Elabora un frame valido ricevuto dal parser.
 * Aggiorna lo stato globale del Dashboard e genera eventuali comandi.
 * TODO: da implementare
 */
void elabora_frame(CanFrame* frame){
    
}