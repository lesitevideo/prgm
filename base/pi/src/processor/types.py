"""
Types de données pour le traitement.
"""

from dataclasses import dataclass, field
from typing import Optional
import numpy as np


@dataclass(frozen=True)
class Plane:
    """Plan vertical défini par un point et une normale."""
    point: np.ndarray      # Point sur le plan (position pylône)
    normal: np.ndarray     # Vecteur normal (vers l'extérieur)

    def __post_init__(self):
        # Normaliser le vecteur normal
        norm = np.linalg.norm(self.normal)
        if norm > 0:
            object.__setattr__(self, 'normal', self.normal / norm)


@dataclass(frozen=True)
class AnchorPosition:
    """Position d'une ancre."""
    anchor_id: int
    position: np.ndarray   # Coordonnées 3D [x, y, z]


@dataclass
class BlinkEvent:
    """Événement blink reçu d'une ancre."""
    anchor_id: int
    tag_id: int
    seq_num: int
    timestamp_raw: int         # Timestamp UWB brut (40-bit)
    timestamp_corrected: float = 0.0  # Timestamp après sync (secondes)
    rssi: int = 0
    received_at: float = 0.0   # Timestamp local de réception


@dataclass
class PassageEvent:
    """Résultat de calcul pour un passage."""
    tag_id: int
    timestamp: float
    side: str              # "EXTERIOR" | "INTERIOR" | "DEAD_ZONE"
    signed_distance: float
    confidence: float
    raw_blinks: list = field(default_factory=list)


@dataclass
class Decision:
    """Décision finale pour un passage."""
    tag_id: int
    result: str            # "VALID" | "PENALTY" | "NO_CALL"
    reason: str
    confidence: float
    supporting_blinks: int
    timestamp: float
    pylon_id: int = 1


@dataclass
class SyncStatus:
    """Status de synchronisation d'une ancre."""
    anchor_id: int
    status: str            # "OK" | "DEGRADED" | "LOST"
    drift_ppm: float = 0.0
    last_sync_age_ms: int = 0
    a: float = 1.0         # Facteur d'échelle
    b: float = 0.0         # Offset


@dataclass
class SystemStatus:
    """Status global du système."""
    anchors: dict          # anchor_id -> SyncStatus
    tags_seen: set         # tag_ids vus récemment
    total_blinks: int = 0
    total_decisions: int = 0
    uptime_seconds: float = 0.0
