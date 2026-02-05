# Historique des versions du protocole

## Version 1.0 (Janvier 2024)

Version initiale du protocole.

### Messages

| Type | Sync | Taille | Description |
|------|------|--------|-------------|
| Blink | 0xAA | 14 bytes | Notification de réception blink |
| Sync | 0x55 | 13 bytes | Synchronisation temporelle |
| Status | 0xBB | 9 bytes | Status de l'ancre |
| Command | 0xCC | 4-12 bytes | Commande de configuration |

### Paramètres

- Baudrate RS485 : 115200
- CRC : CRC-16-CCITT (0x8408)
- Endianness : Little-endian

### Compatibilité

- Firmware tag : v1.0+
- Firmware anchor : v1.0+
- Base Pi : v1.0+

---

## Évolutions prévues

### Version 1.1 (planifiée)

Ajouts envisagés :

- [ ] Message de diagnostic étendu
- [ ] Support multi-pylône dans un seul flux RS485
- [ ] Compression optionnelle des timestamps

### Version 2.0 (future)

Changements majeurs possibles :

- [ ] Protocole binaire optimisé (Protocol Buffers ou MessagePack)
- [ ] Support Ethernet en plus de RS485
- [ ] Chiffrement optionnel

---

## Notes de migration

### De 0.x à 1.0

Pas de version 0.x publiée. Version 1.0 est la première release.

### Compatibilité ascendante

Le protocole est conçu pour être extensible :

1. **Sync bytes uniques** : Chaque type de message a son propre sync byte
2. **CRC obligatoire** : Permet de rejeter les messages corrompus ou inconnus
3. **Champs réservés** : Espace pour extension future

### Détection de version

La base peut détecter la version du firmware par :
1. Message STATUS qui inclut un champ version (futur)
2. Réponse à la commande PING avec version
