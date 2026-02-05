# Câblage et connexions

Ce document décrit le câblage physique du système : RS485, alimentation pseudo-PoE, et protections.

---

## 1. Vue d'ensemble

```
┌─────────────────┐                                    ┌─────────────────┐
│   BASE JUGES    │                                    │     PYLÔNE      │
│                 │         Câble RJ45 (30m)           │                 │
│  Raspberry Pi   │◄──────────────────────────────────►│   4 × Ancres    │
│  + USB-RS485    │         RS485 + 24V                │   UWB + ESP32   │
│                 │                                    │                 │
└─────────────────┘                                    └─────────────────┘
```

---

## 2. Attribution des paires RJ45

### Standard retenu : T568B

```
    ┌─────────────────────────────────────────┐
    │             Connecteur RJ45             │
    │  ┌───┬───┬───┬───┬───┬───┬───┬───┐     │
    │  │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │     │
    │  └───┴───┴───┴───┴───┴───┴───┴───┘     │
    │    │   │   │   │   │   │   │   │       │
    │    │   │   │   │   │   │   │   │       │
    │   O/B  O  V/B  B  B/B  V  M/B  M       │
    │                                         │
    │  O/B = Orange/Blanc  B = Bleu          │
    │  O   = Orange        B/B = Bleu/Blanc  │
    │  V/B = Vert/Blanc    M/B = Marron/Blanc│
    │  V   = Vert          M   = Marron      │
    └─────────────────────────────────────────┘
```

### Attribution des fonctions

| Paire | Broches | Couleurs | Fonction |
|-------|---------|----------|----------|
| 1 | 1, 2 | Orange/Blanc, Orange | **RS485 A (Data+)** |
| 2 | 3, 6 | Vert/Blanc, Vert | **RS485 B (Data-)** |
| 3 | 4, 5 | Bleu, Bleu/Blanc | **+24V (alimentation)** |
| 4 | 7, 8 | Marron/Blanc, Marron | **GND (masse)** |

### Schéma de câblage

```
BASE JUGES                              PYLÔNE
──────────────────────────────────────────────────────

Broche 1 ─────── RS485 A ─────────────► RS485 A (tous)
Broche 2 ─────── RS485 A ─────────────► RS485 A (tous)

Broche 3 ─────── RS485 B ─────────────► RS485 B (tous)
Broche 6 ─────── RS485 B ─────────────► RS485 B (tous)

Broche 4 ─────── +24V ────────────────► +24V entrée
Broche 5 ─────── +24V ────────────────► +24V entrée

Broche 7 ─────── GND ─────────────────► GND commun
Broche 8 ─────── GND ─────────────────► GND commun
```

---

## 3. Alimentation pseudo-PoE

### Principe

Injection de 24V DC sur les paires non utilisées par Ethernet traditionnel (paires 3 et 4).

**Attention** : Ce n'est PAS du PoE standard IEEE 802.3af. Ne pas connecter à un switch PoE.

### Spécifications

| Paramètre | Valeur | Justification |
|-----------|--------|---------------|
| Tension nominale | 24V DC | Marge pour pertes en ligne |
| Tension min | 18V DC | Après pertes câble 30m |
| Courant max | 2A | 4 ancres × 500mA |
| Puissance max | 48W | Avec marge |

### Calcul des pertes en ligne

Câble Cat5e, conducteur 24 AWG, 30m :
- Résistance : ~2.8 Ω par conducteur
- 2 conducteurs en parallèle par paire : ~1.4 Ω
- Aller + retour : ~2.8 Ω total
- À 2A : chute de tension = 5.6V
- Tension au pylône : 24V - 5.6V = **18.4V** (OK)

### Injecteur côté base

```
                    ┌─────────────────────────┐
  Alim 24V ────────►│      INJECTEUR          │
                    │                         │
  USB-RS485 ──────► │  ┌─────────────────┐    │
  (depuis Pi)       │  │ Adaptateur RJ45 │    │────► Vers pylône
                    │  │ + borniers      │    │     (câble 30m)
                    │  └─────────────────┘    │
                    └─────────────────────────┘
```

**Composants** :
- Embase RJ45 femelle
- Borniers à vis pour +24V et GND
- Module USB-RS485 (FTDI ou CH340)

### Répartiteur côté pylône

```
                    ┌─────────────────────────────────────┐
  Depuis base ─────►│         RÉPARTITEUR PYLÔNE         │
  (câble 30m)       │                                     │
                    │  ┌──────────┐    ┌──────────────┐  │
                    │  │ DC-DC    │    │ RS485        │  │
                    │  │ 24V→5V   │    │ splitter     │  │
                    │  │ (LM2596) │    │ (4 sorties)  │  │
                    │  └────┬─────┘    └──────┬───────┘  │
                    │       │                 │          │
                    │       ▼                 ▼          │
                    │    5V vers          RS485 vers    │
                    │    4 ancres         4 ancres      │
                    └─────────────────────────────────────┘
```

---

## 4. Connexions des ancres

### Schéma d'une ancre

```
┌────────────────────────────────────────────────────────┐
│                      ANCRE UWB                         │
│                                                        │
│   +5V ──────────────────┬───────────────► ESP32 VIN   │
│   (depuis DC-DC)        │                              │
│                         └───────────────► DW1000 VCC  │
│                                                        │
│   GND ──────────────────┬───────────────► ESP32 GND   │
│                         └───────────────► DW1000 GND  │
│                                                        │
│   RS485 A ──────────────► MAX485 A                    │
│   RS485 B ──────────────► MAX485 B                    │
│                                                        │
│   MAX485 RO ────────────► ESP32 GPIO16 (RX)           │
│   MAX485 DI ────────────► ESP32 GPIO17 (TX)           │
│   MAX485 DE/RE ─────────► ESP32 GPIO4 (direction)     │
│                                                        │
│   ESP32 SPI ────────────► DW1000 SPI                  │
│   (MOSI, MISO, CLK, CS)                               │
│                                                        │
│   ESP32 GPIO5 ──────────► DW1000 IRQ                  │
│   ESP32 GPIO18 ─────────► DW1000 RST                  │
└────────────────────────────────────────────────────────┘
```

### Pinout ESP32 → DW1000

| ESP32 GPIO | DW1000 Pin | Fonction |
|------------|------------|----------|
| GPIO23 | MOSI | SPI Data Out |
| GPIO19 | MISO | SPI Data In |
| GPIO18 | CLK | SPI Clock |
| GPIO5 | CS | SPI Chip Select |
| GPIO4 | IRQ | Interruption RX |
| GPIO2 | RST | Reset |

### Pinout ESP32 → MAX485

| ESP32 GPIO | MAX485 Pin | Fonction |
|------------|------------|----------|
| GPIO16 | RO | Receive Out |
| GPIO17 | DI | Data In |
| GPIO4 | DE + RE | Direction (liés ensemble) |

---

## 5. Protections

### Protection contre les surtensions

```
+24V ────┬────[Fusible 3A]────┬────► Distribution
         │                    │
         │                 [TVS 28V]
         │                    │
GND ─────┴────────────────────┴────► GND
```

**Composants** :
- Fusible réarmable (PTC) 3A
- TVS bidirectionnel 28V (P6KE28CA)

### Protection RS485

```
RS485 A ────┬────[R 120Ω]────┬────► Vers MAX485
            │                │
         [TVS]            [TVS]
            │                │
RS485 B ────┴────────────────┴────► Vers MAX485
```

**Composants** :
- Résistance de terminaison 120Ω (aux deux extrémités)
- TVS bipolaire pour RS485 (SM712)

### Protection boîtier

| Élément | Spécification |
|---------|---------------|
| Boîtier | IP65 ABS 100×68×50mm |
| Presse-étoupe | PG7 pour câble RJ45 |
| Aération | Membrane respirante IP67 |

---

## 6. Liste des composants câblage

### Par pylône (4 ancres)

| Composant | Quantité | Référence | Prix unitaire |
|-----------|----------|-----------|---------------|
| Câble Cat5e FTP 30m | 1 | Extérieur, gaine PE | 15-25€ |
| Embase RJ45 étanche | 2 | IP67 | 5€ |
| Boîtier ABS IP65 | 4 | 100×68×50mm | 5€ |
| Convertisseur DC-DC | 1 | LM2596 24V→5V 3A | 3€ |
| Module MAX485 | 4 | Breakout board | 2€ |
| Résistance 120Ω | 2 | Terminaison RS485 | 0.10€ |
| Fusible PTC 3A | 1 | Réarmable | 1€ |
| TVS 28V | 2 | P6KE28CA | 0.50€ |
| Borniers à vis | 1 lot | 2.54mm | 2€ |

### Côté base

| Composant | Quantité | Référence | Prix unitaire |
|-----------|----------|-----------|---------------|
| Module USB-RS485 | 2 | FTDI FT232 ou CH340 | 10€ |
| Alimentation 24V 3A | 1 | AC-DC enclosed | 15€ |
| Embase RJ45 | 2 | Panneau | 3€ |
| Boîtier base | 1 | ABS avec aération | 10€ |

---

## 7. Schéma de câblage complet

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              BASE JUGES                                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │ Raspberry Pi │    │ Alim 24V 3A  │    │  Écran       │              │
│  │              │    │ (AC-DC)      │    │  (HDMI)      │              │
│  └──────┬───────┘    └──────┬───────┘    └──────────────┘              │
│         │                   │                                           │
│    USB  │              +24V │                                           │
│         ▼                   ▼                                           │
│  ┌──────────────┐    ┌─────────────┐                                   │
│  │ USB-RS485 #1 │    │ Injecteur   │                                   │
│  │ USB-RS485 #2 │◄──►│ RJ45 + 24V  │                                   │
│  └──────────────┘    └──────┬──────┘                                   │
│                             │                                           │
└─────────────────────────────┼───────────────────────────────────────────┘
                              │
                    Câble RJ45 30m (×2 pylônes)
                              │
┌─────────────────────────────┼───────────────────────────────────────────┐
│                             ▼                    PYLÔNE 1               │
│                      ┌─────────────┐                                    │
│                      │ Répartiteur │                                    │
│                      │ 24V→5V      │                                    │
│                      │ RS485 split │                                    │
│                      └──────┬──────┘                                    │
│                             │                                           │
│         ┌───────────────────┼───────────────────┐                      │
│         │           ┌───────┴───────┐           │                      │
│         ▼           ▼               ▼           ▼                      │
│    ┌─────────┐ ┌─────────┐    ┌─────────┐ ┌─────────┐                 │
│    │ Ancre 1 │ │ Ancre 2 │    │ Ancre 3 │ │ Ancre 4 │                 │
│    │ (bas G) │ │ (bas D) │    │ (bas C) │ │ (haut)  │                 │
│    └─────────┘ └─────────┘    └─────────┘ └─────────┘                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 8. Vérifications avant mise sous tension

### Checklist

- [ ] Continuité des paires RS485 (broches 1-2 et 3-6)
- [ ] Continuité des paires alimentation (broches 4-5 et 7-8)
- [ ] Pas de court-circuit entre paires
- [ ] Polarité +24V correcte
- [ ] Résistances de terminaison 120Ω installées
- [ ] Fusible en place
- [ ] Boîtiers fermés et étanches

### Test de mise en service

1. **Sans charge** : Vérifier 24V à l'entrée du pylône
2. **Avec DC-DC seul** : Vérifier 5V en sortie
3. **Une ancre** : Vérifier démarrage et communication RS485
4. **Toutes ancres** : Vérifier courant total < 2A
