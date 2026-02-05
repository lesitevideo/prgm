/**
 * @file main.cpp
 * @brief Firmware principal de l'ancre UWB (pylône)
 *
 * Reçoit les blinks UWB, horodate au niveau matériel,
 * et transmet les données via RS485 vers la base.
 */

#include <Arduino.h>
#include "config.h"
#include "protocol.h"
// #include "uwb.h"
// #include "rs485.h"
// #include "sync.h"

// === Buffer circulaire pour les blinks reçus ===
#define BLINK_BUFFER_SIZE 32

typedef struct {
    uint16_t tag_id;
    uint16_t seq_num;
    uint64_t timestamp;
    int8_t rssi;
    bool valid;
} blink_entry_t;

static blink_entry_t blink_buffer[BLINK_BUFFER_SIZE];
static volatile uint8_t write_idx = 0;
static uint8_t read_idx = 0;

// === Synchronisation ===
static uint32_t last_sync_time = 0;

// === Prototypes ===
void setup_hardware();
void on_uwb_rx_callback();
void process_blink_buffer();
void send_rs485_message(const blink_entry_t* blink);
void handle_sync();

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Ancre UWB Pylon Racing ===");
    Serial.printf("Anchor ID: 0x%02X\n", ANCHOR_ID);
    Serial.printf("Mode: %s\n", IS_MASTER ? "MASTER" : "SLAVE");

    setup_hardware();

    // TODO: Initialiser UWB en mode réception
    // uwb_init_rx(on_uwb_rx_callback);

    // TODO: Initialiser RS485
    // rs485_init(RS485_BAUDRATE);

    // TODO: Initialiser synchronisation
    // sync_init(IS_MASTER);

    Serial.println("Initialisation terminée. En attente de blinks...");
}

void loop() {
    // Traiter les blinks en attente
    process_blink_buffer();

    // Gestion de la synchronisation
    handle_sync();

    // TODO: Watchdog
}

void setup_hardware() {
    // LED status
    pinMode(LED_BUILTIN, OUTPUT);

    // RS485 direction pin
    pinMode(PIN_RS485_DE, OUTPUT);
    digitalWrite(PIN_RS485_DE, LOW);  // Mode réception par défaut

    // TODO: Configuration SPI pour DW1000
    // TODO: Configuration UART2 pour RS485
}

/**
 * @brief Callback appelé par l'IRQ du DW1000 lors de la réception d'un blink
 *
 * CRITIQUE : Cette fonction doit être aussi rapide que possible.
 * Elle lit le timestamp matériel et stocke les données dans le buffer.
 */
void IRAM_ATTR on_uwb_rx_callback() {
    // CRITIQUE : Lire le timestamp matériel EN PREMIER
    // uint64_t timestamp = uwb_read_rx_timestamp();

    // Placeholder pour le développement
    uint64_t timestamp = esp_timer_get_time();

    // Extraire les données de la trame (à implémenter)
    uint16_t tag_id = 0;     // TODO: uwb_get_tag_id();
    uint16_t seq_num = 0;    // TODO: uwb_get_seq_num();
    int8_t rssi = 0;         // TODO: uwb_get_rssi();

    // Stocker dans le buffer circulaire
    uint8_t idx = write_idx;
    blink_buffer[idx].tag_id = tag_id;
    blink_buffer[idx].seq_num = seq_num;
    blink_buffer[idx].timestamp = timestamp;
    blink_buffer[idx].rssi = rssi;
    blink_buffer[idx].valid = true;

    write_idx = (idx + 1) % BLINK_BUFFER_SIZE;

    // TODO: Réarmer la réception UWB
    // uwb_start_rx();
}

/**
 * @brief Traite les blinks du buffer et les envoie via RS485
 */
void process_blink_buffer() {
    while (read_idx != write_idx) {
        blink_entry_t* blink = &blink_buffer[read_idx];

        if (blink->valid) {
            send_rs485_message(blink);
            blink->valid = false;

            #if DEBUG_SERIAL
            Serial.printf("Blink: tag=0x%04X seq=%u ts=%llu rssi=%d\n",
                blink->tag_id, blink->seq_num, blink->timestamp, blink->rssi);
            #endif
        }

        read_idx = (read_idx + 1) % BLINK_BUFFER_SIZE;
    }
}

/**
 * @brief Envoie un message RS485 formaté selon le protocole
 */
void send_rs485_message(const blink_entry_t* blink) {
    uint8_t msg[RS485_MSG_SIZE];

    // Construire le message
    msg[0] = RS485_SYNC_BYTE;
    msg[1] = ANCHOR_ID;
    msg[2] = blink->tag_id & 0xFF;
    msg[3] = (blink->tag_id >> 8) & 0xFF;
    msg[4] = blink->seq_num & 0xFF;
    msg[5] = (blink->seq_num >> 8) & 0xFF;
    msg[6] = blink->timestamp & 0xFF;
    msg[7] = (blink->timestamp >> 8) & 0xFF;
    msg[8] = (blink->timestamp >> 16) & 0xFF;
    msg[9] = (blink->timestamp >> 24) & 0xFF;
    msg[10] = (blink->timestamp >> 32) & 0xFF;
    msg[11] = blink->rssi;

    // Calculer CRC16
    uint16_t crc = calculate_crc16(msg, 12);
    msg[12] = crc & 0xFF;
    msg[13] = (crc >> 8) & 0xFF;

    // Envoyer via RS485
    // rs485_send(msg, RS485_MSG_SIZE);

    // Placeholder : afficher sur Serial
    #if DEBUG_SERIAL > 1
    Serial.print("RS485 TX: ");
    for (int i = 0; i < RS485_MSG_SIZE; i++) {
        Serial.printf("%02X ", msg[i]);
    }
    Serial.println();
    #endif
}

/**
 * @brief Gère la synchronisation temporelle
 */
void handle_sync() {
    uint32_t now = millis();

    #if IS_MASTER
    // Mode maître : émettre périodiquement une trame de sync
    if (now - last_sync_time >= SYNC_INTERVAL_MS) {
        // TODO: sync_emit();
        last_sync_time = now;

        #if DEBUG_SERIAL
        Serial.println("Sync émis");
        #endif
    }
    #else
    // Mode esclave : vérifier la réception de sync
    // TODO: sync_check_timeout();
    #endif
}

/**
 * @brief Calcule le CRC16-CCITT
 */
uint16_t calculate_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0x8408;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
