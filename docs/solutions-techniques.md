# Solutions techniques - Pylon Racing Detection

Ce document répond aux questions techniques clés du projet et définit les choix d'implémentation.

---

## 1. Bibliothèque UWB pour ESP32 (DW1000/DW3000)

### DW1000 (recommandé pour POC)

**Bibliothèque** : [arduino-dw1000](https://github.com/thotro/arduino-dw1000) de Thomas Trojer

- Mature, bien documentée
- Supporte le mode "blink" natif
- Accès direct aux timestamps matériels
- Compatible Arduino et ESP-IDF

### DW3000 (production)

**Bibliothèque** : API C officielle Qorvo/Decawave

- Meilleure portée (100-150m vs 50-80m pour DW1000)
- Moins de wrappers Arduino disponibles
- Utiliser l'API C directement avec ESP-IDF

### Stratégie recommandée

1. **Phase 1-2** : DW1000 + arduino-dw1000 (simplicité)
2. **Phase 3+** : Migration vers DW3000 si la portée pose problème

---

## 2. Horodatage matériel précis

### Principe

Le timestamp doit être capturé au niveau du module UWB, pas du microcontrôleur. Le DW1000/DW3000 dispose d'un compteur 40-bit à ~15.65 ps/tick.

### Implémentation

```c
// Callback d'interruption RX
void onBlinkReceived(uint8_t* data, uint16_t len) {
    uint64_t rx_timestamp;

    // CRITIQUE : Lecture IMMÉDIATE du registre RX_TIME
    // Avant tout autre traitement
    dwt_readrxtimestamp((uint8_t*)&rx_timestamp);

    // Stocker pour traitement ultérieur
    blink_buffer[write_idx].timestamp = rx_timestamp;
    blink_buffer[write_idx].tag_id = extract_tag_id(data);
    blink_buffer[write_idx].seq_num = extract_seq_num(data);
    write_idx = (write_idx + 1) % BUFFER_SIZE;
}
```

### Points critiques

| Règle | Justification |
|-------|---------------|
| Utiliser l'interruption RX du DW1000 | Pas de polling, latence minimale |
| Lire le registre timestamp EN PREMIER | Avant tout traitement dans l'ISR |
| Pas de traitement lourd dans l'ISR | Juste stocker, traiter dans la boucle principale |
| Buffer circulaire | Évite les allocations dynamiques |

### Précision

- Précision temporelle UWB : ~15 ps (~4.5 mm en distance)
- Largement supérieure aux besoins du projet (zone morte ±1m)

---

## 3. Protocole RS485

### Format retenu : Binaire avec structure fixe

```
┌──────┬────────┬─────────┬────────┬───────────┬─────────┬───────┐
│ SYNC │ AncreID│ TagID   │ SeqNum │ Timestamp │ RSSI    │ CRC16 │
│ 0xAA │ 1 byte │ 2 bytes │ 2 bytes│ 5 bytes   │ 1 byte  │ 2 bytes│
└──────┴────────┴─────────┴────────┴───────────┴─────────┴───────┘
Total : 14 bytes par message
```

### Détail des champs

| Champ | Taille | Description |
|-------|--------|-------------|
| SYNC | 1 byte | Octet de synchronisation (0xAA) |
| AncreID | 1 byte | Identifiant de l'ancre émettrice (0-255) |
| TagID | 2 bytes | Identifiant du tag avion (little-endian) |
| SeqNum | 2 bytes | Compteur de séquence du tag |
| Timestamp | 5 bytes | Timestamp UWB 40-bit (little-endian) |
| RSSI | 1 byte | Indicateur de qualité signal (optionnel) |
| CRC16 | 2 bytes | CRC-16-CCITT |

### Justification du choix binaire

| Critère | Binaire | Texte (JSON/CSV) |
|---------|---------|------------------|
| Taille message | 14 bytes | ~80-100 bytes |
| Parsing | Simple, déterministe | Plus lent, allocations |
| Débit à 115200 baud | ~800 msg/s | ~120 msg/s |
| Debug terrain | Moins lisible | Plus lisible |

**Calcul de charge** : 4 avions × 40 Hz × 4 ancres = **640 messages/s** → binaire nécessaire

### Mode debug (optionnel)

Activable via commande RS485 pour le debug terrain :

```
DEBUG:A01:T0042:S1234:TS12345678AB:R-45:CRC_OK\n
```

---

## 4. Structure calcul géométrique

### Organisation des modules

```
raspberry/
├── processor/
│   ├── __init__.py
│   ├── geometry.py      # Calculs géométriques purs
│   ├── sync.py          # Synchronisation temporelle
│   ├── decision.py      # Logique métier anti-contestation
│   └── types.py         # Structures de données
```

### Types de données (types.py)

```python
from dataclasses import dataclass
import numpy as np

@dataclass(frozen=True)
class Plane:
    """Plan vertical défini par un point et une normale"""
    point: np.ndarray      # Point sur le plan (position pylône)
    normal: np.ndarray     # Vecteur normal (vers l'extérieur)

@dataclass(frozen=True)
class AnchorPosition:
    """Position d'une ancre"""
    anchor_id: int
    position: np.ndarray   # Coordonnées 3D [x, y, z]

@dataclass
class BlinkEvent:
    """Événement blink reçu d'une ancre"""
    anchor_id: int
    tag_id: int
    seq_num: int
    timestamp: int         # Timestamp UWB brut
    timestamp_corrected: float  # Timestamp après sync
    rssi: int

@dataclass
class PassageEvent:
    """Résultat de calcul pour un passage"""
    tag_id: int
    timestamp: float
    side: str              # "EXTERIOR" | "INTERIOR" | "DEAD_ZONE"
    signed_distance: float
    confidence: float
    raw_blinks: list[BlinkEvent]
```

### Calcul géométrique (geometry.py)

```python
import numpy as np
from .types import Plane, AnchorPosition, BlinkEvent

class GeometryCalculator:
    """Calculs de position et de côté par rapport au plan"""

    SPEED_OF_LIGHT = 299_792_458  # m/s
    DW1000_TICK = 1 / (128 * 499.2e6)  # ~15.65 ps

    def __init__(self, anchors: list[AnchorPosition], plane: Plane):
        self.anchors = {a.anchor_id: a.position for a in anchors}
        self.plane = plane

    def compute_position_tdoa(
        self,
        blinks: list[BlinkEvent]
    ) -> np.ndarray | None:
        """
        Calcul position approximative par TDoA (Time Difference of Arrival).

        Utilise une multilatération hyperbolique simplifiée.
        Retourne None si données insuffisantes (< 3 ancres).
        """
        if len(blinks) < 3:
            return None

        # Trier par timestamp pour avoir une référence
        blinks_sorted = sorted(blinks, key=lambda b: b.timestamp_corrected)
        ref = blinks_sorted[0]

        # Construire le système d'équations hyperboliques
        # d_i - d_ref = c * (t_i - t_ref)
        A = []
        b = []

        ref_pos = self.anchors[ref.anchor_id]

        for blink in blinks_sorted[1:]:
            anchor_pos = self.anchors[blink.anchor_id]
            delta_t = (blink.timestamp_corrected - ref.timestamp_corrected)
            delta_d = delta_t * self.SPEED_OF_LIGHT

            # Linéarisation (approximation pour position grossière)
            A.append(2 * (anchor_pos - ref_pos))
            b.append(
                np.dot(anchor_pos, anchor_pos) -
                np.dot(ref_pos, ref_pos) -
                delta_d**2
            )

        A = np.array(A)
        b = np.array(b)

        # Résolution par moindres carrés
        try:
            position, _, _, _ = np.linalg.lstsq(A, b, rcond=None)
            return position
        except np.linalg.LinAlgError:
            return None

    def compute_side(
        self,
        position: np.ndarray
    ) -> tuple[float, str]:
        """
        Détermine le côté par rapport au plan du pylône.

        Returns:
            (signed_distance, side) où side est "EXTERIOR" ou "INTERIOR"
        """
        relative = position - self.plane.point
        signed_distance = float(np.dot(relative, self.plane.normal))

        side = "EXTERIOR" if signed_distance > 0 else "INTERIOR"
        return signed_distance, side
```

### Logique anti-contestation (decision.py)

```python
from collections import defaultdict
from dataclasses import dataclass
from .types import PassageEvent

@dataclass
class Decision:
    """Décision finale pour un passage"""
    tag_id: int
    result: str           # "VALID" | "PENALTY" | "NO_CALL"
    reason: str
    confidence: float
    supporting_blinks: int
    timestamp: float

class AntiContestationLogic:
    """
    Logique de décision avec règle des 3 blinks.

    Règles :
    1. Zone morte (±dead_zone) → NO_CALL
    2. Pénalité seulement si ≥ min_consecutive blinks côté intérieur
    3. Sinon → passage validé
    """

    def __init__(
        self,
        dead_zone: float = 1.0,
        min_consecutive: int = 3,
        window_duration: float = 0.5
    ):
        self.dead_zone = dead_zone
        self.min_consecutive = min_consecutive
        self.window_duration = window_duration
        self.history: dict[int, list[PassageEvent]] = defaultdict(list)

    def process_event(self, event: PassageEvent) -> Decision | None:
        """
        Traite un événement de passage et retourne une décision si applicable.
        """
        tag_history = self.history[event.tag_id]

        # Nettoyer l'historique (garder seulement la fenêtre récente)
        self._prune_old_events(tag_history, event.timestamp)

        # Ajouter l'événement
        tag_history.append(event)

        # Dans la zone morte → pas de décision immédiate
        if abs(event.signed_distance) < self.dead_zone:
            return None

        # Vérifier la règle des blinks consécutifs
        return self._evaluate_passage(event.tag_id, tag_history)

    def _prune_old_events(
        self,
        history: list[PassageEvent],
        current_time: float
    ):
        """Supprime les événements trop anciens"""
        cutoff = current_time - self.window_duration
        while history and history[0].timestamp < cutoff:
            history.pop(0)

    def _evaluate_passage(
        self,
        tag_id: int,
        history: list[PassageEvent]
    ) -> Decision | None:
        """
        Évalue si un passage complet peut être décidé.
        """
        if len(history) < self.min_consecutive:
            return None

        # Compter les blinks consécutifs côté intérieur
        interior_streak = 0
        max_interior_streak = 0

        for event in history:
            if event.side == "INTERIOR" and abs(event.signed_distance) >= self.dead_zone:
                interior_streak += 1
                max_interior_streak = max(max_interior_streak, interior_streak)
            else:
                interior_streak = 0

        # Décision
        if max_interior_streak >= self.min_consecutive:
            return Decision(
                tag_id=tag_id,
                result="PENALTY",
                reason=f"{max_interior_streak} blinks consécutifs côté intérieur",
                confidence=min(1.0, max_interior_streak / 5.0),
                supporting_blinks=max_interior_streak,
                timestamp=history[-1].timestamp
            )

        # Passage détecté mais pas de pénalité
        if len(history) >= self.min_consecutive:
            return Decision(
                tag_id=tag_id,
                result="VALID",
                reason="Passage extérieur ou données insuffisantes pour pénalité",
                confidence=0.8,
                supporting_blinks=len(history),
                timestamp=history[-1].timestamp
            )

        return None
```

---

## 5. Architecture Raspberry Pi

### Choix : Architecture hybride (2-3 processus)

```
┌─────────────────────────────────────────────────────────────┐
│                    Raspberry Pi                              │
│                                                              │
│  ┌──────────────┐    Queue     ┌──────────────────────────┐ │
│  │  COLLECTOR   │─────────────>│      PROCESSOR           │ │
│  │  (Process 1) │  (multiproc) │      (Process 2)         │ │
│  │              │              │                          │ │
│  │ - RS485 read │              │ - Sync temporelle        │ │
│  │ - Parse trame│              │ - Calcul géométrique     │ │
│  │ - Validation │              │ - Décision               │ │
│  │   CRC        │              │ - Logs CSV/JSON          │ │
│  └──────────────┘              │ - WebSocket push         │ │
│                                └───────────┬──────────────┘ │
│                                            │                 │
│                                    ┌───────▼───────┐        │
│                                    │   WEBAPP      │        │
│                                    │  (Process 3)  │        │
│                                    │               │        │
│                                    │ - Dashboard   │        │
│                                    │ - Config API  │        │
│                                    │ - Logs viewer │        │
│                                    └───────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

### Justification

| Approche | Avantages | Inconvénients |
|----------|-----------|---------------|
| Monolithique | Simple, pas d'IPC | Difficile à débugger, GIL Python |
| Microservices | Isolation parfaite | Overkill, latence réseau |
| **Hybride** | Bon compromis | Légère complexité IPC |

### Implémentation

```python
# main.py - Point d'entrée unique

import multiprocessing as mp
import signal
import sys
from collector import CollectorProcess
from processor import ProcessorProcess
from webapp import WebAppProcess

def main():
    # Queues partagées
    blink_queue = mp.Queue(maxsize=1000)
    event_queue = mp.Queue(maxsize=100)

    # Configuration
    config = load_config("config/system.json")

    # Processus
    processes = [
        CollectorProcess(
            name="collector",
            rs485_ports=config["rs485_ports"],
            output_queue=blink_queue,
            baudrate=115200
        ),
        ProcessorProcess(
            name="processor",
            input_queue=blink_queue,
            event_queue=event_queue,
            geometry_config=config["geometry"]
        ),
        WebAppProcess(
            name="webapp",
            event_queue=event_queue,
            port=8080
        )
    ]

    # Gestion arrêt propre
    def shutdown(signum, frame):
        print("Arrêt en cours...")
        for p in processes:
            p.terminate()
        sys.exit(0)

    signal.signal(signal.SIGINT, shutdown)
    signal.signal(signal.SIGTERM, shutdown)

    # Démarrage
    for p in processes:
        p.start()
        print(f"Processus {p.name} démarré (PID {p.pid})")

    # Attente
    for p in processes:
        p.join()

if __name__ == "__main__":
    main()
```

### Collector (collector.py)

```python
import multiprocessing as mp
import serial
import struct
from crc import crc16_ccitt

class CollectorProcess(mp.Process):
    """Processus de collecte RS485 - priorité temps réel"""

    SYNC_BYTE = 0xAA
    MESSAGE_SIZE = 14

    def __init__(self, name, rs485_ports, output_queue, baudrate=115200):
        super().__init__(name=name)
        self.rs485_ports = rs485_ports
        self.output_queue = output_queue
        self.baudrate = baudrate

    def run(self):
        # Ouvrir les ports série
        ports = []
        for port_path in self.rs485_ports:
            ser = serial.Serial(
                port_path,
                self.baudrate,
                timeout=0.01  # 10ms timeout pour réactivité
            )
            ports.append(ser)

        buffer = bytearray()

        while True:
            for ser in ports:
                data = ser.read(64)
                if data:
                    buffer.extend(data)
                    self._process_buffer(buffer)

    def _process_buffer(self, buffer):
        """Parse les messages complets du buffer"""
        while len(buffer) >= self.MESSAGE_SIZE:
            # Chercher le sync byte
            try:
                sync_idx = buffer.index(self.SYNC_BYTE)
                if sync_idx > 0:
                    del buffer[:sync_idx]
            except ValueError:
                buffer.clear()
                return

            if len(buffer) < self.MESSAGE_SIZE:
                return

            # Extraire le message
            msg = bytes(buffer[:self.MESSAGE_SIZE])

            # Vérifier CRC
            if not self._verify_crc(msg):
                del buffer[0]  # Décaler d'un octet et réessayer
                continue

            # Parser et envoyer
            blink = self._parse_message(msg)
            if blink:
                try:
                    self.output_queue.put_nowait(blink)
                except:
                    pass  # Queue pleine, on drop

            del buffer[:self.MESSAGE_SIZE]

    def _verify_crc(self, msg):
        """Vérifie le CRC-16-CCITT"""
        data = msg[:-2]
        expected = struct.unpack('<H', msg[-2:])[0]
        return crc16_ccitt(data) == expected

    def _parse_message(self, msg):
        """Parse un message binaire"""
        _, anchor_id, tag_id, seq_num, ts_low, ts_high, rssi, _ = struct.unpack(
            '<BBHHIBBH', msg
        )
        timestamp = ts_low | (ts_high << 32)

        return {
            'anchor_id': anchor_id,
            'tag_id': tag_id,
            'seq_num': seq_num,
            'timestamp': timestamp,
            'rssi': rssi
        }
```

### Avantages de cette architecture

1. **Collector isolé** : Pas de GIL Python sur la lecture RS485 critique
2. **Processor découplé** : Peut être redémarré sans perdre la collecte
3. **Webapp séparée** : Pas d'impact sur le temps réel
4. **Latence < 100ms** : Queue en mémoire partagée, pas de réseau
5. **Robustesse** : Un crash d'un processus n'affecte pas les autres

---

## Résumé des choix techniques

| Domaine | Choix | Justification |
|---------|-------|---------------|
| Bibliothèque UWB | arduino-dw1000 (POC) / Qorvo API (prod) | Maturité, accès timestamps |
| Horodatage | Registre RX_TIME dans ISR | Précision matérielle |
| Protocole RS485 | Binaire 14 bytes | Débit suffisant pour 640 msg/s |
| Calcul géométrique | Modules séparés geometry/decision | Maintenabilité, testabilité |
| Architecture Pi | Hybride 2-3 processus | Compromis simplicité/performance |

---

## Références

- [DecaWave DW1000 Datasheet](https://www.decawave.com/product/dw1000/)
- [arduino-dw1000](https://github.com/thotro/arduino-dw1000)
- [RS485 Protocol Design](https://www.ni.com/en/support/documentation/supplemental/06/rs-485-protocol.html)
