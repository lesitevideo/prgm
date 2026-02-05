"""
Point d'entrée principal de l'application Base Pi.

Architecture multi-processus :
- Collector : lecture RS485, parsing, mise en queue
- Processor : sync, géométrie, décision, logging
- Webapp : dashboard, API, WebSocket
"""

import argparse
import json
import logging
import multiprocessing as mp
import signal
import sys
from pathlib import Path

# from .collector.process import CollectorProcess
# from .processor.process import ProcessorProcess
# from .webapp.app import run_webapp

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)
logger = logging.getLogger(__name__)


def load_config(config_path: str) -> dict:
    """Charge la configuration depuis un fichier JSON."""
    path = Path(config_path)
    if not path.exists():
        logger.warning(f"Config file {config_path} not found, using defaults")
        return get_default_config()

    with open(path) as f:
        return json.load(f)


def get_default_config() -> dict:
    """Retourne la configuration par défaut."""
    return {
        "rs485_ports": ["/dev/ttyUSB0", "/dev/ttyUSB1"],
        "baudrate": 115200,
        "webapp_port": 8080,
        "log_directory": "logs/",
        "detection": {
            "dead_zone_m": 1.0,
            "min_consecutive_blinks": 3,
            "decision_window_ms": 500
        },
        "sync": {
            "mode": "master_slave",
            "master_anchor_id": 17,
            "sync_interval_ms": 500
        }
    }


def main():
    """Point d'entrée principal."""
    parser = argparse.ArgumentParser(
        description="Pylon Racing Detection System - Base Pi"
    )
    parser.add_argument(
        "--config", "-c",
        default="config/default.json",
        help="Path to configuration file"
    )
    parser.add_argument(
        "--debug", "-d",
        action="store_true",
        help="Enable debug logging"
    )
    parser.add_argument(
        "--simulate", "-s",
        action="store_true",
        help="Run in simulation mode (no hardware)"
    )
    args = parser.parse_args()

    if args.debug:
        logging.getLogger().setLevel(logging.DEBUG)

    logger.info("=== Pylon Racing Detection System ===")
    logger.info(f"Config: {args.config}")
    logger.info(f"Debug: {args.debug}")
    logger.info(f"Simulate: {args.simulate}")

    # Charger configuration
    config = load_config(args.config)
    logger.info(f"RS485 ports: {config['rs485_ports']}")

    # Créer les queues de communication
    blink_queue = mp.Queue(maxsize=1000)
    event_queue = mp.Queue(maxsize=100)

    # Liste des processus
    processes = []

    # TODO: Décommenter quand les modules seront implémentés
    #
    # # Collector
    # collector = CollectorProcess(
    #     name="collector",
    #     rs485_ports=config["rs485_ports"] if not args.simulate else [],
    #     output_queue=blink_queue,
    #     baudrate=config["baudrate"],
    #     simulate=args.simulate
    # )
    # processes.append(collector)
    #
    # # Processor
    # processor = ProcessorProcess(
    #     name="processor",
    #     input_queue=blink_queue,
    #     event_queue=event_queue,
    #     config=config
    # )
    # processes.append(processor)

    # Gestion de l'arrêt propre
    def shutdown(signum, frame):
        logger.info("Arrêt demandé...")
        for p in processes:
            if p.is_alive():
                p.terminate()
                p.join(timeout=2)
        sys.exit(0)

    signal.signal(signal.SIGINT, shutdown)
    signal.signal(signal.SIGTERM, shutdown)

    # Démarrer les processus
    for p in processes:
        p.start()
        logger.info(f"Processus {p.name} démarré (PID {p.pid})")

    # Webapp dans le processus principal
    logger.info(f"Démarrage webapp sur port {config['webapp_port']}...")

    # TODO: Décommenter quand le module webapp sera implémenté
    # run_webapp(event_queue, port=config["webapp_port"])

    # Placeholder : attendre les processus
    if processes:
        for p in processes:
            p.join()
    else:
        logger.info("Mode standalone - pas de processus démarrés")
        logger.info("Implémentation en cours... Appuyez sur Ctrl+C pour quitter.")
        try:
            signal.pause()
        except KeyboardInterrupt:
            pass

    logger.info("Application terminée.")


if __name__ == "__main__":
    main()
