# Base Juges (Raspberry Pi)

Application Python pour la collecte, le traitement et l'affichage des données de détection.

## Matériel cible

- **Raspberry Pi** : Pi 4 Model B 4GB ou Pi 5
- **Interface RS485** : 2× USB-RS485 (FTDI ou CH340)
- **Affichage** : HDMI vers écran fourni par le site

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Raspberry Pi                              │
│                                                              │
│  ┌──────────────┐    Queue     ┌──────────────────────────┐ │
│  │  COLLECTOR   │─────────────>│      PROCESSOR           │ │
│  │  (Process 1) │  (multiproc) │      (Process 2)         │ │
│  └──────────────┘              └───────────┬──────────────┘ │
│                                            │                 │
│                                    ┌───────▼───────┐        │
│                                    │   WEBAPP      │        │
│                                    │  (Process 3)  │        │
│                                    └───────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

## Structure

```
base/pi/
├── README.md
├── requirements.txt        # Dépendances Python
├── pyproject.toml          # Configuration projet
├── config/
│   ├── default.json        # Configuration par défaut
│   └── geometry.json       # Géométrie pylônes/ancres
├── src/
│   ├── __init__.py
│   ├── main.py             # Point d'entrée
│   ├── collector/          # Collecte RS485
│   │   ├── __init__.py
│   │   ├── process.py      # Processus collector
│   │   ├── serial.py       # Lecture série
│   │   └── parser.py       # Parsing protocole
│   ├── processor/          # Traitement
│   │   ├── __init__.py
│   │   ├── process.py      # Processus processor
│   │   ├── sync.py         # Synchronisation temporelle
│   │   ├── geometry.py     # Calculs géométriques
│   │   ├── decision.py     # Logique anti-contestation
│   │   └── types.py        # Structures de données
│   ├── webapp/             # Interface web
│   │   ├── __init__.py
│   │   ├── app.py          # FastAPI application
│   │   ├── routes.py       # Routes API
│   │   ├── websocket.py    # WebSocket temps réel
│   │   └── static/         # Fichiers statiques
│   └── logger/             # Logging
│       ├── __init__.py
│       └── csv_logger.py   # Export CSV
├── tests/
│   ├── test_parser.py
│   ├── test_geometry.py
│   └── test_decision.py
└── scripts/
    ├── run.sh              # Lancer l'application
    └── simulate.py         # Simulateur pour tests
```

## Installation

```bash
cd base/pi

# Créer environnement virtuel
python3 -m venv venv
source venv/bin/activate

# Installer dépendances
pip install -r requirements.txt

# Configurer les ports série
sudo usermod -a -G dialout $USER
```

## Configuration

Éditer `config/default.json` :

```json
{
  "rs485_ports": ["/dev/ttyUSB0", "/dev/ttyUSB1"],
  "baudrate": 115200,
  "webapp_port": 8080,
  "log_directory": "logs/"
}
```

## Lancement

```bash
# Mode normal
python -m src.main

# Mode debug
python -m src.main --debug

# Mode simulation (sans matériel)
python -m src.main --simulate
```

## API Web

| Endpoint | Méthode | Description |
|----------|---------|-------------|
| `/` | GET | Dashboard principal |
| `/api/status` | GET | Status système |
| `/api/events` | GET | Derniers événements |
| `/api/config` | GET/POST | Configuration |
| `/ws` | WebSocket | Événements temps réel |

## TODO

- [ ] Collector : lecture RS485 multi-port
- [ ] Collector : parsing protocole binaire
- [ ] Processor : synchronisation temporelle
- [ ] Processor : calcul géométrique TDoA
- [ ] Processor : logique décision
- [ ] Webapp : dashboard temps réel
- [ ] Webapp : API configuration
- [ ] Logger : export CSV
- [ ] Tests unitaires
