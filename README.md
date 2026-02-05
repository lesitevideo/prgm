# Système de détection de franchissement de pylône – Pylon Racing RC

> Détection fiable et reproductible du passage des avions RC lors de courses de Pylon Racing

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Status](https://img.shields.io/badge/status-in%20development-orange.svg)]()

---

## Contexte et objectif

Les courses de **Pylon Racing RC** mettent en jeu des avions de grande taille (≈ 3 m, moteurs 75 cc) évoluant à très haute vitesse (jusqu'à 200 km/h), avec des trajectoires très variables :

- Passages à 0,5 m comme à 10 m du pylône
- Avion à plat ou sur la tranche
- Jusqu'à 4 avions simultanés

### Objectif du système

Déterminer de façon **fiable et reproductible** si un avion a franchi le pylône **par l'extérieur ou par l'intérieur**, afin de :
- Compter les tours
- Appliquer une pénalité éventuelle

Il ne s'agit **pas** de faire du tracking continu ni de la localisation fine, mais de détecter un **événement de passage**.

---

## Principe de fonctionnement

### Détection par franchissement de plan vertical

Le système ne cherche pas une position au centimètre, mais détermine de quel côté d'un **plan vertical** (le pylône) passe l'avion.

**Calcul** : Produit scalaire `(position_relative · vecteur_normal_plan)`
- Positif → extérieur ✓
- Négatif → intérieur ✗
- ≈ 0 → zone morte (no call)

### Technologie : UWB (Ultra Wide Band)

Utilisation d'une **mesure temporelle** (temps d'arrivée radio), pas de puissance reçue (RSSI).

**Pourquoi pas le RSSI ?**
- Dépend fortement de l'orientation
- Très sensible au multipath
- Non déterministe
- Peu défendable en arbitrage

### Logique de décision anti-contestation

1. **Zone morte** : ±1m autour du plan (configurable)
2. **Règle des 3 blinks** : Pénalité UNIQUEMENT si ≥3 blinks consécutifs côté intérieur avec marge suffisante
3. **Sinon** : passage validé ou "no call"

> **Philosophie** : Robustesse sportive avant précision absolue

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         AVIONS (4 max)                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ESP32-C3  │  │ESP32-C3  │  │ESP32-C3  │  │ESP32-C3  │       │
│  │+ UWB Tag │  │+ UWB Tag │  │+ UWB Tag │  │+ UWB Tag │       │
│  │  < 15g   │  │  < 15g   │  │  < 15g   │  │  < 15g   │       │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘       │
│       │ Blinks      │ Blinks      │ Blinks      │ Blinks      │
│       │ 20-40Hz     │ 20-40Hz     │ 20-40Hz     │ 20-40Hz     │
│       └─────────────┴─────────────┴─────────────┘             │
└───────────────────────────────┬───────────────────────────────┘
                                │ UWB Radio
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                      PYLÔNE 1 (Ancres)                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ESP32 +   │  │ESP32 +   │  │ESP32 +   │  │ESP32 +   │       │
│  │UWB       │  │UWB       │  │UWB       │  │UWB       │       │
│  │+ RS485   │  │+ RS485   │  │+ RS485   │  │+ RS485   │       │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘       │
│       └─────────────┴─────────────┴─────────────┘             │
│                            RS485                               │
│                              │                                 │
│                          RJ45 (25m)                            │
│                    (données + alim 24V PoE)                    │
└──────────────────────────────┬──────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                    BASE JUGES (Raspberry Pi)                    │
│  ┌──────────────────────────────────────────────────┐          │
│  │  - Collecte timestamps                           │          │
│  │  - Synchronisation temporelle ancres             │          │
│  │  - Calcul franchissement de plan                 │          │
│  │  - Décision intérieur/extérieur                  │          │
│  │  - Logs CSV/JSON                                 │          │
│  │  - Interface web temps réel                      │          │
│  └──────────────────────────────────────────────────┘          │
│                                                                 │
│  Sortie : Écran HDMI (fourni par le site)                      │
└─────────────────────────────────────────────────────────────────┘
```

### Composants principaux

#### Tags embarqués (avions)
- **Matériel** : ESP32-C3 + UWB (DWM1000/DWM3000) + LiPo
- **Poids** : < 15g
- **Fonction** : Émission blinks UWB (20-40 Hz) avec ID + compteur + CRC

#### Ancres (pylônes)
- **Quantité** : 3-4 par pylône (géométrie triangulaire, différentes hauteurs)
- **Matériel** : ESP32 + UWB + MAX485 + DC-DC 24V→5V
- **Fonction** : Réception blinks + horodatage matériel + transmission RS485

#### Liaison pylône → base
- **Support** : RJ45 Cat5e/Cat6 (25-30m)
- **Protocole** : RS485
- **Alimentation** : Pseudo-PoE 24V

#### Base juges
- **Matériel** : Raspberry Pi 4 (4GB) ou Pi 5
- **Rôle** : Collecte, synchronisation, calcul géométrique, décision, logs, affichage

---

## Contraintes techniques

### Dynamiques
- **Vitesse max** : 200 km/h (55 m/s)
- **Temps de passage** : 100-300 ms
- **Avions simultanés** : 4 maximum

### Fréquence et couverture

À 200 km/h :
- **20 Hz** → 2,75 m entre mesures
- **40 Hz** → 1,4 m entre mesures

Sur zone ±5m : **4 à 8 mesures exploitables** → décision fiable

### Géométriques
- Distance pylône variable : 0,5m à 10m
- Orientation imprévisible
- Pylônes PVC (non métalliques)

### Terrain
- Installation temporaire
- Décision explicable (arbitrage)
- Juges à ~25m

---

## Liste de matériel

### Budget total : 744€ - 1 194€

Installation complète pour **2 pylônes** et **4-6 avions**.

#### Tags embarqués (4-6 unités)
| Composant | Quantité | Prix unitaire | Total |
|-----------|----------|---------------|-------|
| DWM3000 ou DWM1000 | 4-6 | 20-30€ | 80-180€ |
| ESP32-C3 Mini | 4-6 | 3-5€ | 12-30€ |
| LiPo 1S 150-250mAh | 4-6 | 5-8€ | 20-48€ |
| PCB custom | 1 lot | 15-25€ | 15-25€ |
| Connectique | 1 lot | 10€ | 10€ |

**Sous-total : 137-293€**

#### Ancres UWB (8 unités)
| Composant | Quantité | Prix unitaire | Total |
|-----------|----------|---------------|-------|
| DWM3000 ou DWM1000 | 8 | 20-30€ | 160-240€ |
| ESP32 DevKit | 8 | 5-8€ | 40-64€ |
| MAX485 RS485 | 8 | 2-4€ | 16-32€ |
| DC-DC LM2596 24V→5V | 8 | 2-3€ | 16-24€ |
| Boîtier IP65 | 8 | 5-8€ | 40-64€ |
| RJ45 étanche | 8 | 3-5€ | 24-40€ |
| Support mécanique | 2 lots | 15€ | 30€ |

**Sous-total : 326-494€**

#### Liaison RS485 (2 pylônes)
| Composant | Quantité | Prix unitaire | Total |
|-----------|----------|---------------|-------|
| Câble RJ45 FTP 30m | 2 | 15-25€ | 30-50€ |
| Injecteur PoE custom | 2 | 5-8€ | 10-16€ |
| USB→RS485 (Pi) | 2 | 8-12€ | 16-24€ |

**Sous-total : 56-90€**

#### Base Raspberry Pi
| Composant | Quantité | Prix unitaire | Total |
|-----------|----------|---------------|-------|
| Raspberry Pi 4 4GB | 1 | 60-80€ | 60-80€ |
| Carte SD 32GB | 1 | 10-15€ | 10-15€ |
| Alimentation USB-C 5V 3A | 1 | 10-12€ | 10-12€ |
| Boîtier avec ventilation | 1 | 10-15€ | 10-15€ |

**Sous-total : 90-122€**

#### Outillage
| Élément | Coût |
|---------|------|
| Fer à souder + étain | 30-50€ |
| Multimètre | 20-30€ |
| Pince à sertir RJ45 | 15-25€ |
| Télémètre laser | 20-40€ |
| Visserie, colliers | 30€ |
| Câbles, breadboards | 20€ |

**Sous-total : 135-195€**

### Fournisseurs recommandés

- **UWB** : Makerfabs, Seeed Studio, Mouser
- **ESP32** : AliExpress (C3), Amazon (DevKit)
- **Connectique** : RS Components, Farnell, Amazon
- **PCB** : JLCPCB, PCBWay, Aisler

---

### Configuration

Voir [docs/configuration.md](docs/configuration.md) pour :
- Calibration des ancres
- Configuration du plan de pylône
- Réglage de la zone morte
- Paramètres RS485

---

## Roadmap

### Phase 1 - POC laboratoire (2-3 semaines)
- [x] Firmware tag : Émission blinks UWB
- [x] Firmware ancre : Réception + horodatage + RS485
- [x] Script Python collecte RS485
- [x] Tests : 1 tag + 3 ancres

### Phase 2 - Prototype terrain statique (2-3 semaines)
- [ ] Synchronisation maître/esclaves
- [ ] Algorithme détection de plan
- [ ] Logique anti-contestation
- [ ] Logs CSV/JSON structurés
- [ ] Tests : Tag sur perche, passages manuels

### Phase 3 - Validation dynamique (1-2 semaines)
- [ ] Gestion 4 avions simultanés
- [ ] Interface web temps réel
- [ ] Tests terrain : avions réels

### Phase 4 - Industrialisation
- [ ] Documentation utilisateur
- [ ] Scripts installation automatique
- [ ] Procédure calibration simplifiée
- [ ] Guide dépannage

---


### Points d'attention

**Synchronisation temporelle** : Modèle linéaire `t_corrigé = a × t_local + b` via sync maître/esclaves

**Géométrie** : Support mécanique reproductible > précision absolue

**Multi-avions** : Slotting + jitter ±1-2ms pour anti-collision

---
