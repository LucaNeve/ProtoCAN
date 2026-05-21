#pragma once

#include <stdint.h>

// --- Byte di protocollo -----------------------------
#define PROTO_START_BYTE            0x7E
#define PROTO_END_BYTE              0x7F
#define PROTO_MAX_DATA_LEN          8

// --- Message ID -------------------------------------
#define MSG_ID_RPM                  0x01
#define MSG_ID_TEMP                 0x02
#define MSG_ID_ANOMALY              0x03
#define MSG_ID_CMD                  0x10

// --- Tipi di anomalia (DATA[0] nei frame ANOMALY) ---
#define ANOMALY_NOISE               0x01
#define ANOMALY_OUT_OF_RANGE        0x02
#define ANOMALY_TIMEOUT             0x03

// --- Tipi di comando (DATA[0] nei frame CMD) --------
#define CMD_ALARM_RPM               0x01
#define CMD_ALARM_TEMP              0x02
