#include <stdint.h>
#include "frame.h"

/**
 * Calcola il CRC del frame tramite XOR dei campi logici.
 * Il CRC copre MSG_ID, LEN e tutti i byte di DATA.
 * START, END e il CRC stesso sono esclusi dal calcolo.
 * 
 * @param frame  puntatore al frame da cui calcolare il CRC
 * @return       byte CRC calcolato
 */
uint8_t calcola_crc(const CanFrame* frame) {
    uint8_t crc = frame->msg_id ^ frame->len;  // inizializza con XOR dei campi fissi
    for (int i = 0; i < frame->len; i++) {
        crc ^= frame->data[i];  // accumula XOR di ogni byte del payload
    }
    return crc;
}

/**
 * Serializza un frame nella sequenza di byte da trasmettere sulla UART.
 * Il buffer di output deve essere allocato dal chiamante con dimensione
 * minima di PROTO_MAX_DATA_LEN + 5 byte.
 * 
 * Formato prodotto: [START][MSG_ID][LEN][DATA...][CRC][END]
 * 
 * @param frame    puntatore al frame da serializzare
 * @param buf_out  buffer di output dove scrivere i byte
 * @return         numero di byte scritti nel buffer
 */
uint8_t serializza(const CanFrame* frame, uint8_t* buf_out) {
    buf_out[0] = PROTO_START_BYTE;  // marcatore di inizio frame
    buf_out[1] = frame->msg_id;     // identificatore tipo messaggio
    buf_out[2] = frame->len;        // lunghezza del payload DATA

    for (int i = 0; i < frame->len; i++) {
        buf_out[i + 3] = frame->data[i];  // copia il payload byte per byte
    }

    buf_out[frame->len + 3] = calcola_crc(frame);  // CRC calcolato sul frame logico
    buf_out[frame->len + 4] = PROTO_END_BYTE;       // marcatore di fine frame

    return frame->len + 5;  // totale byte: START + MSG_ID + LEN + DATA + CRC + END
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

    // lunghezza minima: START + MSG_ID + LEN + CRC + END = 5 byte
    if (len < 5) return false;

    // verifica byte di sincronizzazione
    if (buf[0] != PROTO_START_BYTE) return false;
    if (buf[len - 1] != PROTO_END_BYTE) return false;

    // estrae i campi fissi
    out->msg_id = buf[1];
    out->len    = buf[2];

    // verifica che la lunghezza dichiarata sia coerente con il buffer ricevuto
    if (out->len > PROTO_MAX_DATA_LEN) return false; // lunghezza data
    if (len != out->len + 5) return false;           // lunghezza intero frame

    // copia il payload
    for (int i = 0; i < out->len; i++) {
        out->data[i] = buf[i + 3];
    }

    // estrae il CRC ricevuto e lo confronta con quello ricalcolato
    out->crc = buf[out->len + 3];
    if (out->crc != calcola_crc(out)) return false;

    return true;
}