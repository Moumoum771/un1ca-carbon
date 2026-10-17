# 👑 UN1CA Carbon

<div align="center">

```
 ██╗   ██╗███╗   ██╗ ██╗ ██████╗ █████╗      ██████╗ █████╗ ██████╗ ██████╗  ██████╗ ███╗   ██╗
 ██║   ██║████╗  ██║███║██╔════╝██╔══██╗    ██╔════╝██╔══██╗██╔══██╗██╔══██╗██╔═══██╗████╗  ██║
 ██║   ██║██╔██╗ ██║╚██║██║     ███████║    ██║     ███████║██████╔╝██████╔╝██║   ██║██╔██╗ ██║
 ██║   ██║██║╚██╗██║ ██║██║     ██╔══██║    ██║     ██╔══██║██╔══██╗██╔══██╗██║   ██║██║╚██╗██║
 ╚██████╔╝██║ ╚████║ ██║╚██████╗██║  ██║    ╚██████╗██║  ██║██║  ██║██████╔╝╚██████╔╝██║ ╚████║
  ╚═════╝ ╚═╝  ╚═══╝ ╚═╝ ╚═════╝╚═╝  ╚═╝     ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝  ╚═════╝ ╚═╝  ╚═══╝
```

**Next-Generation One UI 9 Custom ROM for Samsung Galaxy S23 Series**  
*Release Version: **IX** (One UI 9.0 • Android 17 • SDK 37)*  
*Engineered, Ported & Stabilized for Galaxy S23 Base (`SM-S911B` / `dm1q`)*

---

[![Android](https://img.shields.io/badge/Android-17-green.svg)](https://developer.android.com)
[![OneUI](https://img.shields.io/badge/OneUI-9.0--Beta2-blue.svg)](https://www.samsung.com)
[![Device](https://img.shields.io/badge/Target-Galaxy%20S23-orange.svg)](https://github.com/alexsanderalri)
[![DFE](https://img.shields.io/badge/Encryption-DFE%20Enabled-success.svg)](https://github.com/alexsanderalri)
[![License](https://img.shields.io/badge/License-GPLv3-brightgreen.svg)](LICENSE)

</div>

---

## 📖 Sobre o Projeto

O **UN1CA Carbon** é uma distribuição customizada e estabilizada da **One UI 9 (Android 17 / SDK 37)** projetada especificamente para trazer a nova era de software da Samsung para o **Galaxy S23 Base (`SM-S911B`)**.

Enquanto os ports upstream iniciais foram desenvolvidos exclusivamente para o S23 Ultra, o **UN1CA Carbon** resolveu os conflitos de hardware, calibrou a taxa de quadros nativa (120Hz), estabilizou o subsistema gráfico (SurfaceFlinger) e implementou o **DFE (Disable Force Encryption)** para permitir recuperação e backups completos via TWRP Recovery.

---

## 🚀 CATÁLOGO COMPLETO DE RECURSOS E FUNCIONALIDADES (TODAS)

Abaixo está o registro exato e exaustivo de **todas as modificações, módulos e funcionalidades** presentes nesta ROM:

### ⚙️ 1. Menu Nativo "Configurações UN1CA Carbon IX" (`SecSettings.apk`)
Acessível diretamente no topo do menu principal de Configurações da Samsung:
* **Allow Application Downgrade:** Permite instalar versões anteriores de qualquer aplicativo sem desinstalar o atual (sem perda de dados).
* **Disable Signature Verification (CorePatch):** Ignora a checagem de assinatura de APKs em nível de sistema, permitindo instalar aplicativos modificados livremente.
* **Disable Low Target SDK Block:** Permite instalar e rodar aplicativos antigos projetados para versões legadas do Android (Android 5, 6, 7).
* **Disable ASKS (Advanced Samsung Knox Security):** Desativa o bloqueador de aplicativos em segundo plano da Samsung.
* **Secure Screenshot:** Permite capturar telas e gravar vídeos dentro de aplicativos com proteção de segurança (bancos, streaming, conversas privadas).
* **Disable Capture Detection:** Impede que aplicativos detectem que você tirou print ou está gravando a tela.
* **Mock Location (Localização Simulada):** Mascara atributos de localização sem alertar serviços restritos.
* **Unlimited Google Photos Storage:** Spoofing automático para backup original ilimitado no Google Fotos em qualidade original.
* **Unlock Higher FPS in Games (GamesProps):** Mascara o aparelho para desbloquear 90 FPS e 120 FPS em jogos competitivos.
* **Spoof as S26 Ultra on Galaxy Store:** Força o aplicativo da Galaxy Store a listar aplicativos e recursos de inteligência artificial de última geração.
* **Galaxy Themes Infinite Trial:** Remove a expiração de 10 minutos de temas baixados da Galaxy Store, tornando-os permanentes.
* **Now Nudge (Galaxy AI Replies):** Motor experimental de respostas rápidas contextuais sugeridas por inteligência artificial.
* **SCPM Allowlist Bypass:** Permite que clientes locais ignorem políticas regionais e de CSC para áudio e inteligência artificial.

---

### 🎨 2. Interface, Display e Customizações Visuais
* **Display 120Hz Nativo sem Flickering:** Calibrado especificamente para a tela plana FHD+ (1080x2340) do S23 base (`HFR Mode 2`).
* **Force Peak Refresh Rate:** Opção de travar a tela em 120Hz estravagantes constantes.
* **Live Blur Effect (Native Blur):** Ativação de desfoque gaussiano em tempo real em todas as superfícies da One UI.
* **Launcher Animation Type:** Seletor de curvas e velocidades de animação da tela inicial da Samsung.
* **Vulkan Graphics Engine:** Força o pipeline de renderização da interface gráfica através da API Vulkan para máxima fluidez.
* **Outdoor Mode (Modo Externo):** Adiciona nas configurações a opção de brilho extremo sob a luz direta do sol.
* **UN1CA Font Selector:** Seletor de fontes customizadas integrado nativamente no menu *Visor > Fonte*, com suporte a fontes Monotype e SamsungSans.
* **Sincronização de Fontes no AOD & Lockscreen:** A fonte customizada escolhida é aplicada automaticamente no relógio da tela de bloqueio e Always On Display.
* **Galeria de Papéis de Parede Exclusivos Sagarmatha (NPL):** Injeção de wallpapers fotográficos em ultra-alta resolução das montanhas do Nepal diretamente no `wallpaper-res.apk`.
* **Galaxy S25 Paradigm Wallpapers:** Inclusão dos papéis de parede animados originais da linha Galaxy S25.

---

### 🛡️ 3. Privacidade, Segurança e Anti-Detecção
* **HMA OSS (Hide My Applist) Integrado:** Proteção de privacidade no nível do Binder IPC do framework do sistema. Impede que apps espiem os outros aplicativos instalados no celular.
* **Hardware Attestation Spoof (Keybox):** Permite importar chaves de Keybox customizadas para passar na validação de hardware do Google.
* **Play Integrity Fix (PIF) Embutido:** Injeção e atualização dinâmica de propriedades para garantir certificação máxima na Play Store e funcionamento de carteiras digitais (Google Wallet).
* **Hide Developer Status:** Esconde o estado de Opções do Desenvolvedor ativo de aplicativos bancários.
* **4 Digits Auto-Confirm PIN:** Permite desbloquear o celular digitando o PIN de 4 dígitos sem precisar tocar no botão "OK".
* **Menu de Energia Avançado (Power Menu):** Adiciona atalhos nativos para reiniciar em modo **Recovery (TWRP)**, **Download** e **Fastboot** diretamente ao segurar o botão liga/desliga.

---

### 📞 4. Telefonia, Áudio e Comunicação
* **Gravação Nativa de Chamadas (Call Recording):** Gravação de áudio em alta definição embutida no discador oficial da Samsung (`ril.call.recording=true`), sem aviso sonoro de gravação.
* **Dual Messenger Universal (`daagent`):** Habilita o recurso de clonar aplicativos para **todos** os apps instalados no celular (não apenas redes sociais pré-definidas).
* **Suporte Multi-Usuário (Multi-user):** Ativa a alternância de múltiplos perfis e contas de convidado no Android.
* **Custom Boot Sound & Call Sounds:** Efeitos sonoros customizados de inicialização e sons de chamada nostálgicos da Samsung (2018 / 2024 / ACH Galaxy Bells).

---

### 🔋 5. Bateria e Eficiência Energética
* **Battery Health & Cycle Count:** Exibe a saúde percentual real da bateria e o contador exato de ciclos de recarga nas Configurações do Sistema em todas as regiões.
* **Stingy Presets (Modo Econômico Extremo):** Ajustes finos no modo de Economia Máxima que permitem desativar processos pesados em segundo plano da Google Play Services e operadoras.

---

### 🛠️ 6. Engenharia Exclusiva do Kernel e Sistema (Camada Carbon)
* **DFE Nativo (Disable Force Encryption):** O `vendor/etc/fstab.qcom` foi configurado como `encryptable`, impedindo que o Android 17 bloqueie a partição `/data`. Liberdade total de montagem e leitura no TWRP.
* **SurfaceFlinger Blindado:** Vinculação dinâmica corrigida contra a biblioteca `libbase.so` e `fmt::v12` nativas do hardware Snapdragon 8 Gen 2, eliminando quedas de display.
* **Teclado Samsung Oficial (`HoneyBoard`):** Assinatura oficial `CN=Samsung Cert` e otimização `oat` preservadas, garantindo funcionamento instantâneo mesmo sob telas de bloqueio FRP.
* **Proteção contra Knox Lockout:** Preservação estrita do `KnoxGuard.apk` e permissões do `kgclient`, impedindo o travamento permanente de hardware da TrustZone (`tz_kg`).
* **Proteção de AVB (Android Verified Boot):** O instalador da ROM atualiza exclusivamente as partições dinâmicas do `super` (`system`, `vendor`, `product`, `system_ext`, `odm`), deixando o kernel e o modem intocados contra rejeições de bootloader.
* **Single System Image (QSSI) em EROFS:** Partições de sistema geradas em `erofs` de alta performance com compressão transparente, economizando memória RAM e espaço em disco.

---

## 🏛️ Estrutura Arquitetural Modular

```text
Un1ca_Carbon/
│
├── 📁 core_unica/           # Fundação oficial UN1CA (Salvo Giangri): scripts, compilador, EROFS
├── 📁 vendors_rezoss/       # Motor One UI 9 Framework (Rezoss): TEE-Sim, HMA OSS, 4-Certs Bypass
├── 📁 vendors_npl/          # Módulos Visuais NPL (Cosine Team): Wallpapers Sagarmatha, ThemeCenter Trial
│
├── 📁 carbon_ours/          # DESENVOLVIMENTO & ENGENHARIA EXCLUSIVA CARBON (AlexsAR / ALRI)
│   ├── target_dm1q/         # Calibração de Hardware S23: Painel FHD+ (1080x2340), HFR Mode 2, Partição Super
│   ├── dfe_engine/          # Fstab com DFE ativo (encryptable) — TWRP com acesso livre ao /data
│   ├── branding_settings/   # SecSettings.apk com menu nativo "Configurações UN1CA Carbon IX"
│   └── security_keys/       # Chaves e certificados oficiais de assinatura
│
└── 📁 docs/                 # Relatórios de Engenharia e Histórico de Correções
    └── relatorio.md         # Documentação forense detalhada de cada bootloop solucionado
```

---

## 👥 Créditos e Reconhecimentos

* **Mantenedor Principal & Lead Developer:** **AlexsAR** ([@alexsanderalri](https://github.com/alexsanderalri))
* **Port, Estabilização Gráfica e Engenharia S23:** **AlexsanderALRI** (ALRI GROUP)
* **One UI 9 Framework Engine:** **@Rezoss-Reza**
* **UN1CA Core Architecture:** **Salvo Giangri** ([@salvogiangri](https://github.com/salvogiangri))
* **Visuals & Wallpapers:** **@cosinemyan** (NPL ROM Team)

---

## 📄 Licença

Este projeto é distribuído sob a licença **GNU General Public License v3.0 (GPL-3.0)**.
