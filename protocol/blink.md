# Protocole Blink UWB

Format des trames UWB émises par les tags embarqués sur les avions.

## Format de trame

```
┌────────────┬──────────┬──────────┬────────┐
│ Frame Ctrl │ Tag ID   │ Seq Num  │ CRC16  │
│ 2 bytes    │ 2 bytes  │ 2 bytes  │ 2 bytes│
└────────────┴──────────┴──────────┴────────┘
Total payload : 8 bytes
```

## Détail des champs

### Frame Control (2 bytes)

Conforme IEEE 802.15.4 UWB :

| Bits | Valeur | Description |
|------|--------|-------------|
| 0-2 | 0b001 | Frame Type = Data |
| 3 | 0 | Security Disabled |
| 4 | 0 | Frame Pending = No |
| 5 | 0 | AR = No ACK Request |
| 6 | 0 | PAN ID Compression |
| 7-9 | 0b000 | Reserved |
| 10-11 | 0b10 | Dest Addr Mode = Short |
| 12-13 | 0b00 | Frame Version |
| 14-15 | 0b10 | Src Addr Mode = Short |

**Valeur fixe** : `0x8841` (little-endian)

### Tag ID (2 bytes, little-endian)

Identifiant unique du tag.

| Plage | Usage |
|-------|-------|
| 0x0001 - 0x000F | Tags de test/calibration |
| 0x0010 - 0x00FF | Tags compétition |
| 0xFF00 - 0xFFFE | Tags de référence (sync) |
| 0x0000, 0xFFFF | Réservés |

### Sequence Number (2 bytes, little-endian)

Compteur incrémenté à chaque blink. Permet de :
- Détecter les pertes de trames
- Associer les timestamps de différentes ancres
- Identifier les doublons

Rollover à 65535 → 0.

### CRC16 (2 bytes, little-endian)

CRC-16-CCITT sur les 6 premiers bytes.

Voir [crc.md](crc.md) pour l'algorithme.

## Configuration UWB

### Paramètres DW1000/DW3000

| Paramètre | Valeur | Justification |
|-----------|--------|---------------|
| Channel | 5 | Bonne portée, peu d'interférences |
| PRF | 64 MHz | Meilleure précision |
| Preamble | 128 symbols | Compromis portée/débit |
| Data Rate | 6.8 Mbps | Trame courte, latence min |
| PAC Size | 8 | Standard |
| SFD | Standard | Compatibilité |

### Code de configuration

```c
static dwt_config_t uwb_config = {
    .chan = 5,
    .txPreambLength = DWT_PLEN_128,
    .rxPAC = DWT_PAC8,
    .txCode = 9,
    .rxCode = 9,
    .nsSFD = 0,
    .dataRate = DWT_BR_6M8,
    .phrMode = DWT_PHRMODE_STD,
    .sfdTO = (129 + 8 - 8)
};
```

## Timing

### Fréquence d'émission

| Paramètre | Valeur |
|-----------|--------|
| Fréquence nominale | 30 Hz |
| Période | 33.3 ms |
| Jitter max | ±2 ms |

### Durée de la trame

| Élément | Durée |
|---------|-------|
| Preamble (128 sym) | ~1 ms |
| SFD | ~0.1 ms |
| PHR | ~0.02 ms |
| Payload (8 bytes) | ~0.01 ms |
| **Total** | **~1.2 ms** |

## Exemple de trame

```
Tag ID: 0x0010, Seq: 0x04D2 (1234)

Bytes (hex): 41 88 10 00 D2 04 XX XX
             ^^^^^ ^^^^^ ^^^^^ ^^^^^
             Frame TagID SeqNu CRC16
             Ctrl
```

## Réception par les ancres

Chaque ancre qui reçoit le blink :
1. Capture le timestamp matériel (RX_TIME)
2. Extrait Tag ID et Seq Num
3. Vérifie le CRC
4. Envoie vers la base via RS485

Voir [rs485.md](rs485.md) pour le format de transmission.
