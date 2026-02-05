# Protocole de communication

Ce dossier contient les spécifications du protocole de communication entre les composants du système.

## Vue d'ensemble

```
┌─────────┐         ┌─────────┐         ┌─────────┐
│  Tags   │  UWB    │ Ancres  │  RS485  │  Base   │
│ (avions)│────────►│(pylône) │────────►│  (Pi)   │
└─────────┘  blink  └─────────┘  binaire└─────────┘
```

## Fichiers

| Fichier | Description |
|---------|-------------|
| `blink.md` | Format des trames UWB (tag → ancre) |
| `rs485.md` | Format des messages RS485 (ancre → base) |
| `crc.md` | Algorithme CRC-16-CCITT |
| `version.md` | Historique des versions du protocole |

## Version actuelle

**Version 1.0** - Janvier 2024

## Principes de conception

1. **Binaire** : Format binaire pour efficacité et débit
2. **Little-endian** : Cohérent avec ESP32 et x86
3. **CRC-16** : Détection d'erreurs robuste
4. **Sync byte** : Resynchronisation facile après erreur
5. **Fixe** : Taille de message fixe pour parsing simple
