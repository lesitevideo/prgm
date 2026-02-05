/**
 * @file config.h
 * @brief Configuration de l'ancre UWB
 */

#ifndef CONFIG_H
#define CONFIG_H

// === Identification ===
#ifndef ANCHOR_ID
#define ANCHOR_ID           0x11        // Identifiant unique (0x11-0x14 pylône 1, 0x21-0x24 pylône 2)
#endif

#ifndef IS_MASTER
#define IS_MASTER           1           // 1 = ancre maître pour sync
#endif

// === RS485 ===
#define RS485_BAUDRATE      115200
#define RS485_TX_PIN        17          // GPIO17 = UART2 TX
#define RS485_RX_PIN        16          // GPIO16 = UART2 RX
#define PIN_RS485_DE        4           // Direction Enable (HIGH = TX, LOW = RX)

// === Configuration UWB ===
#define UWB_CHANNEL         5           // Canal UWB (1-7, 5 recommandé)
#define UWB_PRF             2           // Pulse Rep Freq: 1=16MHz, 2=64MHz
#define UWB_PREAMBLE_LEN    128         // Longueur préambule
#define UWB_DATA_RATE       2           // 0=110kbps, 1=850kbps, 2=6.8Mbps

// === Pinout ESP32 → DW1000 ===
#define PIN_SPI_CLK         18
#define PIN_SPI_MISO        19
#define PIN_SPI_MOSI        23
#define PIN_SPI_CS          5
#define PIN_DW1000_IRQ      4
#define PIN_DW1000_RST      2

// === Synchronisation ===
#define SYNC_INTERVAL_MS    500         // Intervalle émission sync (si maître)
#define SYNC_TIMEOUT_MS     2000        // Timeout sync (si esclave)

// === Debug ===
#ifndef DEBUG_SERIAL
#define DEBUG_SERIAL        1           // 0=off, 1=normal, 2=verbose
#endif

// === Watchdog ===
#define WATCHDOG_TIMEOUT_MS 5000        // Reset si pas d'activité

#endif // CONFIG_H
