# [CARBON ENGINE] — TECHNICAL SPECIFICATION & SOFTWARE ARCHITECTURE CONTRACT
**Canonical Document:** `specs.md`  
**Projeto:** Carbon Engine (Next-Generation Modular ROM Build & Pipeline Orchestrator)  
**Organização:** ALRI Development (ALRIGROUP)  
**Architecture & Systems Engineering:** AlexsAR (@alexsanderalri) & Equipe de Plataforma  
**Classification:** Industrial Engineering Specification & Architecture Contract  
**Language Standard:** C++20 (ISO/IEC 14882:2020) with POSIX.1-2017 Extensions and Linux Syscalls  
**Specification Version:** 1.0.0-PROD  
**Date:** October 2026  

---

## 1. SCOPE, OBJECTIVES & NON-FUNCTIONAL REQUIREMENTS (SLOS)

### 1.1 System Mission
The **Carbon Engine** is a native mass compilation, dependency management, and input-addressed cache orchestrator (*Input-Addressed DAG Engine*), designed to replace legacy sequential Bash scripts. It serves as the core build system for **UN1CA Carbon** and any Android distribution based on ports (One UI, AOSP, PixelOS).

### 1.2 Non-Functional Requirements & Quantitative Targets (Service Level Objectives - SLOs)
* **SLO-1 (Cold Build Latency):** Full clean build (without prior cache) of all 7 logical partitions on 28-thread hardware: $\le 18 \text{ minutos}$.
* **SLO-2 (Incremental Build Latency - Single File/String):** Total rebuild duration after editing a single XML string or smali file: $\le 45 \text{ segundos}$ (compared to ~25 min on legacy scripts).
* **SLO-3 (Stat-Cache Traversal):** Full scan of 250,000 filesystem nodes for `mtime`/`inode` validation: $\le 15 \text{ milissegundos}$.
* **SLO-4 (Throughput de Hashing SIMD):** In-memory SHA-256 compute throughput: $\ge 1.8 \text{ GB/s}$ per core using hardware acceleration (Intel SHA Extensions / ARMv8 Crypto Extensions).
* **SLO-5 (Confiabilidade de Transação):** $0\%$ cache corruption in abrupt termination scenarios (`SIGINT`, `SIGKILL`, `OOM Killer`).
* **SLO-6 (Consumo de Memória do Orquestrador):** Own RAM memory overhead of the `carbon-engine` binary: $\le 64 \text{ MB}$ (excluding spawned child processes like Java/Apktool).

### 1.3 Target Platform
* **Host OS:** Linux (Kernel $\ge 5.10$, suporte nativo a `epoll`, `pidfd`, `renameat2`, `fallocate`).
* **Arquiteturas Suportadas:** x86_64 (Intel Core 12ª+ ger, AMD Zen 3+), aarch64 (ARM64 Neoverse, Apple Silicon em Linux).
* **Toolchain de Compilação:** Clang $\ge 18.0$ ou GCC $\ge 14.0$ com `libc++` / `libstdc++` com suporte completo a C++20 Ranges, Concepts, Coroutines e `<filesystem>`.

---

## 2. TECHNOLOGY RATIONALE & DEPENDENCY MATRIX

### 2.1 Why C++20?
A escolha de C++20 em relação a linguagens interpretadas (Python, Node.js) ou alternativas modernas (Rust, Go) baseia-se em quatro invariantes rígidas do projeto:
1. **Zero-Overhead Syscall Interface:** Acesso direto sem FFI a chamadas de sistema críticas do Linux (`statx(2)`, `renameat2(2)`, `clone3(2)`, `pidfd_open(2)`).
2. **Controle Estrito de Memória em Cargas Massivas:** Ausência de Garbage Collector, evitando pausas de GC durante a indexação de centenas de milhares de arquivos.
3. **Interoperabilidade Nativa com a Toolchain Android:** O ecossistema AOSP/UN1CA já depende de binários em C/C++ nativos (`aapt2`, `simg2img`, `mkfs.erofs`, `brotli`). C++20 permite linkagem estática direta contra essas bibliotecas quando necessário.
4. **Alocação Altamente Concorrente:** Integração transparente com alocadores de alta performance (`mimalloc` ou `jemalloc`) para eliminar contenção de heap entre 28+ threads de trabalho.

### 2.2 Canonical External Dependencies

| Library | Minimum Version | Purpose in Carbon Engine | Technical Rationale |
| :--- | :--- | :--- | :--- |
| **SQLite3** | `3.46.0+` | Banco de dados local do Ledger e Cache de Metadados | Modo WAL (Write-Ahead Logging), Mmap I/O, transações ACID com atomicidade rigorosa. |
| **OpenSSL / libcrypto** | `3.3.0+` | Hashing criptográfico SHA-256 e parsing X.509/PKCS#8 | Implementação em assembly com aceleração por hardware (AVX-512 / ARM Neon). |
| **xxHash** | `0.8.2+` | Hashing ultra-rápido de 64/128 bits para o Stat-Cache | Velocidade de $\approx 15 \text{ GB/s}$ para hashing de identificadores internos e nomes de caminhos. |
| **nlohmann/json** | `3.11.3+` | Leitura e gravação do `build_manifest.json` | Parsing seguro, moderno e sem leaks para serialização de relatórios forenses. |
| **CLI11** | `2.4.2+` | Parser de Linha de Comando (CLI) | Header-only, suporte estrito a subcomandos, validação tipada de parâmetros. |
| **spdlog + libfmt** | `1.14.0+` | Logging assíncrono estruturado de alta velocidade | Zero-allocation no loop principal; gravação concorrente em console e arquivo de log. |
| **mimalloc** | `2.1.7+` | Alocador de Memória Global | Substituição da `malloc` padrão da glibc; reduz fragmentação e acelera alocações concorrentes em $30\%$. |

---

## 3. BUILD SYSTEM SPECIFICATION (CMAKE & TOOLCHAIN)

O Carbon Engine é compilado com linkagem estática máxima de dependências de terceiros para garantir que o binário resultante seja distribuível de forma portátil entre distribuições Linux (Fedora, Ubuntu, Arch, Debian) sem problemas de glibc ou so-hell.

### 3.1 `CMakeLists.txt` Oficial

```cmake
cmake_minimum_required(VERSION 3.25)
project(CarbonEngine 
    VERSION 1.0.0 
    LANGUAGES C CXX
    DESCRIPTION "High-Performance Next-Generation Android ROM Build Engine"
    HOMEPAGE_URL "https://github.com/alrigroup/un1ca-carbon"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Otimizações de Compilação Estritas
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wnon-virtual-dtor
        -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual
        -Wformat=2 -Wimplicit-fallthrough -pipe
    )
    
    # Flags de Release
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto=thin -march=native -fno-math-errno")
    
    # Flags de Sanitizer para Debug
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined")
endif()

# Inclusões Públicas
include_directories(include)

# Descoberta de Pacotes do Sistema
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# SQLite3 com otimizações de concorrência
find_package(SQLite3 REQUIRED)

# Configuração de Alocador de Alta Performance
find_package(mimalloc QUIET)
if(mimalloc_FOUND)
    message(STATUS "Linkando contra mimalloc para alocação concorrente de alta performance")
    set(EXTRA_ALLOCATOR_LIBS mimalloc)
    add_compile_definitions(CARBON_USE_MIMALLOC=1)
else()
    message(STATUS "mimalloc não encontrado; utilizando alocador padrão do sistema operacional")
endif()

# Coleta de Fontes
file(GLOB_RECURSE CARBON_SOURCES 
    "src/common/*.cpp"
    "src/fingerprint/*.cpp"
    "src/dag/*.cpp"
    "src/scheduler/*.cpp"
    "src/ledger/*.cpp"
    "src/supervisor/*.cpp"
    "src/security/*.cpp"
    "src/fs/*.cpp"
    "src/cli/*.cpp"
)

# Alvo Executável Principal
add_executable(carbon-engine src/main.cpp ${CARBON_SOURCES})

target_link_libraries(carbon-engine PRIVATE
    OpenSSL::Crypto
    SQLite::SQLite3
    Threads::Threads
    ${EXTRA_ALLOCATOR_LIBS}
    stdc++fs
)

target_include_directories(carbon-engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Diretiva de Instalação do Binário
install(TARGETS carbon-engine DESTINATION bin)
```

---

## 4. LEDGER DATA SCHEMA (SQLITE SCHEMA DDL)

O estado do cache e o histórico de execuções são persistidos em um arquivo SQLite localizado em `out/.carbon_cache/ledger.db`.

### 4.1 Configuração de Pragmas Obrigatórios
Toda conexão com o `ledger.db` deve obrigatoriamente executar antes de qualquer query:
```sql
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;
PRAGMA foreign_keys = ON;
PRAGMA mmap_size = 268435456; -- 256 MB de memory-mapped I/O
PRAGMA temp_store = MEMORY;
PRAGMA cache_size = -64000;   -- 64 MB de cache de páginas SQLite
PRAGMA busy_timeout = 5000;   -- 5 segundos de espera antes de SQLITE_BUSY
```

### 4.2 Esquema Relacional de Tabelas (DDL)

```sql
-- Versão da migração do banco de dados
CREATE TABLE IF NOT EXISTS schema_version (
    version_id INTEGER PRIMARY KEY,
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
);

-- Tabela de Componentes Registrados na Árvore de Build
CREATE TABLE IF NOT EXISTS build_nodes (
    node_id TEXT PRIMARY KEY,               -- Ex: "apk:SecSettings", "fs:system", "jar:services"
    layer_level INTEGER NOT NULL,            -- 0 (Base), 1 (Core JAR), 2 (Apps), 3 (Media), 4 (FS)
    description TEXT NOT NULL,              -- Rótulo legível humano
    toolchain_signature TEXT NOT NULL       -- Hash da ferramenta compiladora (ex: Apktool v2.10 SHA)
);

-- Tabela de Dependências do Grafo (Arestas Direcionadas)
CREATE TABLE IF NOT EXISTS node_dependencies (
    consumer_node_id TEXT NOT NULL,         -- Nó dependente (ex: "apk:SecSettings")
    provider_node_id TEXT NOT NULL,         -- Nó provedor (ex: "apk:framework-res")
    is_hard_dependency BOOLEAN NOT NULL DEFAULT 1,
    PRIMARY KEY (consumer_node_id, provider_node_id),
    FOREIGN KEY (consumer_node_id) REFERENCES build_nodes(node_id) ON DELETE CASCADE,
    FOREIGN KEY (provider_node_id) REFERENCES build_nodes(node_id) ON DELETE CASCADE
);

-- Tabela Histórica de Execuções e Snapshots de Cache
CREATE TABLE IF NOT EXISTS execution_cache (
    node_id TEXT NOT NULL,
    input_fingerprint TEXT NOT NULL,        -- SHA-256 composto das entradas
    env_fingerprint TEXT NOT NULL,          -- SHA-256 das flags e variáveis de ambiente
    artifact_path TEXT NOT NULL,            -- Caminho relativo no work_dir (ex: "system/priv-app/SecSettings/SecSettings.apk")
    artifact_size_bytes INTEGER NOT NULL,
    artifact_sha256 TEXT NOT NULL,          -- Hash do arquivo gerado para verificação de integridade
    execution_duration_ms INTEGER NOT NULL, -- Tempo que levou para compilar
    last_build_timestamp INTEGER NOT NULL,  -- Timestamp Unix Epoch
    is_valid BOOLEAN NOT NULL DEFAULT 1,
    PRIMARY KEY (node_id, input_fingerprint),
    FOREIGN KEY (node_id) REFERENCES build_nodes(node_id) ON DELETE CASCADE
);

-- Tabela de Flags de Ambiente e Parâmetros da CLI
CREATE TABLE IF NOT EXISTS build_runs (
    run_id INTEGER PRIMARY KEY AUTOINCREMENT,
    start_timestamp INTEGER NOT NULL,
    finish_timestamp INTEGER,
    target_device TEXT NOT NULL,            -- Ex: "dm1q"
    target_pda TEXT NOT NULL,               -- Ex: "S911BXXUAZZI8"
    cli_flags_json TEXT NOT NULL,           -- Dump JSON completo dos argumentos passados
    total_nodes_processed INTEGER DEFAULT 0,
    nodes_cached INTEGER DEFAULT 0,
    nodes_rebuilt INTEGER DEFAULT 0,
    total_time_saved_ms INTEGER DEFAULT 0,
    exit_code INTEGER DEFAULT -1
);

-- Índices de Alta Performance
CREATE INDEX IF NOT EXISTS idx_exec_lookup 
    ON execution_cache(node_id, input_fingerprint, is_valid);

CREATE INDEX IF NOT EXISTS idx_deps_consumer 
    ON node_dependencies(consumer_node_id);
```

---

## 5. COMPOSITE FINGERPRINTING ALGORITHM & SPECIFICATION

### 5.1 A Fórmula Matemática do Hash Composto
O Carbon Engine define a unicidade de uma tarefa de compilação através de uma assinatura imutável $H_{composite}$:

$$H_{composite} = \mathcal{H}\Big( H_{tree} \parallel H_{patches} \parallel H_{script} \parallel H_{env} \parallel H_{toolchain} \Big)$$

Onde:
* $\mathcal{H}$ é a função criptográfica $\text{SHA-256}$.
* $\parallel$ denota concatenação binária sem delimitadores ambíguos.
* $H_{tree}$ é o hash canônico ordenado do diretório do componente.
* $H_{patches}$ é o hash SHA-256 sequencial dos arquivos de patch smali/diff vinculados.
* $H_{script}$ é o hash do script de customização (ex: `customize.sh`).
* $H_{env}$ é o hash JSON canônico (chaves ordenadas lexicograficamente) das variáveis de ambiente e flags CLI.
* $H_{toolchain}$ é o hash SHA-256 do binário/JAR do compilador executado (`apktool.jar`, `aapt2`, `mkfs.erofs`).

### 5.2 O Procedimento de Varredura Canônica de Diretórios (Canonical Tree Hashing)
Para garantir que dois sistemas de arquivos diferentes (ex: ext4 local vs Btrfs em container) produzam a mesma digital sobre os mesmos arquivos, a travessia de diretórios obedece rigorosamente à seguinte rotina em pseudocódigo algorítmico:

```text
Procedimento HashCanonicalTree(caminho_raiz):
  1. Inicializar buffer OpenSSL EVP_MD_CTX para SHA-256.
  2. Coletar todos os caminhos relativos de arquivos dentro de caminho_raiz.
  3. Filtrar itens que correspondam à lista de exclusão fixa:
     - .git/**
     - **/*.bak
     - **/*.orig
     - **/*.rej
     - **/*.tmp
     - **/.DS_Store
     - build/**
     - dist/**
  4. Ordenar a lista de caminhos relativos usando ordenação binária estrita (strcmp / LC_ALL=C).
  5. Para cada caminho_relativo na lista ordenada:
     a. Executar statx() para ler tamanho, modo de permissão e tipo de arquivo.
     b. Atualizar Hash com caminho_relativo.
     c. Atualizar Hash com modo (normalizado para 0755 ou 0644).
     d. Se for Link Simbólico:
        - Ler destino do link via readlinkat().
        - Atualizar Hash com a string do destino.
     e. Se for Arquivo Regular:
        - Abrir arquivo com O_RDONLY | O_NOATIME.
        - Ler arquivo em blocos de 64 KB (buffer alinhado).
        - Para cada bloco: EVP_DigestUpdate(ctx, buffer, bytes_lidos).
        - Fechar descritor de arquivo.
  6. Finalizar EVP_DigestFinal_ex(ctx, digest).
  7. Retornar hex_string(digest).
```

---

## 6. DIRECTED ACYCLIC GRAPH (DAG) ARCHITECTURE & SCHEDULER

### 6.1 Modelo de Dados dos Nós de Tarefa (`TaskNode`)
Cada etapa da build é representada em memória como uma estrutura isolada com contratos explícitos:

```cpp
namespace carbon::dag {

enum class NodeState : uint8_t {
    PENDING = 0,
    EVALUATING_CACHE,
    SKIPPED_CACHE_HIT,
    ENQUEUED_FOR_BUILD,
    RUNNING,
    COMPLETED_SUCCESS,
    FAILED_ABORT
};

enum class LayerLevel : uint8_t {
    LAYER_0_BASE_FIRMWARE = 0,
    LAYER_1_CORE_FRAMEWORK = 1,
    LAYER_2_SYSTEM_APPS    = 2,
    LAYER_3_MEDIA_BRANDING = 3,
    LAYER_4_FILESYSTEM_IMG = 4,
    LAYER_5_FLASHABLE_ZIP  = 5
};

struct ArtifactMetadata {
    std::string relative_path;
    uint32_t uid;
    uint32_t gid;
    uint32_t mode;
    std::string selinux_context;
    uint64_t size_bytes;
    std::string sha256_checksum;
};

class TaskNode {
public:
    std::string node_id;
    LayerLevel layer;
    std::string display_name;
    
    // Dependências diretas
    std::vector<std::string> upstream_dependencies;
    std::vector<std::string> downstream_dependents;
    
    // Contratos de entrada e saída
    std::vector<std::string> input_paths;
    std::vector<ArtifactMetadata> output_artifacts;
    std::unordered_map<std::string, std::string> environment_flags;
    
    // Estado de execução
    std::atomic<NodeState> state{NodeState::PENDING};
    std::string calculated_fingerprint;
    uint64_t execution_duration_ms{0};
    int exit_code{0};
    std::string error_message;
};

} // namespace carbon::dag
```

### 6.2 Resolução Topológica e Detecção de Ciclos
Antes do disparo de qualquer thread de compilação, o grafo passa por validação através do Algoritmo de Kahn:
1. Identificam-se todos os nós com grau de entrada zero ($\text{in-degree} = 0$).
2. Os nós são processados e removidos da fila, decrementando o grau de entrada de seus dependentes.
3. Se a lista ordenada final não contiver todos os nós registrados, existe uma **dependência circular** no projeto. O Carbon Engine aborta imediatamente, emitindo erro `E_CYCLIC_DEPENDENCY` e listando os nós causadores do ciclo.

### 6.3 O Mecanismo OOM Guard (Token Bucket de Concorrência JVM)
O `apktool` e o `aapt2` utilizam máquinas virtuais Java que alocam entre 1.5 GB e 4 GB de memória Heap cada. Em uma máquina com 28 threads, disparar 28 instâncias simultâneas de `apktool b` resultaria em consumo de $\approx 80 \text{ GB}$ de RAM, acionando o temido **Linux OOM Killer**.

Para impedir isso com determinismo, o Scheduler implementa um **Limitador de Carga por Tokens**:
* O motor calcula a memória livre real do sistema via `/proc/meminfo` (`MemAvailable`).
* Define uma cota máxima de memória permitida para instâncias concorrentes da JVM:
  $$\text{MaxTokens} = \min\left(\text{ThreadsDisponíveis}, \frac{\text{MemóriaDisponível} \times 0.70}{2048 \text{ MB}}\right)$$
* Um nó que vai rodar `apktool b` só inicia se conseguir retirar 1 token de execução pesado.
* Nós leves de I/O puro (cópia de prebuilts, patches smali rápidos com sed/python, metadados) utilizam a fila normal de threads sem restrição de token.

---

## 7. PROCESS SUPERVISOR SPECIFICATION (SUBPROCESS & I/O ENGINE)

### 7.1 Criação de Processos com Isolamento de Grupo
O `ProcessSupervisor` não utiliza `system(3)` ou `popen(3)`. Ele executa processos através de `posix_spawn` / `fork` configurado estritamente com `setpgid(0, 0)`:
* Cada processo filho recebe um ID de Grupo de Processos (`PGID`) próprio.
* Se o processo travar ou sofrer timeout, a interrupção é enviada para todo o grupo de processos (`kill(-pgid, SIGTERM)`), impedindo que processos zumbis ou ferramentas filhas do Java fiquem presas em segundo plano.

### 7.2 I/O Assíncrono Não-Bloqueante com `epoll(7)`
* O supervisor cria pipes unidirecionais para `stdout` e `stderr` do subprocesso com a flag `O_NONBLOCK`.
* Um descritor `epoll` monitora a chegada de dados em tempo real.
* Os dados lidos são transmitidos imediatamente para o arquivo de log central do Carbon Engine e para a janela de log do terminal em formato estruturado.

### 7.3 Transações Atômicas de Saída (`AtomicFileStaging`)
Para garantir integridade física no disco:
1. O comando é instruído a gerar a saída em um diretório temporário isolado:
   `out/.staging/<node_id>/<filename>.tmp`
2. O supervisor verifica:
   - Código de retorno do processo $== 0$.
   - Tamanho do arquivo $> 0$ bytes.
   - Assinatura mágica do arquivo (ex: PKzip magic `50 4B 03 04` para APKs e ZIPs).
3. Efetua a promoção atômica via syscall Linux:
   ```c
   renameat2(AT_FDCWD, staging_path, AT_FDCWD, destination_path, 0);
   ```
4. Se o processo falhar ou for interrompido, o diretório de staging é expurgado completamente. O destino final nunca recebe arquivos incompletos.

---

## 8. SECURITY ARCHITECTURE & KEY MANAGEMENT

### 8.1 Padrão de Assinatura Criptográfica
O Carbon Engine gerencia três perfis de assinatura do ecossistema Android:
* **`platform`:** Privilégios máximos de sistema operacional (`UID 0` e `UID 1000`). Utilizada para assinar `SecSettings`, `SystemUI`, `framework-res`, `SettingsProvider`.
* **`media`:** Privilégios de galeria e provedores de mídia do sistema.
* **`releasekey` / `ota`:** Assinatura do pacote flashable ZIP final consumido pelo TWRP / Recovery.

### 8.2 Matriz de Resolução de Chaves
O motor adota o seguinte algoritmo de decisão estrito:

```text
┌────────────────────────────────────────────────────────┐
│              Verificar se existe Flag CLI              │
│                 --keys=<caminho_custom>                │
└───────────────────────────┬────────────────────────────┘
                            │
               ┌────────────┴────────────┐
               ▼                         ▼
            [Existe]                [Não Existe]
     Carregar chaves de         Buscar em:
     <caminho_custom>           security/keys/platform.pk8
                                         │
                            ┌────────────┴────────────┐
                            ▼                         ▼
                         [Existe]                [Não Existe]
                  Carregar chaves de          Fallback Seguro:
                  security/keys/              carbon_ours/security_keys/aosp_*
                  ro.build.tags =             ro.build.tags = "test-keys"
                  "release-keys"              Emitir Warning Visual no Terminal
```

### 8.3 Injeção Automática de Assinatura no `services.jar`
No Android moderno da Samsung, o `PackageManagerService` valida se os apps do sistema possuem a mesma assinatura da plataforma antes de conceder acesso privilegiado.
O Carbon Engine automatiza a injeção da chave do desenvolvedor:
1. Lê o certificado `platform.x509.pem` ativo.
2. Extrai a sequência binária DER do certificado:
   ```bash
   CERT_DER_HEX="$(openssl x509 -in platform.x509.pem -outform DER | xxd -p -c 0)"
   ```
3. Localiza no código Smali do `services.jar` o método inicializador `InstallPackageHelper`:
   ```smali
   # Alvo de substituição
   const-string v0, "CONFIG_CUSTOM_PLATFORM_SIGNATURE"
   ```
4. Substitui a string pelo valor `CERT_DER_HEX` real da chave do desenvolvedor.
5. **Resultado:** Qualquer aplicativo modificado ou recompilado com a chave do mantenedor é aceito como legítimo pelo Android no primeiro boot.

---

## 9. SYSTEM IMAGE COMPOSER (FS COMPOSER: EROFS & BROTLI)

### 9.1 Validação de Inalterabilidade de Partições
Uma partição inteira (ex: `vendor.img`) pesa entre 2 GB e 3 GB. Recompilar e comprimir uma partição inalterada custa de **5 a 7 minutos de CPU e disco**.
O `FsComposer` do Carbon Engine resolve isso na raiz:
* Toda a árvore de arquivos de `/system`, `/vendor`, `/product`, etc., possui uma **Digital de Partição Agregada**:
  $$H_{partition} = \mathcal{H}\left(\sum_{f \in \text{PartitionFiles}} \text{SHA256}(f) + \text{fs\_config} + \text{file\_contexts}\right)$$
* Se $H_{partition}$ for idêntico à compilação anterior:
  - **Pula** o `mkfs.erofs`.
  - **Pula** a conversão para `.new.dat`.
  - **Pula** a compressão multi-thread do Brotli (`.new.dat.br`).
  - Reaproveita os blocos de transferência pré-existentes. Tempo gasto na partição: **0.00 segundos**.

### 9.2 Otimização Multi-Core de Compressão Brotli
Quando uma partição realmente precisa ser recompilada:
* O Carbon Engine implementa compressão concorrente por blocos (Chunked Multi-Stream Compression).
* O arquivo `.new.dat` é fatiado em blocos lógicos proporcionais ao número de threads disponíveis (ex: 28 chunks no hardware atual).
* Cada thread comprime seu chunk com nível de qualidade `brotli -q 6` (equilíbrio perfeito entre taxa de compressão e velocidade de descompressão no TWRP).
* Os streams comprimidos são linkados sequencialmente no arquivo `.new.dat.br` final.

---

## 10. C++20 PROGRAMMING INTERFACES (HEADER SPECIFICATIONS)

Abaixo estão os contratos formais de software que a equipe de engenharia deve implementar.

### 10.1 `include/carbon/fingerprint.hpp`
```cpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "carbon/error.hpp"

namespace carbon::fingerprint {

struct StatMetadata {
    uint64_t inode;
    uint64_t size_bytes;
    int64_t  mtime_seconds;
    int64_t  mtime_nanoseconds;
    uint32_t permissions_mode;
};

class InputHasher {
public:
    explicit InputHasher(std::filesystem::path root_directory);
    ~InputHasher();

    // Hashing ultra-rápido de metadados (Tier 1)
    CarbonResult<std::string> ComputeStatHash(const std::vector<std::string>& exclusions = {});

    // Hashing criptográfico completo de conteúdo (Tier 2 - SHA-256)
    CarbonResult<std::string> ComputeDeepContentHash(const std::vector<std::string>& exclusions = {});

    // Adiciona flags de linha de comando ou variáveis de ambiente ao hash composto
    void AppendEnvironmentVariable(const std::string& key, const std::string& value);
    void AppendToolchainChecksum(const std::string& tool_name, const std::string& sha256_hex);

    // Retorna a digital final composta
    std::string FinalizeCompositeFingerprint();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace carbon::fingerprint
```

### 10.2 `include/carbon/dag.hpp`
```cpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "carbon/error.hpp"

namespace carbon::dag {

class DependencyGraph {
public:
    DependencyGraph();
    ~DependencyGraph();

    // Adiciona um nó ao grafo
    CarbonResult<void> RegisterNode(std::shared_ptr<TaskNode> node);

    // Cria uma aresta de dependência (consumer depende de provider)
    CarbonResult<void> AddDependency(const std::string& consumer_id, 
                                     const std::string& provider_id, 
                                     bool is_hard_dependency = true);

    // Executa o algoritmo de Kahn e retorna a ordem topológica de execução
    CarbonResult<std::vector<std::shared_ptr<TaskNode>>> ResolveTopologicalOrder();

    // Invalida em cascata todos os nós descendentes de um nó que sofreu alteração
    void InvalidateDownstreamCascade(const std::string& changed_node_id);

    // Retorna todos os nós prontos para execução imediata (in-degree zero)
    std::vector<std::shared_ptr<TaskNode>> GetExecutableNodes();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace carbon::dag
```

### 10.3 `include/carbon/scheduler.hpp`
```cpp
#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "carbon/dag.hpp"

namespace carbon::scheduler {

struct SchedulerConfig {
    uint32_t max_concurrency_threads{std::thread::hardware_concurrency()};
    uint64_t max_jvm_memory_mb{16384}; // Limite total para processos Java
    uint32_t jvm_token_capacity{4};    // Máximo de instâncias pesadas simultâneas de apktool
};

class WorkStealingScheduler {
public:
    explicit WorkStealingScheduler(SchedulerConfig config);
    ~WorkStealingScheduler();

    // Inicia a execução do grafo em paralelo
    CarbonResult<void> ExecuteGraph(std::shared_ptr<dag::DependencyGraph> graph,
                                    std::function<void(const std::string& node_id, float progress)> progress_callback);

    // Controle de tokens pesados para processos Java/Apktool
    bool TryAcquireJvmToken(uint32_t timeout_ms = 10000);
    void ReleaseJvmToken();

    // Interrompe imediatamente todas as threads (usado em caso de erro fatal ou SIGINT)
    void RequestEarlyAbort();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace carbon::scheduler
```

### 10.4 `include/carbon/ledger.hpp`
```cpp
#pragma once
#include <string>
#include <optional>
#include "carbon/error.hpp"

namespace carbon::ledger {

struct CacheRecord {
    std::string node_id;
    std::string input_fingerprint;
    std::string artifact_path;
    uint64_t artifact_size;
    std::string artifact_sha256;
    uint64_t execution_duration_ms;
    int64_t timestamp_epoch;
};

class SqliteLedger {
public:
    explicit SqliteLedger(const std::filesystem::path& database_path);
    ~SqliteLedger();

    CarbonResult<void> InitializeDatabase();

    // Consulta se uma tarefa já possui resultado válido em cache
    CarbonResult<std::optional<CacheRecord>> QueryCache(const std::string& node_id, 
                                                        const std::string& input_fingerprint);

    // Registra o sucesso de uma tarefa recém-compilada
    CarbonResult<void> CommitExecution(const CacheRecord& record);

    // Invalida explicitamente entradas de cache
    CarbonResult<void> InvalidateNode(const std::string& node_id);
    CarbonResult<void> PurgeAll();

    // Inicia e encerra sessão de compilação
    CarbonResult<int64_t> BeginBuildRun(const std::string& target_device, const std::string& flags_json);
    CarbonResult<void> CompleteBuildRun(int64_t run_id, int exit_code, uint64_t time_saved_ms);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace carbon::ledger
```

### 10.5 `include/carbon/supervisor.hpp`
```cpp
#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "carbon/error.hpp"

namespace carbon::supervisor {

struct CommandSpec {
    std::string executable_path;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    std::unordered_map<std::string, std::string> environment_overrides;
    std::chrono::milliseconds timeout{std::chrono::minutes(15)};
};

struct ExecutionOutput {
    int exit_code;
    std::string standard_output;
    std::string standard_error;
    std::chrono::milliseconds elapsed_time;
};

class ProcessSupervisor {
public:
    ProcessSupervisor();
    ~ProcessSupervisor();

    // Executa comando de forma assíncrona com monitoramento de epoll e pipes
    CarbonResult<ExecutionOutput> ExecuteSynchronous(const CommandSpec& spec);

    // Criação e promoção de arquivos atômicos
    static CarbonResult<void> AtomicRename(const std::filesystem::path& source_tmp, 
                                           const std::filesystem::path& final_destination);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace carbon::supervisor
```

### 10.6 `include/carbon/key_manager.hpp`
```cpp
#pragma once
#include <string>
#include <filesystem>
#include "carbon/error.hpp"

namespace carbon::security {

enum class KeyProfile : uint8_t {
    DEVELOPER_RELEASE_KEYS = 0,
    OFFICIAL_CARBON_KEYS = 1,
    AOSP_TEST_KEYS_FALLBACK = 2
};

struct KeyBundle {
    KeyProfile profile;
    std::filesystem::path platform_pk8;
    std::filesystem::path platform_pem;
    std::filesystem::path ota_pk8;
    std::filesystem::path ota_pem;
    std::string platform_der_hex;
};

class KeyManager {
public:
    explicit KeyManager(std::filesystem::path project_root);
    ~KeyManager();

    // Localiza e determina as chaves a serem utilizadas
    CarbonResult<KeyBundle> DiscoverActiveKeys(const std::optional<std::filesystem::path>& custom_keys_dir = std::nullopt);

    // Gera um novo par de chaves RSA 4096-bit no padrão Android
    CarbonResult<void> GenerateNewKeyPair(const std::filesystem::path& output_dir, 
                                         const std::string& organization_name, 
                                         const std::string& common_name);

    // Injeta a chave ativa diretamente no bytecode Smali do services.jar
    CarbonResult<void> InjectPlatformSignature(const std::filesystem::path& decoded_services_dir, 
                                              const std::string& platform_der_hex);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace carbon::security
```

### 10.7 `include/carbon/recipe.hpp` (O Motor de Receita Declarativa)
```cpp
#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include "carbon/common.hpp"
#include "carbon/ledger.hpp"

namespace carbon::recipe {

struct StepMetadata {
    uint32_t uid{0};
    uint32_t gid{0};
    uint32_t mode{0644};
    std::string selabel{"u:object_r:system_file:s0"};
};

struct RecipeStep {
    uint32_t step_number{0};
    std::string id;
    std::string category;
    std::string description;
    std::string credit;
    std::string action;

    std::string source;
    std::string destination;
    std::string file;
    std::string key;
    std::string value;
    std::string search_pattern;
    std::string replace_pattern;
    std::string script_path;
    std::vector<std::string> script_args;
    std::string condition;
    std::string apk;
    std::string patch;

    std::optional<StepMetadata> metadata;
};

struct RecipeManifest {
    std::string project_name{"UN1CA Carbon IX"};
    std::string target_device{"dm1q"};
    std::string engine_version{"1.0.0-PROD"};
    std::vector<RecipeStep> steps;
};

class RecipeExecutor {
public:
    explicit RecipeExecutor(fs::path project_root, fs::path recipe_file, ledger::SqliteLedger& ledger);
    ~RecipeExecutor();

    CarbonResult<RecipeManifest> LoadRecipe();
    CarbonResult<void> ExecuteAll(const std::unordered_map<std::string, std::string>& env_vars);
    CarbonResult<bool> ExecuteStep(const RecipeStep& step, const std::unordered_map<std::string, std::string>& env_vars);

private:
    fs::path project_root_;
    fs::path recipe_file_;
    ledger::SqliteLedger& ledger_;

    CarbonResult<void> ActionFsCopy(const RecipeStep& step);
    CarbonResult<void> ActionFsDelete(const RecipeStep& step);
    CarbonResult<void> ActionPropSet(const RecipeStep& step);
    CarbonResult<void> ActionFloatingFeatureSet(const RecipeStep& step);
    CarbonResult<void> ActionSmaliPatch(const RecipeStep& step);
    CarbonResult<void> ActionScriptRun(const RecipeStep& step);
    CarbonResult<void> ActionApkPatch(const RecipeStep& step);
};

} // namespace carbon::recipe
```

---

## 11. ERROR SYSTEM & FORENSIC DIAGNOSTIC CODES

O Carbon Engine utiliza uma classe de tipo resultado monadic `CarbonResult<T>` inspirada no padrão `std::expected` (C++23) e `Result<T, E>` do Rust, eliminando exceções descontroladas em threads de compilação.

### 11.1 Tabela de Códigos de Erro Padronizados

| Código Enum | Valor Numérico | Categoria | Descrição / Ação Corretiva |
| :--- | :--- | :--- | :--- |
| `E_SUCCESS` | `0` | Sucesso | Operação completada com sucesso. |
| `E_STAT_SCAN_FAILED` | `101` | Fingerprint | Falha de syscall `statx` ao ler metadados do arquivo. |
| `E_HASH_IO_ERROR` | `102` | Fingerprint | Arquivo desapareceu ou erro de leitura de bloco durante SHA-256. |
| `E_CYCLE_DETECTED` | `201` | DAG | Dependência circular detectada entre nós da compilação. |
| `E_NODE_NOT_FOUND` | `202` | DAG | Referência a nó inexistente no grafo de tarefas. |
| `E_JVM_TOKEN_TIMEOUT`| `301` | Scheduler | Esgotamento de tempo aguardando token de memória da JVM. |
| `E_SUBPROCESS_FAIL` | `302` | Supervisor | O comando compilador retornou código de saída diferente de zero. |
| `E_SUBPROCESS_TIMEOUT`| `303`| Supervisor | Processo ultrapassou o tempo limite configurado e foi abortado. |
| `E_ATOMIC_RENAME_FAIL`| `304`| Supervisor | Falha ao promover arquivo temporário via `renameat2`. |
| `E_SQLITE_ERROR` | `401` | Ledger | Erro na execução de instrução SQL ou corrupção de banco de dados. |
| `E_KEY_INVALID` | `501` | Security | Chave privada PKCS#8 ou certificado X.509 malformado/inválido. |
| `E_EROFS_BUILD_FAIL` | `601` | Filesystem | Falha na ferramenta `mkfs.erofs` ao construir imagem de partição. |
| `E_BROTLI_FAIL` | `602` | Filesystem | Erro de stream durante a compressão de blocos `.new.dat.br`. |
| `E_SUPER_OVERFLOW` | `603` | Filesystem | Tamanho total das partições excede a partição física `super` do aparelho. |

---

## 12. COMMAND-LINE INTERFACE SPECIFICATION (CLI)

O executável binário compilado chama-se `carbon-engine` e expõe a seguinte árvore de comandos:

```text
USO:
  carbon-engine [SUBCOMANDO] [OPÇÕES]

SUBCOMANDOS:
  build       Inicia o pipeline de compilação da ROM (padrão)
  clean       Limpa artefatos temporários e/ou banco de dados do cache
  keys        Assistente de gerenciamento e geração de chaves criptográficas
  inspect     Inspeciona o cache, fingerprints e histórico de componentes
  diff        Compara a árvore atual com a fingerprint da última compilação

OPÇÕES DO SUBCOMANDO 'build':
  -t, --target <device>        Define o codinome do aparelho alvo (padrão: "dm1q")
  -j, --jobs <n>               Define o número de threads paralelas (padrão: CPU Cores)
  --keys <path>                Caminho customizado para chaves de assinatura privadas
  --clean-node <node_id>       Força a invalidação e recompilação de um nó específico
  --no-cache                   Desativa completamente o cache (força cold build do zero)
  --dry-run                    Calcula fingerprints e exibe ações sem executar comandos
  --bootanim <carbon|rezoss>   Define a animação de boot padrão de fábrica (default: carbon)
  --telemetry-debloat <bool>   Ativa ou desativa o módulo de debloat de telemetria (default: true)
  --manifest-out <path>        Caminho customizado para gravação do build_manifest.json

OPÇÕES DO SUBCOMANDO 'keys':
  --generate                   Dispara o assistente interativo de geração de chaves
  --out <dir>                  Diretório de destino das novas chaves geradas
  --org <nome>                 Nome da Organização (ex: "ALRI Development")
  --cn <nome>                  Nome Comum do Certificado (ex: "AlexsAR")
```

---

## 13. FORMAL BUILD MANIFEST SCHEMA (`build_manifest.json`)

Toda compilação gera e preserva o arquivo `out/build_manifest.json`, estruturado segundo a seguinte especificação de schema JSON:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "CarbonEngineBuildManifest",
  "type": "object",
  "required": [
    "engine_version",
    "build_run_id",
    "timestamp_iso",
    "target_device",
    "pda_version",
    "key_profile",
    "summary",
    "layers"
  ],
  "properties": {
    "engine_version": { "type": "string", "example": "1.0.0-PROD" },
    "build_run_id": { "type": "integer", "example": 42 },
    "timestamp_iso": { "type": "string", "format": "date-time" },
    "target_device": { "type": "string", "example": "dm1q" },
    "pda_version": { "type": "string", "example": "S911BXXUAZZI8" },
    "key_profile": { "type": "string", "enum": ["DEVELOPER_RELEASE_KEYS", "OFFICIAL_CARBON_KEYS", "AOSP_TEST_KEYS_FALLBACK"] },
    "summary": {
      "type": "object",
      "properties": {
        "total_nodes": { "type": "integer" },
        "nodes_cached": { "type": "integer" },
        "nodes_rebuilt": { "type": "integer" },
        "wall_time_ms": { "type": "integer" },
        "time_saved_ms": { "type": "integer" },
        "cache_hit_ratio": { "type": "number", "minimum": 0.0, "maximum": 1.0 }
      }
    },
    "layers": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "layer_level": { "type": "integer" },
          "layer_name": { "type": "string" },
          "nodes": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "node_id": { "type": "string" },
                "status": { "type": "string", "enum": ["CACHE_HIT", "REBUILT", "FAILED", "SKIPPED"] },
                "input_fingerprint": { "type": "string" },
                "output_sha256": { "type": "string" },
                "execution_duration_ms": { "type": "integer" },
                "artifacts": {
                  "type": "array",
                  "items": { "type": "string" }
                }
              }
            }
          }
        }
      }
    }
  }
}
```

---

## 14. THREE-PHASE IMPLEMENTATION ROADMAP

A transição da infraestrutura atual em Bash para o motor nativo `carbon-engine` será executada de forma estritamente modular:

```
┌────────────────────────────────────────────────────────┐
│  FASE 1: O Wrapper Inteligente de Cache (Híbrido)      │
│  - Binário Carbon Engine executa em C++ apenas:         │
│    * Stat-Cache e Fingerprinting SHA-256               │
│    * Cache de APKs individuais (SecSettings, etc.)     │
│  - make_carbon.sh consome o cache chamando o binário.  │
│  - Redução de tempo esperada: de 25 min para 8 min.    │
└───────────────────────────┬────────────────────────────┘
                            │
┌───────────────────────────┴────────────────────────────┐
│  FASE 2: Orquestração de Partições & Brotli Concorrente│
│  - Carbon Engine assume a Camada 4 e 5:                │
│    * Detecção de partições inalteradas (skip EROFS)    │
│    * Compressão paralela Brotli multithread            │
│  - Redução de tempo esperada: de 8 min para 2 min.     │
└───────────────────────────┬────────────────────────────┘
                            │
┌───────────────────────────┴────────────────────────────┐
│  FASE 3: Motor Nativo Autônomo Completo                │
│  - Carbon Engine assume o Grafo Completo (DAG).        │
│  - Elimina chamadas sequenciais legadas do make_rom.sh.│
│  - Compilação incremental concluída em: < 60 segundos. │
└────────────────────────────────────────────────────────┘
```

---

## 15. TEST PLAN & CONTINUOUS VERIFICATION PROTOCOL

O Carbon Engine é acompanhado por uma suíte de testes de estresse e conformidade automatizada antes de cada liberação de versão:

### 15.1 Teste de Determinismo Criptográfico (Bit-for-Bit Determinism)
```bash
# Executa duas compilações idênticas consecutivas a partir do zero
carbon-engine build --target=dm1q --out=/tmp/build_A
carbon-engine build --target=dm1q --out=/tmp/build_B

# Valida se os hashes dos arquivos finais são 100% idênticos
sha256sum /tmp/build_A/UN1CA_Carbon_IX_1.0_dm1q_ZZI8.zip
sha256sum /tmp/build_B/UN1CA_Carbon_IX_1.0_dm1q_ZZI8.zip
# Critério de Aceitação: Hashes devem coincidir exatamente.
```

### 15.2 Teste de Injeção de Falhas e Interrupção Abrupta (`Fault Injection`)
```bash
# Inicia compilação em background
carbon-engine build --target=dm1q &
PID=$!

# Aguarda atingir a fase de escrita de APK mais pesada (SecSettings)
sleep 45

# Envia interrupção violenta não tratável
kill -9 $PID

# Executa imediatamente nova compilação
carbon-engine build --target=dm1q
# Critério de Aceitação:
# 1. O banco SQLite não pode acusar corrupção (PRAGMA integrity_check == OK).
# 2. Nenhum arquivo .tmp órfão pode ser linkado como se fosse válido.
# 3. A compilação é concluída com sucesso a partir do ponto de recuperação.
```

### 15.3 Teste de Invalidação Seletiva de Cache (Single-Node Invalidation)
```bash
# Modifica uma string em SecSettings
echo "<!-- test comment -->" >> carbon_ours/branding_settings/SecSettings.apk/res/values/strings.xml

# Executa build incremental monitorando tempo
time carbon-engine build --target=dm1q
# Critério de Aceitação:
# - Apenas SecSettings.apk e system.img devem ser reconstruídos.
# - vendor.img, product.img, odm.img, etc., devem registrar CACHE_HIT (0.00s).
# - Tempo total de execução: < 45 segundos.
```

---

## 16. IMPLEMENTATION ACCEPTANCE CRITERIA

1. **Paridade Funcional Absoluta:** O arquivo `.zip` final produzido pelo `carbon-engine` deve ser bit-a-bit compatível com o instalador do TWRP e inicializar o sistema operacional com o mesmo comportamento do pipeline original.
2. **Determinismo de Cache:** Duas execuções consecutivas de `carbon-engine build` sem nenhuma alteração nos arquivos fonte devem resultar em **$100\%$ de Cache HIT** e terminar em menos de **15 segundos**.
3. **Isolamento de Segurança:** Nenhuma chave privada fornecida em `security/keys/` pode vazar em logs, manifestos públicos ou artefatos do repositório.
4. **Resiliência a Falhas:** Interromper a compilação com `SIGINT` (`Ctrl+C`) em qualquer instante da execução não pode corromper o banco `ledger.db` nem deixar arquivos parciais utilizáveis na árvore de build.

---
*Fim da Especificação Técnica `specs.md` — Aprovado para Implementação pela Equipe de Plataforma ALRIGROUP.*
