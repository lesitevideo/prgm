# Firmware Tag (Avion)

Firmware embarqué dans les tags UWB montés sur les avions RC.

## Matériel cible

- **MCU** : ESP32-C3 Mini ou ESP32-PICO-D4
- **UWB** : DWM1000 ou DWM3000
- **Alimentation** : LiPo 1S 150-250mAh

## Fonctionnalités

- Émission périodique de blinks UWB (20-40 Hz)
- Gestion de l'alimentation (deep sleep entre blinks)
- Anti-collision par slotting + jitter
- LED status

## Structure prévue

```
firmware/tag/
├── README.md
├── platformio.ini          # Configuration PlatformIO
├── src/
│   ├── main.cpp            # Point d'entrée
│   ├── uwb.cpp             # Interface DW1000/DW3000
│   ├── uwb.h
│   ├── blink.cpp           # Logique d'émission blink
│   ├── blink.h
│   ├── config.h            # Configuration (ID, fréquence)
│   └── power.cpp           # Gestion alimentation
├── lib/
│   └── DW1000/             # Bibliothèque UWB (submodule)
└── test/
    └── test_blink.cpp
```

## Format de la trame blink

```
┌──────────┬────────┬────────┬───────┐
│ Frame    │ TagID  │ SeqNum │ CRC16 │
│ Control  │ 2bytes │ 2bytes │ 2bytes│
│ 2 bytes  │        │        │       │
└──────────┴────────┴────────┴───────┘
Total : 8 bytes (hors préambule UWB)
```

## Configuration

Définie dans `config.h` :

```cpp
#define TAG_ID          0x0010      // Identifiant unique
#define BLINK_RATE_HZ   30          // Fréquence d'émission
#define JITTER_MAX_MS   2           // Jitter anti-collision
#define UWB_CHANNEL     5           // Canal UWB
#define UWB_PRF         DWT_PRF_64M // Pulse Repetition Frequency
```

## Compilation

```bash
cd firmware/tag
pio run                    # Compiler
pio run -t upload          # Flasher
pio device monitor         # Moniteur série
```

## TODO

- [ ] Initialisation DW1000
- [ ] Émission blink basique
- [ ] Slotting par ID
- [ ] Jitter pseudo-aléatoire
- [ ] Deep sleep entre blinks
- [ ] Indicateur batterie faible
