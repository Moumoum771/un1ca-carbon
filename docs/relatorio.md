# 📊 RELATÓRIO TÉCNICO DE ENGENHARIA: ARQUITETURA, INTEGRAÇÃO E CORREÇÃO DE FALHAS
**Projeto:** ALRI_Un1ca — Port One UI 9.0 (Android 17 / SDK 37)  
**Dispositivo Alvo:** Samsung Galaxy S23 (`SM-S911B` / `dm1q` / Qualcomm Snapdragon 8 Gen 2)  
**Base de Firmware:** `S911BXXUAZZI8` (One UI 9 Beta 2)  
**Status Final:** ✅ **Boot 100% Funcional, Interface Gráfica Estável e Teclado Oficial Operante**

---

## 🏛️ 1. Origem dos Repositórios e o que foi aproveitado de cada um

A construção da árvore unificada **`ALRI_Un1ca`** realizou a fusão estratégica de três bases de código distintas:

```
                  ┌────────────────────────────────────────┐
                  │      salvogiangri/UN1CA (sixteen)      │
                  │   Arquitetura Base & Dynamic Partitions │
                  └───────────────────┬────────────────────┘
                                      │
        ┌─────────────────────────────┴─────────────────────────────┐
        ▼                                                           ▼
┌─────────────────────────────────┐               ┌─────────────────────────────────┐
│   Rezoss-Reza/UN1CA (sembilan)  │               │ cosinemyan/NPLROM_UN1CA (npl-9) │
│ One UI 9 Motor / S23U Framework │               │   Visual NPL & Wallpapers Pack  │
└───────────────┬─────────────────┘               └────────────────┬────────────────┘
                │                                                  │
                └─────────────────────┬────────────────────────────┘
                                      ▼
                      ┌───────────────────────────────┐
                      │    ALRI_Un1ca (alri-oneui9)   │
                      │   Port Estável para S23 Base  │
                      └───────────────────────────────┘
```

### A. Da UN1CA Oficial (`salvogiangri/UN1CA:sixteen`)
* **Pipeline de Build Modular:** Scripts de orquestração (`buildenv.sh`, `make_rom.sh`, `scripts/build_fs_image.sh`), descompactação com `simg2img`, geração de imagens em `erofs` e compressão por blocos com `brotli`.
* **Mapeamento de Hardware do S23 (`target/dm1q/config.sh`):** Estrutura física das partições dinâmicas do Snapdragon 8 Gen 2 (`sm8550`).
* **Módulos Centrais de Customização:**
  * `deknox`: Remoção de serviços intrusivos e telemetrias redundantes.
  * `knoxpatch`: Emulação das APIs de integridade do Knox.
  * `games-fps-unlocker`: Injeção de `GamesPropsUtils` para desbloqueio de taxa máxima de quadros (120 FPS).
  * `pif`: Injeção de propriedades para Play Integrity.

### B. Do Rezoss-Reza (`Rezoss-Reza/UN1CA:sembilan`)
Base pioneira da One UI 9 portada para o Snapdragon 8 Gen 2 no S23 Ultra:
* **Bypass de Assinatura de 4 Certificados (`services.jar`):** Patch no `PackageManagerService` permitindo executar APKs customizados de sistema no Android 17 sem bloqueio de inicialização.
* **HMA OSS (Hide My Applist):** Hooks integrados no `framework.jar` para privacidade de aplicativos no nível de chamadas IPC do Binder.
* **TEE-Simulator:** Módulo de simulação do Trusted Execution Environment contornando rejeições de hardware da TrustZone.
* **DFE Auto-Detection (Disable Force Encryption):** Lógica dinâmica que checa o estado de criptografia do `/data` durante o flashing.
* **Recursos do Changelog de 17/10:** Novos controles no modo de Economia Máxima de Bateria e unificação de fontes customizadas no Lockscreen e Always On Display (AOD).

### C. Da NPL ROM (`cosinemyan/NPLROM_UN1CA:npl-oneui9`)
* **Identidade Visual e Temas:** Módulo `cosine` contendo o pacote de papéis de parede estendidos da região de Sagarmatha (`wallpaper-res.apk`).
* **Desbloqueio do ThemeCenter:** Script `inject_trial_gate.py` permitindo uso ilimitado de temas de teste da Galaxy Store.
* **Deduplicação de Firmware:** Otimizações no `download_fw.sh` para evitar downloads redundantes em disco.

---

## ✂️ 2. O que foi Cortado / Modificado e Justificativa Técnica

Para transformar uma base instável e pensada unicamente para o S23 Ultra em uma ROM sólida para o S23 base (`dm1q`), foram realizadas as seguintes intervenções cirúrgicas:

| Componente | Origem | Ação Realizada | Justificativa Técnica |
| :--- | :--- | :--- | :--- |
| **`libframebooster.so`** | Rezoss (`m3qxxx`) | **Removido e Restaurado do S23** | O binário do S26 Ultra exigia o símbolo `fmt::v11`. O sistema do S23 exporta `fmt::v12`. A discrepância causava crash fatal (`CANNOT LINK EXECUTABLE`) no `surfaceflinger`. |
| **Família `libmpp*.so`** | Rezoss (`m3qxxx`) | **Removida e Restaurada do S23** | A implementação do S26 Ultra alterou a assinatura de `renderAndReleaseOutputBuffer` no `MppWrapper`, gerando `UnsatisfiedLinkError` na `libmediasndk.so` do `SystemUI`. |
| **`HoneyBoard.apk` (Teclado)** | Rezoss | **Removidos Patches de Smali e Restaurado Original** | Os patches para o "Now Nudge" quebravam a assinatura da Samsung (`CN=Samsung Cert`). Em modo de bloqueio FRP, o Android 17 rejeita teclados assinados com Testkey. |
| **`DYN_RESOLUTION_CONTROL`** | Rezoss / S23U | **Desativado (`false`)** | O painel do S23 é FHD+ (1080x2340) físico. Forçar controle de resolução dinâmica (exclusivo de telas WQHD+) causava falha de alocação de buffer gráfico. |
| **Modo de Display (`HFR_MODE`)** | Config dm1q | **Alterado de 3 para 2** | O S23 base utiliza modo adaptativo de 24Hz a 120Hz (Modo 2). O modo 3 é calibrado para painéis LTPO 1Hz–120Hz do S23 Ultra. |
| **Tamanho da Partição Super** | Config dm1q | **Ajustado para `12266242048` B** | Alinhado com a capacidade exata dos blocos de hardware UFS do S23 normal (`dm1q`). |
| **`KnoxGuard.apk` & `kgclient`** | Upstream UN1CA | **Preservado Intacto** | A TrustZone (`tz_kg`) trava o aparelho em tela azul irreversível caso o pacote do KnoxGuard não seja encontrado no `/system/priv-app/`. |
| **Partições de Kernel no Zip** | Upstream / NPL | **Removido Flash de Kernel** | O `updater-script` foi configurado para tocar apenas nas partições lógicas do `super`, mantendo o kernel oficial (`boot`, `init_boot`, `dtbo`, `vendor_boot`) intacto contra rejeições de AVB. |
| **Downloads de Galaxy Store** | Módulos Rezoss | **Criado Fallback Seguro** | O `customize.sh` abortava o build quando a Galaxy Store não retornava URL para SDK 37 (`KmxService`, `GameTools`, `SBrowser`). Agora há fallback automático para as versões do firmware. |
| **`file_context-system_ext`** | Módulos Rezoss | **Rótulo SELinux Corrigido** | Uma linha truncada sem contexto para `MoseyApp.apk` impedia o `mkfs.erofs` de compilar a partição `system_ext`. Contexto padrão aplicado. |

---

## 💥 3. Histórico e Diagnóstico das Falhas Solucionadas

### 🔴 Falha 1: SurfaceFlinger Linker Abort (Bootloop para TWRP)
* **Sintoma:** O aparelho vibrava, iniciava o carregamento do kernel e reiniciava abruptamente para o Recovery após 10 segundos.
* **Evidência no Log (`/cache/recovery/rescueparty_log`):**
  ```text
  CANNOT LINK EXECUTABLE "/system/bin/surfaceflinger": 
  cannot locate symbol "_ZN3fmt3v117vformatENS0_17basic_string_viewIcEENS0_17basic_format_argsINS0_7contextEEE" 
  referenced by "/system/lib64/libframebooster.so"...
  ```
* **Diagnóstico:** O módulo experimental do Rezoss sobrescrevia a biblioteca de otimização de taxa de quadros por um blob extraído do S26 Ultra (`m3qxxx`). Esse blob foi compilado usando a versão 11 da biblioteca C++ `fmt`, enquanto a `libbase.so` do firmware oficial do S23 fornecia a versão 12. O dynamic linker do Linux abortava o SurfaceFlinger. Ao falhar 5 vezes consecutivas, o subsistema **RescueParty** forçava o reboot para o TWRP.
* **Solução:** Criada a função de guarda `_REZOSS_ADD_SYSTEM_LIB64` para impedir a sobrescrita de bibliotecas que já existam nativamente no firmware oficial do S23, restaurando a `libframebooster.so` original compatível com `fmt::v12`.

---

### 🔴 Falha 2: Crash em Loop do SystemUI / ImageWallpaper
* **Sintoma:** Display AMOLED piscando intermitentemente e tela inicial inacessível.
* **Evidência no Log (`/cache/recovery/rescueparty_log`):**
  ```text
  FATAL EXCEPTION: ImageWallpaper (Process: com.android.systemui)
  java.lang.UnsatisfiedLinkError: dlopen failed: 
  cannot locate symbol "_ZN7android10MppWrapper28renderAndReleaseOutputBufferEillRKNS_2spINS_8AMessageEEE" 
  referenced by "/system/lib64/libmediasndk.so"...
  ```
* **Diagnóstico:** As bibliotecas do ecossistema `libmpp*.so` do S26 Ultra foram compiladas com uma interface ABI diferente para o método `renderAndReleaseOutputBuffer`. O player de vídeo e papel de parede da One UI 9 do S23 tentava vincular essa função e abortava o processo `com.android.systemui`.
* **Solução:** Restauração integral dos binários de mídia nativos (`libmpp.so`, `libmpp_common.so`, `libmppclient.so`, `libmppfilter.so`, `vendor.samsung.hardware.media.mpp-V5-ndk.so`) extraídos do firmware do S23.

---

### 🔴 Falha 3: Incompatibilidade do F2FS no TWRP pós-Android 17
* **Sintoma:** O TWRP reportava `Failed to mount '/data' (Invalid argument)` e impedia a leitura do armazenamento interno via `/sdcard`.
* **Diagnóstico:** Ao iniciar o primeiro estágio de boot, o Android 17 atualiza as estruturas de metadados e o keyring de criptografia do sistema de arquivos F2FS em `/dev/block/sda41`. Como o kernel do TWRP 3.7.1 foi construído em versões anteriores, o superblock alterado pelo Android 17 não era montado sem um reset limpo.
* **Solução:** Executado o procedimento de **Format Data** (com confirmação por digitação de `yes`) no TWRP, recriando o superblock F2FS limpo e restabelecendo 220 GB livres para gravação.

---

### 🔴 Falha 4: Teclado Samsung Ausente na Configuração Inicial (FRP)
* **Sintoma:** O sistema concluía o boot normalmente até o Assistente de Configuração, mas ao tocar no campo de PIN ou login de conta Google, o teclado virtual não abria.
* **Diagnóstico:** O script do Rezoss aplicava 4 patches de Smali no `HoneyBoard.apk` para forçar o recurso experimental "Now Nudge", exigindo recompilação via Apktool e assinatura com chave genérica (`CN=Android`). Quando o aparelho está sob bloqueio de restauração (FRP), o Android 17 entra em modo `USER_SETUP_RESTRICTED` e **rejeita sumariamente qualquer InputMethodService que não possua a assinatura oficial da Samsung (`CN=Samsung Cert`)**.
* **Solução:** Remoção definitiva dos patches de Smali do HoneyBoard, exclusão dos arquivos temporários de recompilação e restauração do diretório oficial `system/app/HoneyBoard` (com `HoneyBoard.apk` original assinado pela Samsung e artefatos pré-otimizados em `oat/arm64/HoneyBoard.odex` e `.art`).

---

## 🚀 4. Estado Atual e Orientações para Uso

Com as correções aplicadas:
1. **O sistema operacional One UI 9.0 (Android 17)** executa de forma suave e nativa no Galaxy S23 (`dm1q`).
2. **O painel AMOLED** opera em 120Hz nativos, com brilho automático estável e sem flickering.
3. **O teclado da Samsung** responde com digitação em tempo real, suporte multilingue e integração total ao sistema.
4. **As partições críticas de bootloader e modem** estão 100% protegidas contra soft-brick.

*Documento gerado e registrado no repositório local `/home/alexsanderalri/Documentos/ALRI_Un1ca/relatorio.md`.*
