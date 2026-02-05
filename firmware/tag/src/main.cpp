/**
 * @file main.cpp
 * @brief Firmware principal du tag UWB (avion)
 *
 * Émet des blinks UWB périodiques pour la détection de passage pylône.
 */

#include <Arduino.h>
#include "config.h"
// #include "uwb.h"
// #include "blink.h"

// === État global ===
static uint16_t sequence_number = 0;
static uint32_t last_blink_time = 0;

// === Prototypes ===
void setup_hardware();
void emit_blink();
uint16_t calculate_slot_offset();
int16_t calculate_jitter();

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Tag UWB Pylon Racing ===");
    Serial.printf("Tag ID: 0x%04X\n", TAG_ID);
    Serial.printf("Blink rate: %d Hz\n", BLINK_RATE_HZ);

    setup_hardware();

    // TODO: Initialiser UWB
    // uwb_init();

    Serial.println("Initialisation terminée. Début des blinks.");
}

void loop() {
    uint32_t now = millis();
    uint32_t blink_interval = 1000 / BLINK_RATE_HZ;

    // Calculer le timing avec slotting et jitter
    uint16_t slot_offset = calculate_slot_offset();
    int16_t jitter = calculate_jitter();

    uint32_t next_blink = last_blink_time + blink_interval + slot_offset + jitter;

    if (now >= next_blink) {
        emit_blink();
        last_blink_time = now;
        sequence_number++;
    }

    // TODO: Deep sleep entre les blinks pour économiser la batterie
    // delay(1); // Placeholder
}

void setup_hardware() {
    // LED status
    pinMode(LED_BUILTIN, OUTPUT);

    // TODO: Configuration SPI pour DW1000
    // SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, SPI_CS);
}

void emit_blink() {
    // Indicateur visuel
    digitalWrite(LED_BUILTIN, HIGH);

    // TODO: Construire et émettre la trame UWB
    // blink_frame_t frame = {
    //     .tag_id = TAG_ID,
    //     .seq_num = sequence_number,
    // };
    // uwb_send_blink(&frame);

    #if DEBUG_SERIAL
    if (sequence_number % 100 == 0) {
        Serial.printf("Blink #%u émis\n", sequence_number);
    }
    #endif

    digitalWrite(LED_BUILTIN, LOW);
}

uint16_t calculate_slot_offset() {
    // Slotting basé sur l'ID du tag
    // Chaque tag a un slot différent dans la période
    uint16_t slot = TAG_ID % NUM_SLOTS;
    uint16_t slot_duration_ms = (1000 / BLINK_RATE_HZ) / NUM_SLOTS;
    return slot * slot_duration_ms;
}

int16_t calculate_jitter() {
    // Jitter pseudo-aléatoire pour éviter les collisions
    // Simple PRNG basé sur sequence_number
    uint32_t seed = sequence_number * 1103515245 + 12345;
    int16_t jitter = (seed % (2 * JITTER_MAX_MS + 1)) - JITTER_MAX_MS;
    return jitter;
}
