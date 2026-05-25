#pragma once

#include <stdint.h>
#include "protocol.h"

// --- Struttura del frame di comunicazione ---
typedef struct {
    uint8_t msg_id;
    uint8_t seq;
    uint8_t len;
    uint8_t data[PROTO_MAX_DATA_LEN];
    uint8_t crc;
} CanFrame;

// --- Funzioni -------------------------------

// Calcola il CRC del frame (esclusi start, end e crc)
uint8_t calcola_crc(const CanFrame* frame); 

// Prende un frame, riempie un buffer di byte pronti per la UART
uint8_t serializza(const CanFrame* frame, uint8_t* buf_out);

// Prende un buffer di byte ricevuti, restituisce un frame oppure un errore
bool    deserializza(const uint8_t* buf,uint8_t len, CanFrame* out);
