# Simulateur RISC-V avec Extension F

Ce projet est un simulateur pédagogique de l'architecture **RISC-V RV32I**, enrichi avec un support partiel de l'**extension F** (virgule flottante, simple précision - IEEE-754).

---

## ✨ Fonctionnalités

- Support des instructions entières **RV32I**
- Ajout des instructions flottantes :
  - `flw` / `fsw` : chargement et stockage de float
  - `fadd.s` / `fsub.s` : addition et soustraction IEEE-754
- Gestion du registre `fcsr`, avec :
  - `fflags` (exceptions : NV, DZ, OF, UF, NX)
  - `frm` (mode d’arrondi : RNE, RTZ, etc.)
- Intégration de la bibliothèque **SoftFloat**
- Organisation mémoire réaliste : `mem0` à `mem3` simulant la RAM
- Compatible avec une future **génération RTL (Vivado HLS)**

---

## 🗂️ Structure du projet

simulateur-riscv/
├── riscv_sim.cpp # Code principal de la boucle Fetch-Decode-Execute
├── softfloat/ # Bibliothèque SoftFloat (version simplifiée)
│ ├── f32_add.c
│ ├── f32_sub.c
│ └── ...
├── makefile
└── README.md

Auteur

**Hamza Echerarhi**  
Master 1 Logiciel pour Systèmes Embarqués  
Université de Bretagne Occidentale (UBO) – 2024/2025


