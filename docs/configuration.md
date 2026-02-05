# Configuration système

Ce document décrit les paramètres configurables du système de détection.

---

## 1. Plan du pylône

### Définition géométrique

Le plan de détection est un **plan vertical** passant par le pylône, orienté perpendiculairement à la trajectoire nominale des avions.

```
        Vue de dessus

    INTÉRIEUR (piste)
           │
           │  ← Plan vertical
           │
    ───────┼─────── Trajectoire avion
           │
           │
           ●  Pylône
           │
    EXTÉRIEUR
```

### Paramètres

| Paramètre | Type | Valeur par défaut | Description |
|-----------|------|-------------------|-------------|
| `plane.point` | [x, y, z] | [0, 0, 0] | Point de référence (base du pylône) |
| `plane.normal` | [nx, ny, nz] | [1, 0, 0] | Vecteur normal (vers l'extérieur) |

### Fichier de configuration

```json
{
  "pylon_1": {
    "plane": {
      "point": [0.0, 0.0, 0.0],
      "normal": [1.0, 0.0, 0.0]
    },
    "anchors": [
      {"id": 1, "position": [-0.5, 0.0, 0.3]},
      {"id": 2, "position": [0.5, -0.5, 0.3]},
      {"id": 3, "position": [0.5, 0.5, 0.3]},
      {"id": 4, "position": [0.0, 0.0, 2.0]}
    ]
  }
}
```

---

## 2. Zone morte

### Principe

La zone morte est une bande autour du plan où aucune décision n'est prise. Cela évite les faux positifs pour les passages très proches du pylône.

```
    │←──── dead_zone ────→│
    │                     │
────┼──────────┼──────────┼────
    │    ZONE MORTE       │
    │   (pas de décision) │
              │
           Plan pylône
```

### Paramètres

| Paramètre | Type | Valeur par défaut | Plage recommandée |
|-----------|------|-------------------|-------------------|
| `dead_zone` | float (m) | 1.0 | 0.5 - 2.0 |

### Comportement

| Distance signée | Décision |
|-----------------|----------|
| `d > dead_zone` | EXTÉRIEUR (valide) |
| `-dead_zone ≤ d ≤ dead_zone` | ZONE MORTE (no call) |
| `d < -dead_zone` | INTÉRIEUR (potentielle pénalité) |

---

## 3. Identifiants

### Tags (avions)

| Paramètre | Format | Plage | Description |
|-----------|--------|-------|-------------|
| `tag_id` | uint16 | 0x0001 - 0xFFFE | Identifiant unique par avion |

**Convention suggérée** :
- 0x0001 - 0x000F : Tags de test/calibration
- 0x0010 - 0x00FF : Tags compétition
- 0xFF00 - 0xFFFE : Tags de référence (sync)

### Ancres

| Paramètre | Format | Plage | Description |
|-----------|--------|-------|-------------|
| `anchor_id` | uint8 | 0x01 - 0xFE | Identifiant unique par ancre |

**Convention suggérée** :
- Pylône 1 : ancres 0x11, 0x12, 0x13, 0x14
- Pylône 2 : ancres 0x21, 0x22, 0x23, 0x24

### Session

| Paramètre | Format | Description |
|-----------|--------|-------------|
| `session_id` | uint8 | Identifiant de course (évite pollution) |

---

## 4. Fréquences

### Fréquence d'émission des tags

| Paramètre | Valeur par défaut | Plage | Justification |
|-----------|-------------------|-------|---------------|
| `blink_rate_hz` | 30 | 20 - 40 | Compromis couverture / collisions |

**Calcul de couverture** à 200 km/h (55 m/s) :
- 20 Hz → 2.75 m entre blinks
- 30 Hz → 1.83 m entre blinks
- 40 Hz → 1.38 m entre blinks

Sur une zone de contrôle ±5 m → **4 à 8 blinks exploitables**

### Fréquence de synchronisation

Voir [sync.md](sync.md) pour les détails.

| Paramètre | Valeur par défaut |
|-----------|-------------------|
| `sync_interval_ms` | 500 |

---

## 5. Règles anti-collision (multi-tags)

### Problème

Avec 4 avions à 30 Hz, il y a risque de collision des trames UWB.

### Solutions implémentées

#### 5.1 Slotting par ID

Chaque tag démarre avec un décalage temporel basé sur son ID :

```
slot_offset_ms = (tag_id % 4) * (1000 / blink_rate_hz / 4)
```

**Exemple** à 40 Hz (période 25 ms) :
- Tag 0x10 : offset 0 ms
- Tag 0x11 : offset 6.25 ms
- Tag 0x12 : offset 12.5 ms
- Tag 0x13 : offset 18.75 ms

#### 5.2 Jitter pseudo-aléatoire

Ajout d'un jitter sur chaque émission :

| Paramètre | Valeur par défaut | Description |
|-----------|-------------------|-------------|
| `jitter_max_ms` | 2.0 | Jitter max (±) |
| `jitter_seed` | tag_id | Graine PRNG |

```c
// Calcul du jitter
int16_t jitter_us = (prng_next() % (2 * JITTER_MAX_US)) - JITTER_MAX_US;
delay_us(slot_offset_us + jitter_us);
```

#### 5.3 Tolérance aux pertes

La logique de décision tolère la perte de blinks grâce à :
- Règle des **3 blinks consécutifs** minimum
- Fenêtre temporelle de **500 ms**

---

## 6. Règles de décision

### Paramètres

| Paramètre | Valeur par défaut | Description |
|-----------|-------------------|-------------|
| `min_consecutive_blinks` | 3 | Blinks consécutifs pour pénalité |
| `decision_window_ms` | 500 | Fenêtre d'analyse |
| `min_confidence` | 0.7 | Seuil de confiance |

### Logique

```
SI (≥3 blinks consécutifs avec distance < -dead_zone)
   ET (confiance > min_confidence)
ALORS
   PENALTY
SINON SI (passage détecté avec blinks côté extérieur)
ALORS
   VALID
SINON
   NO_CALL
```

---

## 7. Fichier de configuration complet

```json
{
  "version": "1.0",
  "session_id": 1,

  "pylons": {
    "pylon_1": {
      "plane": {
        "point": [0.0, 0.0, 0.0],
        "normal": [1.0, 0.0, 0.0]
      },
      "anchors": [
        {"id": 17, "position": [-0.5, 0.0, 0.3]},
        {"id": 18, "position": [0.5, -0.5, 0.3]},
        {"id": 19, "position": [0.5, 0.5, 0.3]},
        {"id": 20, "position": [0.0, 0.0, 2.0]}
      ]
    },
    "pylon_2": {
      "plane": {
        "point": [100.0, 0.0, 0.0],
        "normal": [-1.0, 0.0, 0.0]
      },
      "anchors": [
        {"id": 33, "position": [100.5, 0.0, 0.3]},
        {"id": 34, "position": [99.5, -0.5, 0.3]},
        {"id": 35, "position": [99.5, 0.5, 0.3]},
        {"id": 36, "position": [100.0, 0.0, 2.0]}
      ]
    }
  },

  "detection": {
    "dead_zone_m": 1.0,
    "min_consecutive_blinks": 3,
    "decision_window_ms": 500,
    "min_confidence": 0.7
  },

  "tags": {
    "blink_rate_hz": 30,
    "jitter_max_ms": 2.0
  },

  "sync": {
    "interval_ms": 500,
    "master_anchor_id": 17
  }
}
```

---

## 8. Procédure de calibration

1. **Positionner les ancres** selon la géométrie prévue
2. **Mesurer les positions** relatives avec un mètre/télémètre
3. **Saisir dans le fichier** de configuration
4. **Test de validation** : passer un tag de référence côté extérieur clair
5. **Vérifier** que le système détecte bien "EXTÉRIEUR"
6. **Ajuster** le vecteur normal si nécessaire (inversion de signe)
