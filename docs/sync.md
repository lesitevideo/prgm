# Synchronisation temporelle

Ce document décrit les mécanismes de synchronisation entre les ancres UWB.

---

## 1. Problématique

### Pourquoi synchroniser ?

Dans une architecture TDoA (Time Difference of Arrival), les timestamps des différentes ancres doivent être comparables. Or chaque ancre a sa propre horloge, sujette à :

- **Offset initial** : décalage constant entre horloges
- **Dérive (drift)** : écart qui augmente avec le temps (~1-20 ppm typique)

### Précision requise

| Paramètre | Valeur | Justification |
|-----------|--------|---------------|
| Précision cible | < 10 ns | ~3 m en distance (largement suffisant) |
| Durée de validité | 500 ms | Temps de passage au pylône |
| Dérive tolérée | < 20 ppm | Standard pour oscillateurs |

**Calcul** : À 20 ppm, en 500 ms, la dérive max = 10 µs = 3 km (négligeable pour notre usage).

---

## 2. Architecture maître/esclaves

### Principe

```
                    ┌─────────────┐
                    │   MAÎTRE    │
                    │  (Ancre 1)  │
                    └──────┬──────┘
                           │ Sync pulse / trame
              ┌────────────┼────────────┐
              │            │            │
        ┌─────▼─────┐ ┌────▼────┐ ┌─────▼─────┐
        │ ESCLAVE 1 │ │ESCLAVE 2│ │ ESCLAVE 3 │
        │ (Ancre 2) │ │(Ancre 3)│ │ (Ancre 4) │
        └───────────┘ └─────────┘ └───────────┘
```

### Rôles

| Rôle | Fonction |
|------|----------|
| **Maître** | Émet périodiquement un événement de synchronisation |
| **Esclave** | Mesure l'écart et corrige son horloge locale |

### Configuration

```json
{
  "sync": {
    "mode": "master_slave",
    "master_anchor_id": 17,
    "sync_interval_ms": 500,
    "sync_channel": "uwb"
  }
}
```

---

## 3. Mécanismes de synchronisation

### Option A : Synchronisation par trame UWB dédiée

Le maître émet une trame UWB spéciale que les esclaves reçoivent.

**Avantages** :
- Pas de câblage supplémentaire
- Précision native UWB (~15 ps)

**Inconvénients** :
- Consomme de la bande passante UWB
- Peut interférer avec les blinks des tags

**Format de la trame sync** :

```
┌──────┬──────────┬───────────┬───────┐
│ TYPE │ MasterID │ SyncCount │ CRC16 │
│ 0x55 │ 1 byte   │ 4 bytes   │ 2 bytes│
└──────┴──────────┴───────────┴───────┘
```

### Option B : Synchronisation par ligne dédiée (RS485)

Le maître envoie une commande sur le bus RS485.

**Avantages** :
- Pas d'interférence UWB
- Simple à implémenter

**Inconvénients** :
- Latence RS485 (~1 ms) à compenser
- Précision limitée par le bus

**Commande RS485** :

```
SYNC:<master_id>:<sync_count>:<master_timestamp>\n
```

### Option C : Tag de référence fixe (recommandé pour POC)

Un tag UWB fixe, placé à position connue, émet des blinks réguliers. Toutes les ancres le voient et s'alignent sur lui.

**Avantages** :
- Pas de modification firmware ancres
- Auto-calibration géométrique possible

**Inconvénients** :
- Nécessite un tag supplémentaire
- Position doit rester stable

```
        Tag référence (fixe)
               ●
              /|\
             / | \
            /  |  \
           /   |   \
          ▼    ▼    ▼
        A1    A2    A3    (ancres)
```

---

## 4. Modèle de correction temporelle

### Équation

```
t_corrigé = a × t_local + b
```

| Paramètre | Description | Unité |
|-----------|-------------|-------|
| `a` | Facteur d'échelle (corrige la dérive) | sans unité (~1.0) |
| `b` | Offset (corrige le décalage initial) | secondes |

### Calcul des paramètres

À chaque événement de synchronisation, l'esclave mesure :
- `t_local` : timestamp local de réception
- `t_master` : timestamp du maître (inclus dans la trame)

Avec deux mesures successives :

```
Mesure 1: t_local_1, t_master_1
Mesure 2: t_local_2, t_master_2

a = (t_master_2 - t_master_1) / (t_local_2 - t_local_1)
b = t_master_1 - a × t_local_1
```

### Implémentation

```c
// Structure de synchronisation (côté esclave)
typedef struct {
    double a;           // Facteur d'échelle
    double b;           // Offset
    uint64_t last_local;
    uint64_t last_master;
    bool initialized;
} sync_state_t;

sync_state_t sync_state = {1.0, 0.0, 0, 0, false};

void on_sync_received(uint64_t local_ts, uint64_t master_ts) {
    if (!sync_state.initialized) {
        // Première sync : juste stocker
        sync_state.last_local = local_ts;
        sync_state.last_master = master_ts;
        sync_state.initialized = true;
        return;
    }

    // Calculer a et b
    double delta_local = (double)(local_ts - sync_state.last_local);
    double delta_master = (double)(master_ts - sync_state.last_master);

    sync_state.a = delta_master / delta_local;
    sync_state.b = (double)master_ts - sync_state.a * (double)local_ts;

    // Mettre à jour pour prochaine sync
    sync_state.last_local = local_ts;
    sync_state.last_master = master_ts;
}

uint64_t correct_timestamp(uint64_t local_ts) {
    return (uint64_t)(sync_state.a * (double)local_ts + sync_state.b);
}
```

---

## 5. Fréquence de resynchronisation

### Paramètres

| Paramètre | Valeur recommandée | Justification |
|-----------|-------------------|---------------|
| `sync_interval_ms` | 500 | 2 sync/seconde, suffisant pour drift 20 ppm |
| `sync_timeout_ms` | 2000 | Alerte si pas de sync reçue |
| `max_drift_ppm` | 50 | Seuil d'alerte |

### Calcul de la fréquence optimale

Pour une dérive de 20 ppm et une tolérance de 10 µs :

```
Intervalle max = tolérance / dérive
               = 10 µs / (20 × 10⁻⁶)
               = 500 ms
```

### États de synchronisation

```
┌────────────┐  sync reçue   ┌────────────┐
│   INIT     │──────────────>│   SYNCED   │
└────────────┘               └─────┬──────┘
                                   │
                        timeout    │ sync reçue
                        ┌──────────┴──────────┐
                        │                     │
                        ▼                     │
                 ┌────────────┐               │
                 │  DEGRADED  │───────────────┘
                 └────────────┘
```

---

## 6. Protocole de synchronisation complet

### Séquence au démarrage

```
1. Maître démarre, commence à émettre sync
2. Esclaves démarrent en état INIT
3. Première sync reçue → stockage t_local_1, t_master_1
4. Deuxième sync reçue → calcul a, b → état SYNCED
5. Boucle : mise à jour a, b à chaque sync
```

### Messages RS485 (mode master_slave)

**Maître → Esclaves** :

```
Commande:  SYNC
Format:    S:<master_id>:<sync_count>:<timestamp_40bit_hex>\r\n
Exemple:   S:11:00042:1A2B3C4D5E\r\n
```

**Esclaves → Base (status)** :

```
Format:    Y:<anchor_id>:<sync_status>:<drift_ppm>:<last_sync_age_ms>\r\n
Exemple:   Y:12:OK:+12:150\r\n
           Y:13:DEGRADED:-45:2500\r\n
```

---

## 7. Gestion des erreurs

### Perte de synchronisation

| Condition | Action |
|-----------|--------|
| Pas de sync depuis 2s | Passer en DEGRADED |
| Pas de sync depuis 10s | Alerter l'opérateur |
| Drift > 50 ppm | Alerter (problème oscillateur) |

### Indicateurs côté Raspberry Pi

```python
class SyncMonitor:
    def check_anchor_sync(self, anchor_id: int) -> SyncStatus:
        status = self.anchor_status[anchor_id]

        if status.last_sync_age_ms > 10000:
            return SyncStatus.LOST
        elif status.last_sync_age_ms > 2000:
            return SyncStatus.DEGRADED
        elif abs(status.drift_ppm) > 50:
            return SyncStatus.DRIFT_WARNING
        else:
            return SyncStatus.OK
```

---

## 8. Configuration recommandée

### Pour le POC (Phase 1-2)

```json
{
  "sync": {
    "mode": "reference_tag",
    "reference_tag_id": 65535,
    "reference_position": [0.0, 0.0, 1.0],
    "expected_blink_rate_hz": 10
  }
}
```

### Pour la production (Phase 3+)

```json
{
  "sync": {
    "mode": "master_slave",
    "master_anchor_id": 17,
    "sync_interval_ms": 500,
    "sync_channel": "rs485",
    "timeout_ms": 2000,
    "max_drift_ppm": 50
  }
}
```
