# BRIEFING PROJET - Système de détection de franchissement de pylône (Pylon Racing RC)

## Contexte du projet

Développement d'un système électronique pour détecter si des avions RC passent du bon côté (extérieur) ou du mauvais côté (intérieur) d'un pylône lors de courses de Pylon Racing.

**Contraintes principales** :
- Avions de 3m évoluant jusqu'à 200 km/h (55 m/s)
- Distance au pylône variable : 0,5m à 10m
- Orientation imprévisible (avion à plat ou sur la tranche)
- Jusqu'à 4 avions simultanés
- Installation temporaire sur terrain d'aéromodélisme
- Décision doit être explicable en cas de contestation

## Architecture matérielle retenue

### Tags embarqués (dans les avions)
- **Matériel** : ESP32-C3 + module UWB (DWM1000 ou DWM3000) + LiPo
- **Poids cible** : < 15g
- **Fonction** : Émettre des "blinks" UWB périodiques (20-40 Hz) contenant ID avion + compteur + CRC
- **Pas de dialogue radio** : émission unidirectionnelle uniquement

### Ancres (au pylône)
- **Quantité** : 3-4 ancres par pylône (2 pylônes total = 6-8 ancres)
- **Matériel** : ESP32 DevKit + module UWB + MAX485 (RS485) + convertisseur DC-DC 24V→5V
- **Disposition** : Géométrie triangulaire autour du pylône, à différentes hauteurs
- **Fonction** : 
  - Réception des blinks UWB
  - **Horodatage matériel précis** (timestamp au niveau UWB)
  - Envoi des timestamps via RS485 vers la base

### Liaison pylône → base juges
- **Support physique** : Câble RJ45 (Cat5e/Cat6) sur ~25-30m
- **Protocole** : RS485
- **Alimentation** : Pseudo-PoE 24V injecté sur le RJ45

### Base juges
- **Matériel** : Raspberry Pi 4 (4GB) ou Pi 5
- **Fonction** :
  - Collecte des timestamps de toutes les ancres via RS485
  - Synchronisation temporelle entre ancres
  - Calcul géométrique du franchissement de plan
  - Décision intérieur/extérieur avec logique anti-contestation
  - Logs (CSV/JSON)
  - Interface web temps réel (affichage tours, pénalités)

## Principe de détection : Franchissement de plan vertical

**Objectif** : Déterminer de quel côté d'un plan vertical (le pylône) passe l'avion.

**Ce qu'on NE fait PAS** :
- Localisation 3D précise au centimètre
- Tracking continu de trajectoire
- Cartographie temps réel

**Ce qu'on fait** :
- Définir un plan vertical passant par le pylône
- Calculer le signe du produit scalaire : `(position_relative · vecteur_normal_plan)`
  - Positif → extérieur ✓
  - Négatif → intérieur ✗
  - ~0 → zone morte (no call)

## Logique de décision (règles sportives)

1. **Zone morte** : ±1m autour du plan (configurable)
2. **Règle anti-contestation** : Pénalité UNIQUEMENT si ≥3 blinks consécutifs côté intérieur avec marge suffisante
3. **Sinon** : passage validé ou "no call"
4. **Priorité** : Robustesse sportive > précision théorique absolue

## Gestion multi-avions (4 simultanés)

**Stratégies anti-collision** :
- Décalage temporel par ID (slotting)
- Jitter pseudo-aléatoire ±1-2ms sur chaque émission
- Fréquence 20-40 Hz garantit plusieurs mesures exploitables même avec pertes

**Identification** :
- Chaque trame contient : ID avion + compteur + ID session + CRC

## Synchronisation temporelle entre ancres

**Problème** : Les ancres ont des horloges indépendantes sujettes à dérive.

**Solutions possibles** :
1. **Maître/esclaves** : Une ancre maître émet des signaux de sync périodiques (500ms-1s) via RS485. Modèle linéaire simple : `t_corrigé = a × t_local + b`
2. **Tag fixe de référence** : Un tag UWB fixe près du pylône sert de référence commune pour recalage

## Stack logicielle suggérée

**Tags (ESP32-C3)** :
- Arduino ou ESP-IDF
- Bibliothèque UWB : DecaWave DW1000/DW3000

**Ancres (ESP32)** :
- Arduino ou ESP-IDF
- UWB + RS485
- Horodatage précis via interruptions matérielles

**Raspberry Pi** :
- Python 3
- Bibliothèques : pyserial (RS485), numpy (calculs), flask/fastapi (interface web)
- Logs : CSV + JSON
- Interface web : HTML/CSS/JS simple ou framework léger (Bootstrap)

## Structure projet souhaitée

```
pylon-racing-detection/
├── firmware/
│   ├── tag/              # Code ESP32-C3 pour tags avions
│   ├── anchor/           # Code ESP32 pour ancres
│   └── common/           # Bibliothèques communes (protocole, CRC, etc.)
├── raspberry/
│   ├── collector/        # Service Python collecte RS485
│   ├── processor/        # Algorithme de décision
│   ├── webapp/           # Interface web
│   └── logs/             # Stockage logs
├── simulation/
│   ├── tag_simulator.py  # Simulateur de tags pour tests
│   └── trajectory_viz.py # Visualisation trajectoires
├── docs/
│   ├── specs.md          # Spécifications complètes
│   ├── protocol.md       # Protocole RS485
│   └── calibration.md    # Procédure calibration terrain
└── tests/
    ├── unit/
    └── integration/
```

## Livrables attendus (par ordre de priorité)

### Phase 1 - POC en laboratoire (2-3 semaines)
1. **Firmware tag** : Émission blinks UWB avec ID
2. **Firmware ancre** : Réception + horodatage + envoi RS485
3. **Script Python de collecte** : Lecture RS485 et parsing
4. **Test** : 1 tag + 3 ancres sur table, validation timestamps

### Phase 2 - Prototype terrain statique (2-3 semaines)
1. **Synchronisation maître/esclaves** : Implémentation et validation
2. **Algorithme de détection de plan** : Calcul géométrique
3. **Logique anti-contestation** : Zone morte + règle des 3 blinks
4. **Logs structurés** : CSV + JSON avec tous les événements
5. **Test** : Tag sur perche, passages manuels à différentes distances

### Phase 3 - Validation dynamique (1-2 semaines)
1. **Gestion 4 avions simultanés** : Anti-collision + identification
2. **Interface web temps réel** : Dashboard avec tours et pénalités
3. **Tests terrain** : Avions réels ou voiture RC rapide

### Phase 4 - Industrialisation (variable)
1. Documentation utilisateur
2. Scripts d'installation automatique (Raspberry Pi)
3. Procédure de calibration simplifiée
4. Guide de dépannage

## Contraintes et préférences

**Code quality** :
- Code commenté en français ou anglais
- Variables/fonctions explicites
- Gestion d'erreurs robuste (critical pour RS485)
- Logging verbeux pour debug

**Performance** :
- Latence minimale sur horodatage UWB (critique)
- Traitement temps réel sur Raspberry Pi (pas de retard > 100ms)

**Portabilité** :
- Code compatible Linux (Raspberry Pi OS)
- Pas de dépendances propriétaires

**Sécurité** :
- Validation CRC sur toutes les trames
- Détection de tags inconnus (ID session)
- Protection contre injections RS485

## Budget matériel

**Total estimé** : 744€ - 1 194€ pour installation complète (2 pylônes, 4-6 tags)

### Détail par poste

| Poste | Fourchette basse | Fourchette haute |
|-------|------------------|------------------|
| Tags embarqués (4-6 avions) | 137€ | 293€ |
| Ancres UWB (8 unités) | 326€ | 494€ |
| Liaison RS485 | 56€ | 90€ |
| Base Raspberry Pi | 90€ | 122€ |
| Outillage et consommables | 135€ | 195€ |

Voir détails complets dans le document de spécifications.

## Documents annexes à consulter

1. **Synthèse technique complète** : [Google Doc](https://docs.google.com/document/d/1nQsoaW9spUC2_j_aSYXD833FrEQhV2qeF5vMoCuNrRQ/edit)
2. **Datasheets** :
   - DWM1000/DWM3000 (modules UWB Qorvo)
   - ESP32-C3 / ESP32 DevKit
   - MAX485 (RS485)

## Questions pour Claude Code

1. Quelle bibliothèque UWB recommandes-tu pour ESP32 (DW1000/DW3000) ?
2. Comment implémenter l'horodatage matériel le plus précis possible ?
3. Quel format de protocole RS485 proposes-tu (binaire ou texte) ?
4. Comment structurer le calcul géométrique de franchissement de plan de façon maintenable ?
5. Architecture logicielle Raspberry Pi : monolithique ou services séparés ?

## Calculs de fréquence et dynamique

À 200 km/h (≈ 55 m/s) :
- **20 Hz** → 2,75 m entre mesures
- **40 Hz** → 1,4 m entre mesures

Sur une zone de contrôle ±5 m autour du pylône :
- **4 à 8 mesures exploitables** par passage
- Suffisant pour décision fiable avec règle des 3 blinks consécutifs

## Philosophie du projet

> "La pertinence finale sera tranchée par les **tests**, pas par les hypothèses."

Privilégier :
- La **robustesse** sur la sophistication
- La **reproductibilité** sur la précision absolue
- L'**explicabilité** (logs, décisions traçables) pour l'arbitrage sportif
- La **simplicité** d'installation terrain

## Calibration terrain

Le système ne cherche pas une localisation absolue au centimètre près, mais une décision de **côté par rapport à un plan vertical**.

**Procédure simplifiée** :
1. Montage des ancres sur support mécanique fixe (colliers, platines)
2. Mesure des positions relatives (mètre ruban ou télémètre)
3. Enregistrement de la géométrie
4. **Test de cohérence** : passage d'un tag de référence dans des positions connues pour valider le signe du calcul

**Tolérance** :
- Zone morte ±1m absorbe les petites imprécisions mécaniques
- Support reproductible d'une manche à l'autre plus important que précision absolue

## Données techniques UWB

**Portée** :
- DWM1000 : ~50-80m en champ libre
- DWM3000 : ~100-150m en champ libre

**Précision temporelle** :
- Time-of-Flight : ~10cm (largement suffisant pour notre usage)
- Ce qui compte : **différence de temps d'arrivée** entre ancres, pas distance absolue

**Consommation** :
- Tags : autonomie cible 1-2 heures (durée d'une compétition)
- Ancres : alimentées en continu via pseudo-PoE

---

**Prêt à démarrer ?** 🚀

Contact : kinoki@kinoki.fr
