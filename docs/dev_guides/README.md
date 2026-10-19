# 📚 UN1CA Carbon — Developer & Engineering Guides

Welcome to the **UN1CA Carbon** technical developer documentation. This directory contains in-depth architectural guides, engineering specifications, and migration tutorials for maintaining, porting, and extending the ROM.

---

## 📑 Index of Guides

| Guide | Description | Target Audience |
| :--- | :--- | :--- |
| **[01. Carbon Engine Architecture](01_CARBON_ENGINE_ARCHITECTURE.md)** | Deep dive into the native C++20 build orchestrator, Process Supervisor, SQLite ledger, and the 9 macroscopic pipeline stages. | Core Engineers, CI/CD Maintainers |
| **[02. Porting UN1CA Scripts to Engine](02_PORTING_UN1CA_SCRIPTS_TO_ENGINE.md)** | Step-by-step tutorial on translating legacy imperative bash scripts (`customize.sh`, `apply_modules.sh`) into declarative `Carbonfile.json` actions, multi-target macros, and idempotency patterns. | Module Developers, Porters |
| **[03. DAG Caching & SQLite Ledger](03_DAG_CACHING_AND_LEDGER.md)** | Comprehensive explanation of OpenSSL SHA-256 fingerprinting, cache invalidation, topological task ordering, and incremental phase skipping. | Build Engineers |
| **[04. Boot Animation & Graphics](04_BOOTANIMATION_AND_GRAPHICS_SUBSYSTEM.md)** | Architecture of Frame-0 SurfaceFlinger rendering, removal of Samsung QMG daemons, and runtime dynamic switching between Carbon and Android 17. | Display & UI Developers |

---

## 🚀 Quick Start for Contributors

### Compiling Carbon Engine
```bash
cd carbon_engine/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) carbon-engine
```

### Running an Incremental Build (Fast DAG Caching)
```bash
./carbon_engine/make_carbon_engine.sh --target=dm1q
```

### Running a Clean Rebuild (Wiping Cache Ledger)
```bash
./carbon_engine/make_carbon_engine.sh --target=dm1q --force
```

### Inspecting Cache Records
```bash
./carbon_engine/build/carbon-engine inspect
```

---

## 🏛️ Upstream & Acknowledgements
* **One UI 9 Framework Engine:** [@Rezoss-Reza](https://github.com/Rezoss-Reza)
* **Custom Kernel (Linux 5.15.216):** [@GoRhanHee](https://github.com/GoRhanHee)
* **Root Solution:** [KernelSU-Next](https://github.com/rifsxd/KernelSU-Next)
* **Custom Recovery:** [TeamWin/TWRP](https://twrp.me)
* **UN1CA Core:** [Salvo Giangreco](https://github.com/salvogiangri)
* **Visuals & Wallpapers:** [Cosine & NPL ROM Team](https://github.com/cosinemyan)
