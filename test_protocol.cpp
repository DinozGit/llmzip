#include <iostream>
#include <string>
#include "core/protocol_parser.h"

int main() {
    llmzip::ProtocolParser parser;
    parser.load_default_rules();

    // Пример из чата — уже в протоколе (как AI сгенерировал)
    std::string protocol_input = R"(Status: AnalystApproved; Arch: DataEngine; →tech; →no-meta
1.Flywheel: Cache:FoodCache(is_verified→↑qual); Log:ai_usage_log→↓cost/query; Metric:model:eff
2.ColdStart: Strategy:DataAcq→1st; ~RAG/pgvector→iter2; ✅API:CRUD; ~CSV:load; ✅Seed:admin
3.Budget: Log:tokens_in/out; duration_ms; provider → cost:full
4.Monitor: Admin:graphs/provider+model → ✅
5.Drift: RAG-phase: updated_at→re-embed
Next: →steps; 1.AI:log→live; 2.PG+pgvector+Emb; 3.CSV→FoodCache
→ready)";

    std::cout << "=== TEST 1: apply_rules(protocol, false) — 'compress' ===\n";
    std::string compressed = parser.apply_rules(protocol_input, false);
    std::cout << compressed << "\n";
    std::cout << "Size: " << compressed.size() << " chars\n\n";

    std::cout << "=== TEST 2: apply_rules(protocol, true) — 'decompress' ===\n";
    std::string decompressed = parser.apply_rules(protocol_input, true);
    std::cout << decompressed << "\n";
    std::cout << "Size: " << decompressed.size() << " chars\n\n";

    // Теперь наоборот — исходный русский текст
    std::string russian_input = R"(Что мы уже заложили в админке под эту стратегию

### 1. Flywheel Effect — Мы уже строим
Аналитик прав — ключевое преимущество в **самообучающемся кэше**. В админке:
- **Food Cache с флагом `is_verified`** — каждое подтверждение пользователя (или админа) повышает качество
- **Таблица `ai_usage_log`** — каждый запрос логируется, чтобы понимать cost-per-query
- **Статистика по моделям** — видно, какие модели экономнее/точнее

### 2. Cold Start Problem — Наш план на следующий чат
Аналитик подчеркивает **Data Acquisition**. Именно поэтому я отложил pgvector/RAG на следующую итерацию и сначала сделал:
- ✅ REST API для CRUD FoodCache — чтобы можно было наполнить базу вручную
- ✅ **CSV-загрузчик** (в плане) — массовый импорт продуктов
- ✅ Seed-админ — чтобы ты мог сразу заливать данные

### 3. Token Budgeting — Уже отслеживаем
`ai_usage_log` хранит `tokens_in`, `tokens_out`, `duration_ms`, `provider` — это даёт полную картину затрат.

### 4. Data Drift — Учтём в RAG-фазе
Добавим `updated_at` в FoodCache + механизм переиндексации эмбеддингов для недавно изменённых записей.

---

**TL;DR:** Аналитик по сути одобрил наш roadmap и план. Админ-панель, которую мы сделали — это **фундамент Data Engine**. В следующем чате:
1. Подключим логирование AI (чтобы статистика ожила)
2. PostgreSQL + pgvector + эмбеддинги
3. CSV-загрузчик для быстрого наполнения FoodCache)";

    std::cout << "=== TEST 3: apply_rules(russian, false) — 'compress' ===\n";
    std::string comp_ru = parser.apply_rules(russian_input, false);
    std::cout << comp_ru << "\n";
    std::cout << "Size: " << comp_ru.size() << " chars (original: " << russian_input.size() << ")\n\n";

    std::cout << "=== TEST 4: apply_rules(comp_ru, true) — roundtrip ===\n";
    std::string roundtrip = parser.apply_rules(comp_ru, true);
    std::cout << roundtrip << "\n";
    std::cout << "Size: " << roundtrip.size() << " chars\n";

    return 0;
}
