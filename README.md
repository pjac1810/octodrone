# OCTODRONE — synthé drone « organique » pour M-VAVE FM-1

Firmware alternatif qui transforme le FM-1 en synthé drone à 8 voix sur le modèle de la SOMA Laboratory Lyra-8. On y retrouve :

- 8 voix groupées en 4 paires et 2 quatuors ;
- des formes d'onde du triangle au carré (SHARP) ;
- la modulation de fréquence croisée entre paires, avec le commutateur 34>56 qui referme les quatre paires en une seule boucle ;
- l'HYPER LFO (deux carrés, OR/AND, LINK) ;
- le TOTAL FEEDBACK ;
- les enveloppes SLOW/FAST, HOLD et VIBRATO ;
- le double MOD DELAY à feedback croisé, modulé par lui-même ou par le LFO ;
- la DISTORTION.

C'est un projet indépendant, sans lien avec SOMA Laboratory ni M-VAVE. Le son est entièrement calculé en entiers.

Il est bâti sur [foma](https://github.com/konsumer/foma) (plateforme Felucca), avec installeur web, mode secours et retour au firmware d'origine.

## Jouer

Les **touches blanches** sont les 8 capteurs :

| Touches | Voix |
|---|---|
| F3 G3 A3 B3 C4 D4 E4 F4 | 1 à 8 |
| G4 A4 B4 C5 D5 E5 F5 G5 | 1 à 8 (une seconde fois) |

Une **touche noire** touche à la fois ses deux voisines blanches. Les touches donnent la hauteur de chaque voix, mais pas la mélodie : chaque voix a son propre réglage TUNE, comme sur l'original. Les touches allumées montrent les voix qui sonnent.

| Commande FM-1 | Fonction |
|---|---|
| SELECT | paire courante : 1-2, 3-4, 5-6, 7-8 |
| ALGORITHM | modulation de la paire : OFF, FM (par la paire voisine), LFO (ou la sortie, si TOTAL FB est actif) |
| PRESETS | panneau P1 à P8 |
| HOME | PAIR : TUNE (1re voix), TUNE (2e voix), MOD (profondeur), SHARP (triangle → carré) |
| ENV | QUADS : HOLD 1-4, HOLD 5-8, PITCH 1-4, PITCH 5-8 (transposition du quatuor) |
| LFO | HYPER LFO : FREQ A, FREQ B (0,05 Hz à 400 Hz), MODE OR/AND, LINK (A module B) |
| FX | MOD DELAY : TIME 1, TIME 2 (2 ms à 0,9 s), FB, MIX ; re-presser : DELAY MOD : MOD 1, MOD 2, SOURCE SELF/LFO, SHAPE TRI/SQR |
| EDIT | DISTORTION : DRIVE, MIX, SPREAD (lignes de délai à gauche et à droite), TOUCH (vitesse d'ouverture des capteurs) |
| SEL | FAST / SLOW pour la paire courante |
| GLO | TOTAL FB : la sortie remplace le LFO comme source de modulation |
| ARP | VIBRATO (chaque voix a le sien) |
| SEQ | 34>56 : les quatre paires en une seule boucle FM (1-2 → 3-4 → 5-6 → 7-8 → 1-2) |
| PLAY | LATCH : une touche allume ou éteint son capteur, comme une main posée |
| REC | relâche tout ce qui est verrouillé |
| SAVE | garde les 8 panneaux en mémoire flash ; maintenu 2 s, remet le panneau d'usine dans l'emplacement courant |
| OCT− / OCT+ | octave de toutes les voix (les deux ensemble : 0) |
| MASTER | volume |
| MIDI | les notes jouent les capteurs comme le clavier ; CC 123 coupe tout |
| Audio USB | un ordinateur peut jouer dans l'entrée, comme la prise EXT IN, avant le délai et la distorsion |

**Écran** :

- en haut, l'oscilloscope de la sortie ;
- au milieu, les 4 paires, avec leur source de modulation (« FM 34 » veut dire modulée par la paire 3-4), S ou F, et la jauge MOD ;
- en dessous, les 8 capteurs, qui s'illuminent avec le niveau de chaque voix ; le trait orange signale un capteur verrouillé ;
- puis les 4 boutons rotatifs de la page ;
- en bas, les commutateurs actifs.

Le TUNE se règle par pas de 5 cents, plus vite si on tourne vite. Les plages sont :

- voix 1-2 : graves, de C1 à C5 ;
- voix 3 à 6 : médiums, de C2 à C6 ;
- voix 7-8 : une octave au-dessus, de C3 à C7.

**HOLD** maintient le quatuor comme un bourdon à partir d’un tiers de course environ. Les paires en FAST demandent de le pousser plus haut, comme sur l'original.

## Panneaux d'usine

| N° | Nom | Caractère |
|---|---|---|
| 1 | DRONE | la mineur ouvert, délai doux |
| 2 | CATHEDRAL | ré grave tenu par HOLD 1-4, longs délais, vibrato |
| 3 | SWARM | toutes les paires en FM, boucle 34>56, clusters désaccordés |
| 4 | APOCALYPSE | TOTAL FB, LFO en AND, distorsion à fond |
| 5 | HYPER PULSE | LFO audio en AND + LINK, ondes carrées |
| 6 | TAPE BENDS | délais modulés par eux-mêmes (SELF), effets de bande |
| 7 | BELLS | paires FAST, FM légère, notes aiguës |
| 8 | INIT | point de départ neutre |

## Obtenir et installer le firmware

- **GitHub** : pousse ce dossier sur la branche `main` d'un dépôt. Le workflow compile tout.
  - Le `.fwsc` est dans l'artefact `octodrone-fwsc` et sur la branche `firmware`.
  - Avec Settings → Pages → Source : GitHub Actions, l'émulateur et l'installeur sont publiés sur GitHub Pages.
- **Docker Desktop** : `./fm1 build` ou `./fm1 dev` (http://localhost:8000).

L'installation se fait depuis la page Install (Chrome ou Edge, FM-1 branché en USB).

**Secours** : maintiens OCT− à l'allumage ; l'onglet Rescue remet le firmware M-VAVE V15. Installer un firmware non officiel se fait à tes risques.

## Développement

- `src/dsp.c` : voix, FM croisée, HYPER LFO, MOD DELAY, distorsion (entiers uniquement)
- `src/main.c` : capteurs, pages, écran, LEDs, MIDI, sauvegarde
- `src/presets.c` : les 8 panneaux d'usine
- `tests/run.sh` : rend les 8 panneaux en WAV, vérifie l'absence d'écrêtage et de composante continue, puis fait un fuzz de tous les réglages (UBSan)
- Identité du paquet : `FM-1_64` (dans `firmware.json`)

## Licence

GPL-3.0-only. Crédits : [LICENSING.md](LICENSING.md).
