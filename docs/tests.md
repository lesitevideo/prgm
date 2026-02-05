# Tests terrain

Ce document définit le protocole de test, les métriques à collecter et les critères de succès.

---

## 1. Objectifs des tests

| Objectif | Description |
|----------|-------------|
| Valider la détection | Le système détecte correctement les passages intérieur/extérieur |
| Mesurer la fiabilité | Taux de blinks reçus, taux de no-call |
| Identifier les limites | Conditions d'échec, cas limites |
| Calibrer les seuils | Zone morte, règle des 3 blinks |

---

## 2. Configuration de test

### Matériel requis

| Élément | Quantité | Usage |
|---------|----------|-------|
| Pylône complet | 1 | Installation standard |
| Tags UWB | 4-6 | Simulation multi-avions |
| Perche/canne | 1 | Porter le tag à distance |
| Mètre ruban | 1 | Mesurer les distances |
| Chronomètre | 1 | Mesurer les vitesses |
| Ordinateur portable | 1 | Monitoring temps réel |

### Disposition du terrain

```
                    10m
        ◄───────────────────────►

        ┌─────────────────────────┐
        │      Zone de test       │     ▲
        │                         │     │
   ─────┼─────────●───────────────┼─────┤ 5m
        │       Pylône            │     │
        │                         │     ▼
        └─────────────────────────┘

        Marquages au sol :
        ● 0.5m du pylône
        ● 2m du pylône
        ● 5m du pylône
        ● 10m du pylône
```

---

## 3. Protocole de test

### Phase A : Tests statiques (calibration)

#### A1. Validation du signe

| # | Action | Résultat attendu |
|---|--------|------------------|
| 1 | Placer tag à 3m côté EXTÉRIEUR, immobile | Distance signée > 0, "EXTERIOR" |
| 2 | Placer tag à 3m côté INTÉRIEUR, immobile | Distance signée < 0, "INTERIOR" |
| 3 | Placer tag sur le plan du pylône | Distance ≈ 0, "DEAD_ZONE" |

#### A2. Test de portée

| # | Distance | Durée | Métrique |
|---|----------|-------|----------|
| 1 | 0.5m | 30s | Taux de réception |
| 2 | 2m | 30s | Taux de réception |
| 3 | 5m | 30s | Taux de réception |
| 4 | 10m | 30s | Taux de réception |
| 5 | 15m | 30s | Taux de réception |

**Critère** : Taux de réception > 95% jusqu'à 10m

### Phase B : Tests dynamiques (passage à la main)

#### B1. Passages lents (marche ~5 km/h)

| # | Trajectoire | Distance | Attendu |
|---|-------------|----------|---------|
| 1 | Extérieur → Extérieur | 0.5m | VALID |
| 2 | Extérieur → Extérieur | 2m | VALID |
| 3 | Extérieur → Extérieur | 5m | VALID |
| 4 | Intérieur → Intérieur | 2m | PENALTY |
| 5 | Zone morte (sur le plan) | 0m | NO_CALL |

**Répéter** : 10 fois chaque cas

#### B2. Passages rapides (course ~15 km/h)

Même protocole que B1, à vitesse de course.

### Phase C : Tests multi-tags

#### C1. Passages simultanés (2 tags)

| # | Configuration | Attendu |
|---|---------------|---------|
| 1 | 2 tags extérieur, même instant | 2 × VALID |
| 2 | 1 extérieur + 1 intérieur | 1 × VALID + 1 × PENALTY |
| 3 | 2 tags, décalage 100ms | Détection correcte des 2 |

#### C2. Passages simultanés (4 tags)

| # | Configuration | Attendu |
|---|---------------|---------|
| 1 | 4 tags extérieur | 4 × VALID |
| 2 | 4 tags, positions variées | Détection correcte |

### Phase D : Tests de robustesse

#### D1. Orientation du tag

| # | Orientation | Attendu |
|---|-------------|---------|
| 1 | Tag horizontal (plat) | Détection OK |
| 2 | Tag vertical (tranche) | Détection OK |
| 3 | Tag à 45° | Détection OK |
| 4 | Tag tournant pendant passage | Détection OK |

#### D2. Conditions dégradées

| # | Condition | Action | Observation |
|---|-----------|--------|-------------|
| 1 | 1 ancre déconnectée | Passage test | Dégradation acceptable ? |
| 2 | 2 ancres déconnectées | Passage test | Système détecte l'erreur ? |
| 3 | Batterie tag faible | Passage test | Dégradation RSSI ? |

---

## 4. Métriques à collecter

### 4.1 Taux de blinks reçus

```
taux_reception = blinks_recus / blinks_attendus × 100%
```

| Seuil | Interprétation |
|-------|----------------|
| > 98% | Excellent |
| 95-98% | Acceptable |
| 90-95% | Limite |
| < 90% | Échec |

**Collecte** : Par le Raspberry Pi, log de chaque blink avec timestamp.

### 4.2 Taux de "no call"

```
taux_no_call = passages_no_call / passages_totaux × 100%
```

| Contexte | Seuil acceptable |
|----------|------------------|
| Passage clair (>2m du plan) | < 1% |
| Passage zone limite (0.5-2m) | < 10% |
| Passage sur le plan | 100% (attendu) |

### 4.3 Faux positifs pénalité

```
taux_faux_positif = penalites_incorrectes / passages_exterieurs × 100%
```

| Seuil | Interprétation |
|-------|----------------|
| 0% | Objectif |
| < 0.5% | Acceptable |
| > 1% | Inacceptable |

### 4.4 Faux négatifs pénalité

```
taux_faux_negatif = penalites_manquees / passages_interieurs × 100%
```

| Seuil | Interprétation |
|-------|----------------|
| < 5% | Acceptable (mieux vaut no-call que faux positif) |
| > 10% | À investiguer |

### 4.5 Latence de décision

```
latence = timestamp_decision - timestamp_dernier_blink
```

| Seuil | Interprétation |
|-------|----------------|
| < 100ms | Excellent |
| 100-500ms | Acceptable |
| > 500ms | Trop lent |

---

## 5. Format des logs de test

### Fichier de session

```
tests/
└── 2024-03-15_session_01/
    ├── config.json          # Configuration utilisée
    ├── raw_blinks.csv       # Tous les blinks bruts
    ├── decisions.csv        # Toutes les décisions
    ├── test_runs.csv        # Métadonnées des passages
    └── summary.json         # Résumé des métriques
```

### Format raw_blinks.csv

```csv
timestamp_ms,anchor_id,tag_id,seq_num,timestamp_uwb,rssi,sync_status
1710512345123,17,16,1234,0x1A2B3C4D5E,-45,OK
1710512345156,18,16,1234,0x1A2B3C4D60,-48,OK
...
```

### Format decisions.csv

```csv
timestamp_ms,tag_id,decision,signed_distance_m,confidence,supporting_blinks,reason
1710512345200,16,VALID,2.34,0.95,5,"Passage extérieur clair"
1710512346500,17,PENALTY,-1.82,0.88,4,"3 blinks consécutifs intérieur"
...
```

### Format test_runs.csv

```csv
run_id,timestamp_start,tag_id,trajectory,expected_result,actual_result,distance_m,speed_kmh,notes
A1_01,1710512345000,16,exterior,VALID,VALID,2.0,5,"Test calibration"
B1_05,1710512400000,17,interior,PENALTY,PENALTY,1.5,15,"Passage rapide"
...
```

---

## 6. Critères de succès

### Phase POC (Phase 1-2)

| Critère | Seuil | Priorité |
|---------|-------|----------|
| Taux réception 0.5-10m | > 95% | Bloquant |
| Faux positifs pénalité | 0% | Bloquant |
| Faux négatifs pénalité | < 10% | Important |
| Taux no-call (clair) | < 5% | Important |
| Latence décision | < 500ms | Souhaité |

### Phase Production (Phase 3+)

| Critère | Seuil | Priorité |
|---------|-------|----------|
| Taux réception | > 98% | Bloquant |
| Faux positifs pénalité | 0% | Bloquant |
| Faux négatifs pénalité | < 5% | Bloquant |
| Taux no-call (clair) | < 2% | Important |
| Latence décision | < 200ms | Important |
| Multi-tags (4 avions) | 100% détection | Bloquant |

---

## 7. Procédure de test complète

### Checklist pré-test

- [ ] Batteries tags chargées (> 80%)
- [ ] Ancres alimentées et connectées
- [ ] Synchronisation OK (vérifier sur dashboard)
- [ ] Configuration chargée et vérifiée
- [ ] Logs activés
- [ ] Marquages au sol en place
- [ ] Météo acceptable (pas de pluie forte)

### Déroulement

```
1. Installation (30 min)
   - Monter le pylône et les ancres
   - Câbler et mettre sous tension
   - Vérifier les connexions

2. Calibration (15 min)
   - Exécuter tests A1 (validation signe)
   - Ajuster si nécessaire

3. Tests statiques (30 min)
   - Exécuter tests A2 (portée)
   - Noter les résultats

4. Tests dynamiques (45 min)
   - Exécuter tests B1 et B2
   - 10 répétitions par cas

5. Tests multi-tags (30 min)
   - Exécuter tests C1 et C2

6. Tests robustesse (30 min)
   - Exécuter tests D1 et D2

7. Analyse (après session)
   - Calculer les métriques
   - Générer le rapport
   - Identifier les améliorations
```

### Template rapport de test

```markdown
# Rapport de test - [DATE]

## Configuration
- Version firmware : X.Y.Z
- Configuration : config_test_01.json
- Conditions météo : [soleil/nuageux/etc.]

## Résultats

### Métriques globales
| Métrique | Valeur | Seuil | Status |
|----------|--------|-------|--------|
| Taux réception | XX% | >95% | OK/KO |
| Faux positifs | X% | 0% | OK/KO |
| ... | ... | ... | ... |

### Détail par phase
[Tableaux détaillés]

### Problèmes identifiés
1. [Description problème 1]
2. [Description problème 2]

### Actions correctives
1. [Action 1]
2. [Action 2]

## Conclusion
[GO / NO-GO pour phase suivante]
```

---

## 8. Outils de test

### Script de test automatisé

```bash
# Lancer une session de test
./scripts/run_test_session.sh --config config/test_outdoor.json --duration 3600

# Analyser les résultats
./scripts/analyze_results.py tests/2024-03-15_session_01/

# Générer le rapport
./scripts/generate_report.py tests/2024-03-15_session_01/ --output report.pdf
```

### Dashboard de test

Interface web temps réel affichant :
- Status de chaque ancre (sync, RSSI)
- Blinks reçus par tag
- Décisions en temps réel
- Métriques cumulées
