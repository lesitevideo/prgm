/**
 * @file protocol.h
 * @brief Définition du protocole RS485
 *
 * Format des trames échangées entre les ancres et la base.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// === Constantes du protocole ===
#define RS485_SYNC_BYTE     0xAA
#define RS485_MSG_SIZE      14

// === Structure d'un message blink ===
// Format binaire (little-endian) :
// [SYNC:1][ANCHOR_ID:1][TAG_ID:2][SEQ_NUM:2][TIMESTAMP:5][RSSI:1][CRC16:2]

typedef struct __attribute__((packed)) {
    uint8_t sync;           // 0xAA
    uint8_t anchor_id;      // ID de l'ancre
    uint16_t tag_id;        // ID du tag (little-endian)
    uint16_t seq_num;       // Numéro de séquence (little-endian)
    uint8_t timestamp[5];   // Timestamp 40-bit (little-endian)
    int8_t rssi;            // RSSI (dBm, signé)
    uint16_t crc;           // CRC16-CCITT (little-endian)
} rs485_blink_msg_t;

// === Messages de synchronisation ===
#define SYNC_MSG_SYNC_BYTE  0x55
#define SYNC_MSG_SIZE       10

typedef struct __attribute__((packed)) {
    uint8_t sync;           // 0x55
    uint8_t master_id;      // ID de l'ancre maître
    uint32_t sync_count;    // Compteur de sync
    uint8_t timestamp[5];   // Timestamp maître 40-bit
    uint16_t crc;           // CRC16-CCITT
} rs485_sync_msg_t;

// === Messages de status ===
#define STATUS_MSG_SYNC_BYTE 0xBB
#define STATUS_MSG_SIZE     8

typedef struct __attribute__((packed)) {
    uint8_t sync;           // 0xBB
    uint8_t anchor_id;      // ID de l'ancre
    uint8_t status;         // 0=OK, 1=DEGRADED, 2=ERROR
    int16_t drift_ppm;      // Dérive en ppm (signé)
    uint16_t last_sync_age; // Age dernière sync (ms)
    uint16_t crc;           // CRC16-CCITT
} rs485_status_msg_t;

// === Valeurs de status ===
#define STATUS_OK           0
#define STATUS_DEGRADED     1
#define STATUS_ERROR        2

// === Fonction CRC ===
uint16_t calculate_crc16(const uint8_t* data, size_t len);

#endif // PROTOCOL_H
