# Firmware Ancre (Pylône)

Firmware des ancres UWB installées autour des pylônes.

## Matériel cible

- **MCU** : ESP32 DevKit C ou WROOM-32
- **UWB** : DWM1000 ou DWM3000
- **RS485** : MAX485 ou MAX3485
- **Alimentation** : 5V via DC-DC (depuis pseudo-PoE 24V)

## Fonctionnalités

- Réception des blinks UWB
- Horodatage matériel précis (registre RX_TIME)
- Transmission vers base via RS485
- Synchronisation maître/esclave
- Watchdog et diagnostics

## Structure prévue

```
firmware/anchor/
├── README.md
├── platformio.ini          # Configuration PlatformIO
├── src/
│   ├── main.cpp            # Point d'entrée
│   ├── uwb.cpp             # Interface DW1000/DW3000
│   ├── uwb.h
│   ├── rs485.cpp           # Communication RS485
│   ├── rs485.h
│   ├── sync.cpp            # Synchronisation temporelle
│   ├── sync.h
│   ├── config.h            # Configuration
│   └── protocol.h          # Format des trames
├── lib/
│   └── DW1000/
└── test/
    └── test_rs485.cpp
```

## Flux de données

```
         ┌─────────────┐
         │ Tag (blink) │
         └──────┬──────┘
                │ UWB
                ▼
         ┌─────────────┐
         │   DW1000    │
         │ (RX + IRQ)  │
         └──────┬──────┘
                │ SPI
                ▼
         ┌─────────────┐
         │   ESP32     │
         │ (timestamp) │
         └──────┬──────┘
                │ UART
                ▼
         ┌─────────────┐
         │   MAX485    │
         └──────┬──────┘
                │ RS485
                ▼
         [ Vers Base ]
```

## Format trame RS485 (sortie)

```
┌──────┬────────┬─────────┬────────┬───────────┬─────────┬───────┐
│ SYNC │ AncreID│ TagID   │ SeqNum │ Timestamp │ RSSI    │ CRC16 │
│ 0xAA │ 1 byte │ 2 bytes │ 2 bytes│ 5 bytes   │ 1 byte  │ 2 bytes│
└──────┴────────┴─────────┴────────┴───────────┴─────────┴───────┘
Total : 14 bytes
```

## Configuration

Définie dans `config.h` :

```cpp
#define ANCHOR_ID       0x11        // Identifiant unique (0x11-0x14 pour pylône 1)
#define IS_MASTER       true        // true si ancre maître
#define RS485_BAUDRATE  115200      // Vitesse RS485
#define SYNC_INTERVAL   500         // Intervalle sync (ms), si maître
```

## Compilation

```bash
cd firmware/anchor
pio run                    # Compiler
pio run -t upload          # Flasher
pio device monitor         # Moniteur série
```

## TODO

- [ ] Réception UWB et IRQ
- [ ] Lecture timestamp matériel
- [ ] Envoi RS485
- [ ] Mode maître (émission sync)
- [ ] Mode esclave (réception sync)
- [ ] Correction temporelle a*t+b
- [ ] Watchdog
- [ ] Diagnostic RSSI
