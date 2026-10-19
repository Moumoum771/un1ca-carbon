#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include "carbon/error.hpp"
#include "carbon/fingerprint.hpp"
#include "carbon/ledger.hpp"
#include "carbon/dag.hpp"
#include "carbon/supervisor.hpp"
#include "carbon/key_manager.hpp"

namespace fs = std::filesystem;
using namespace carbon;

static void test_result_monad() {
    std::cout << "[TEST] 1. Testando Monad CarbonResult<T>... ";
    CarbonResult<int> ok_res(42);
    assert(ok_res.is_ok());
    assert(!ok_res.is_err());
    assert(ok_res.value() == 42);

    CarbonResult<int> err_res = CarbonError(ErrorCode::ERR_INVALID_ARGUMENT, "Parametro invalido");
    assert(err_res.is_err());
    assert(err_res.error().code == ErrorCode::ERR_INVALID_ARGUMENT);
    assert(err_res.value_or(99) == 99);

    CarbonResult<void> void_ok = CarbonResult<void>::ok();
    assert(void_ok.is_ok());

    std::cout << "\033[1;32mPASSED\033[0m\n";
}

static void test_input_hasher() {
    std::cout << "[TEST] 2. Testando InputHasher (SHA-256 SIMD OpenSSL)... ";
    fs::path tmp_dir = fs::temp_directory_path() / "carbon_test_hasher";
    fs::create_directories(tmp_dir);

    // Cria arquivos de teste
    std::ofstream f1(tmp_dir / "file1.txt");
    f1 << "UN1CA Carbon Engine Test File 1";
    f1.close();

    std::ofstream f2(tmp_dir / "file2.smali");
    f2 << ".method public static test()V\nreturn-void\n.end method";
    f2.close();

    fingerprint::InputHasher hasher(tmp_dir);
    auto stat_res = hasher.ComputeStatHash();
    assert(stat_res.is_ok());
    assert(stat_res.value().size() == 64);

    auto deep_res = hasher.ComputeDeepContentHash();
    assert(deep_res.is_ok());
    assert(deep_res.value().size() == 64);

    hasher.AppendEnvironmentFlag("BOOTANIMATION_STYLE", "carbon");
    hasher.AppendToolchainChecksum("apktool", "373fa3a000000000");
    std::string comp1 = hasher.FinalizeCompositeFingerprint();
    assert(comp1.size() == 64);

    // Modifica uma flag e verifica se a digital composta altera
    fingerprint::InputHasher hasher2(tmp_dir);
    hasher2.ComputeDeepContentHash();
    hasher2.AppendEnvironmentFlag("BOOTANIMATION_STYLE", "rezoss");
    hasher2.AppendToolchainChecksum("apktool", "373fa3a000000000");
    std::string comp2 = hasher2.FinalizeCompositeFingerprint();
    assert(comp1 != comp2);

    fs::remove_all(tmp_dir);
    std::cout << "\033[1;32mPASSED\033[0m\n";
}

static void test_sqlite_ledger() {
    std::cout << "[TEST] 3. Testando SqliteLedger (ACID & WAL mode)... ";
    fs::path test_db = fs::temp_directory_path() / "carbon_test_ledger" / "test.db";
    fs::remove(test_db);

    ledger::SqliteLedger ledger(test_db);
    auto init_res = ledger.InitializeDatabase();
    assert(init_res.is_ok());

    // Consulta nó não existente
    auto q1 = ledger.QueryCache("apk:SecSettings", "hash_teste_123");
    assert(q1.is_ok());
    assert(!q1.value().has_value());

    // Insere registro
    ledger::CacheRecord rec;
    rec.node_id = "apk:SecSettings";
    rec.input_fingerprint = "hash_teste_123";
    rec.artifact_path = "system/priv-app/SecSettings/SecSettings.apk";
    rec.artifact_size = 135000000;
    rec.artifact_sha256 = "sha256_mock_abc";
    rec.execution_duration_ms = 45000;

    auto commit_res = ledger.CommitExecution(rec);
    assert(commit_res.is_ok());

    // Consulta novamente (deve ser HIT)
    auto q2 = ledger.QueryCache("apk:SecSettings", "hash_teste_123");
    assert(q2.is_ok());
    assert(q2.value().has_value());
    assert(q2.value()->artifact_size == 135000000);
    assert(q2.value()->execution_duration_ms == 45000);

    // Limpa banco
    assert(ledger.PurgeAll().is_ok());
    auto q3 = ledger.QueryCache("apk:SecSettings", "hash_teste_123");
    assert(q3.is_ok());
    assert(!q3.value().has_value());

    fs::remove_all(test_db.parent_path());
    std::cout << "\033[1;32mPASSED\033[0m\n";
}

static void test_dag_cycle_and_order() {
    std::cout << "[TEST] 4. Testando DAG (Algoritmo de Kahn & Deteccao de Ciclos)... ";
    dag::DependencyGraph graph;

    auto n1 = std::make_shared<dag::TaskNode>();
    n1->node_id = "A";
    auto n2 = std::make_shared<dag::TaskNode>();
    n2->node_id = "B";
    auto n3 = std::make_shared<dag::TaskNode>();
    n3->node_id = "C";

    graph.RegisterNode(n1);
    graph.RegisterNode(n2);
    graph.RegisterNode(n3);

    // B depende de A, C depende de B
    graph.AddDependency("B", "A");
    graph.AddDependency("C", "B");

    auto order = graph.ResolveTopologicalOrder();
    assert(order.is_ok());
    assert(order.value().size() == 3);
    assert(order.value()[0]->node_id == "A");
    assert(order.value()[1]->node_id == "B");
    assert(order.value()[2]->node_id == "C");

    // Testa detecção de ciclo: A passa a depender de C (A -> C -> B -> A)
    dag::DependencyGraph cyclic_graph;
    cyclic_graph.RegisterNode(n1);
    cyclic_graph.RegisterNode(n2);
    cyclic_graph.RegisterNode(n3);
    cyclic_graph.AddDependency("B", "A");
    cyclic_graph.AddDependency("C", "B");
    cyclic_graph.AddDependency("A", "C"); // Ciclo!

    auto cycle_res = cyclic_graph.ResolveTopologicalOrder();
    assert(cycle_res.is_err());
    assert(cycle_res.error().code == ErrorCode::ERR_CYCLE_DETECTED);

    std::cout << "\033[1;32mPASSED\033[0m\n";
}

static void test_process_supervisor_and_atomic_rename() {
    std::cout << "[TEST] 5. Testando ProcessSupervisor & AtomicRename... ";
    supervisor::ProcessSupervisor supervisor;

    supervisor::CommandSpec spec;
    spec.executable = "echo";
    spec.args = { "CARBON_ENGINE_TEST" };

    auto out = supervisor.ExecuteSynchronous(spec);
    assert(out.is_ok());
    assert(out.value().exit_code == 0);
    assert(out.value().std_out.find("CARBON_ENGINE_TEST") != std::string::npos);

    // Teste de Atomic Rename
    fs::path tmp_src = fs::temp_directory_path() / "atomic_test.tmp";
    fs::path tmp_dst = fs::temp_directory_path() / "atomic_test.final";

    std::ofstream f(tmp_src);
    f << "atomic_content";
    f.close();

    assert(supervisor::ProcessSupervisor::AtomicRename(tmp_src, tmp_dst).is_ok());
    assert(!fs::exists(tmp_src));
    assert(fs::exists(tmp_dst));
    fs::remove(tmp_dst);

    std::cout << "\033[1;32mPASSED\033[0m\n";
}

static void test_key_manager_generation() {
    std::cout << "[TEST] 6. Testando KeyManager (Geração de Chaves RSA-4096 X.509)... ";
    fs::path tmp_keys = fs::temp_directory_path() / "carbon_test_keys";
    fs::create_directories(tmp_keys);

    security::KeyManager km(tmp_keys);
    auto gen_res = km.GenerateNewKeyPair(tmp_keys, "ALRI Development", "AlexsAR Platform Key");
    assert(gen_res.is_ok());

    assert(fs::exists(tmp_keys / "platform.pk8"));
    assert(fs::exists(tmp_keys / "platform.x509.pem"));

    auto disc_res = km.DiscoverActiveKeys(tmp_keys);
    assert(disc_res.is_ok());
    assert(disc_res.value().profile == security::KeyProfile::DEVELOPER_RELEASE_KEYS);
    assert(disc_res.value().platform_der_hex.size() > 500);

    fs::remove_all(tmp_keys);
    std::cout << "\033[1;32mPASSED\033[0m\n";
}

int main() {
    std::cout << "\033[1;35m============================================================\n"
              << "       CARBON ENGINE — SUÍTE DE TESTES E VALIDAÇÃO          \n"
              << "============================================================\033[0m\n\n";

    test_result_monad();
    test_input_hasher();
    test_sqlite_ledger();
    test_dag_cycle_and_order();
    test_process_supervisor_and_atomic_rename();
    test_key_manager_generation();

    std::cout << "\n\033[1;32m✔ TODOS OS TESTES PASSARAM COM 100% DE SUCESSO!\033[0m\n\n";
    return 0;
}
