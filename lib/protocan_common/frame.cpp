#include <stdint.h>
#include "frame.h"

static uint8_t crc8_table[256];
static bool table_inizializzata = false;

/**
 * Genera a runtime la lookup table per il calcolo CRC-8/SMBUS.
 * Usa il polinomio generatore 0x07 (x⁸ + x² + x + 1).
 * 
 * Per ogni valore possibile di un byte (0-255) simula la divisione
 * polinomiale bit per bit — se il bit più alto è 1 c'è un resto,
 * si applica XOR con il polinomio; se è 0 si shifta e basta.
 * 
 * Va chiamata una sola volta prima di usare calcola_crc().
 * Il risultato viene salvato in crc8_table[256].
 */
static void inizializza_tabella() {
    for (int i = 0; i < 256; i++) {
        uint8_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
        crc8_table[i] = crc;
    }
    table_inizializzata = true;
}

/**
 * Calcola il CRC-8/SMBUS del frame usando il polinomio 0x07.
 * Il CRC copre MSG_ID, LEN e tutti i byte di DATA, in questo ordine.
 * START, END e il CRC stesso sono esclusi dal calcolo.
 * 
 * Più robusto del semplice XOR — rileva byte scambiati di posizione
 * e pattern di errore che XOR non individua.
 * 
 * @param frame  puntatore al frame da cui calcolare il CRC
 * @return       byte CRC-8 calcolato
 */
uint8_t calcola_crc(const CanFrame* frame) {
    if (!table_inizializzata) inizializza_tabella();

    uint8_t crc = 0x00;

    crc = crc8_table[crc ^ frame->msg_id];
    crc = crc8_table[crc ^ frame->seq];
    crc = crc8_table[crc ^ frame->len];

    for (int i = 0; i < frame->len; i++) {
        crc = crc8_table[crc ^ frame->data[i]];  // accumula XOR di ogni byte del payload
    }
    return crc;
}

/**
 * Serializza un frame nella sequenza di byte da trasmettere sulla UART.
 * Il buffer di output deve essere allocato dal chiamante con dimensione
 * minima di PROTO_MAX_DATA_LEN + 5 byte.
 * 
 * Formato prodotto: [START][MSG_ID][SEQ][LEN][DATA...][CRC][END]
 * 
 * @param frame    puntatore al frame da serializzare
 * @param buf_out  buffer di output dove scrivere i byte
 * @return         numero di byte scritti nel buffer
 */
uint8_t serializza(const CanFrame* frame, uint8_t* buf_out) {
    buf_out[0] = PROTO_START_BYTE;  // marcatore di inizio frame
    buf_out[1] = frame->msg_id;     // identificatore tipo messaggio
    buf_out[2] = frame->seq;        // sequenza del frame
    buf_out[3] = frame->len;        // lunghezza del payload DATA

    for (int i = 0; i < frame->len; i++) {
        buf_out[i + 4] = frame->data[i];  // copia il payload byte per byte
    }

    buf_out[frame->len + 4] = calcola_crc(frame);  // CRC calcolato sul frame logico
    buf_out[frame->len + 5] = PROTO_END_BYTE;       // marcatore di fine frame

    return frame->len + 6;  // totale byte: START + MSG_ID + SEQ + LEN + DATA + CRC + END
}

/**
 * Deserializza un buffer di byte ricevuto dalla UART in un CanFrame.
 * Valida START, END, lunghezza minima e CRC prima di accettare il frame.
 *
 * @param buf   buffer di byte ricevuti
 * @param len   lunghezza del buffer
 * @param out   puntatore al frame di output popolato se valido
 * @return      true se il frame è valido, false altrimenti
 */
bool deserializza(const uint8_t* buf,uint8_t len, CanFrame* out){

    // lunghezza minima: START + MSG_ID + SEQ + LEN + CRC + END = 6 byte
    if (len < 6) return false;

    // verifica byte di sincronizzazione
    if (buf[0] != PROTO_START_BYTE) return false;
    if (buf[len - 1] != PROTO_END_BYTE) return false;

    // estrae i campi fissi
    out->msg_id = buf[1];
    out->seq    = buf[2];
    out->len    = buf[3];

    // verifica che la lunghezza dichiarata sia coerente con il buffer ricevuto
    if (out->len > PROTO_MAX_DATA_LEN) return false; // lunghezza data
    if (len != out->len + 6) return false;           // lunghezza intero frame

    // copia il payload
    for (int i = 0; i < out->len; i++) {
        out->data[i] = buf[i + 4];
    }

    // estrae il CRC ricevuto e lo confronta con quello ricalcolato
    out->crc = buf[out->len + 4];
    if (out->crc != calcola_crc(out)) return false;

    return true;
}