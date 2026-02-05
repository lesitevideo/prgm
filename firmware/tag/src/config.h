/**
 * @file config.h
 * @brief Configuration du tag UWB
 */

#ifndef CONFIG_H
#define CONFIG_H

// === Identification ===
#ifndef TAG_ID
#define TAG_ID              0x0010      // Identifiant unique du tag (0x0001-0xFFFE)
#endif

// === Timing blinks ===
#ifndef BLINK_RATE_HZ
#define BLINK_RATE_HZ       30          // Fréquence d'émission (20-40 Hz)
#endif

#define JITTER_MAX_MS       2           // Jitter max ±2ms
#define NUM_SLOTS           4           // Nombre de slots (= nombre max d'avions)

// === Configuration UWB ===
#define UWB_CHANNEL         5           // Canal UWB (1-7, 5 recommandé)
#define UWB_PRF             2           // Pulse Rep Freq: 1=16MHz, 2=64MHz
#define UWB_PREAMBLE_LEN    128         // Longueur préambule
#define UWB_DATA_RATE       2           // 0=110kbps, 1=850kbps, 2=6.8Mbps

// === Pinout ESP32-C3 ===
#define PIN_SPI_CLK         4
#define PIN_SPI_MISO        5
#define PIN_SPI_MOSI        6
#define PIN_SPI_CS          7
#define PIN_DW1000_IRQ      8
#define PIN_DW1000_RST      9

// === Pinout ESP32 classique (alternative) ===
// #define PIN_SPI_CLK      18
// #define PIN_SPI_MISO     19
// #define PIN_SPI_MOSI     23
// #define PIN_SPI_CS       5
// #define PIN_DW1000_IRQ   4
// #define PIN_DW1000_RST   2

// === Debug ===
#ifndef DEBUG_SERIAL
#define DEBUG_SERIAL        1           // 1 = activer logs série
#endif

// === Power management ===
#define ENABLE_DEEP_SLEEP   0           // 1 = deep sleep entre blinks (économie batterie)
#define BATTERY_CHECK_INTERVAL 1000     // Vérification batterie toutes les N blinks

#endif // CONFIG_H
