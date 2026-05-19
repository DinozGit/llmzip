# Полное руководство по протоколу сжатия информации для взаимодействия с ИИ

## Структурные маркеры

| Маркер | Описание | Пример | Расшифровка |
|--------|----------|--------|-------------|
| `Req:` | Требования к задаче или системе | `Req: ✓✓Auth (OAuth2); ~Logging (ELK)` | Обязательная авторизация через OAuth2, опциональное логирование через ELK |
| `Stack:` | Стек технологий | `Stack: Py/DRF/PG` | Python, Django REST Framework, PostgreSQL |
| `Offer:` | Предложение или решение | `Offer: RAG →prob-0.95; Stack: Qdrant/LLM` | Предлагается RAG с достоверностью 95%, стек: Qdrant и LLM |
| `Opt:` | Опциональные элементы | `Opt: Monitoring (Prometheus)` | Опционально добавить мониторинг через Prometheus |
| `Arch:` | Архитектурные решения | `Arch: µserv; →perf` | Микросервисная архитектура с акцентом на производительность |
| `Ctx:` | Контекст сессии/запроса | `Ctx: user:mappex; proj:ronks; lang:ru` | Контекст: пользователь mappex, проект ronks, язык русский |
| `Meta:` | Метаданные запроса | `Meta: ts:2026-05-18T12:00; ver:2.0` | Временная метка и версия протокола |
| `Out:` | Ожидаемый формат вывода | `Out: →json; →tab; →byte-200` | Вывод: JSON, таблица, лимит 200 токенов |
| `Err:` | Обработка ошибок | `Err: →fallback:default; ⚠timeout:30s` | При ошибке: использовать дефолт, таймаут 30 сек |
| `Test:` | Тестовые сценарии | `Test: ✓unit; ~e2e; →mock-llm` | Тесты: юнит-обязательно, e2e-опционально, мок LLM |

## Синтаксические операторы

| Оператор | Описание | Пример | Расшифровка |
|----------|----------|--------|-------------|
| `;` | Разделение групп элементов | `Req: A; B; →perf` | Требования A и B, акцент на производительность |
| `/` | Альтернатива | `LLM: Qwen/Llama/Mistral` | Любая из перечисленных моделей |
| `:` | Ключ-значение | `Metric: latency:ms` | Метрика: задержка в миллисекундах |
| `→` | Причинно-следственная связь | `Cache(Redis) → ↓latency ↑RPS` | Кэширование снижает задержку, повышает RPS |
| `⇒` | Жёсткая импликация | `✓✓Auth ⇒ ✓✓AuditLog` | Если критична авторизация — обязателен аудит-лог |
| `\` | Параллельное выполнение | `Task1 \ Task2` | Задачи 1 и 2 параллельно |
| `>>` | Цепочка преобразований | `RawText >> Normalize >> Compress >> Encode` | Пайплайн обработки текста |
| `[]` | Необязательный блок | `Header[Auth:Bearer]` | Заголовок авторизации (если есть) |
| `{}` | Группировка параметров | `Config:{retry:3; timeout:10s}` | Конфигурация: 3 попытки, таймаут 10 сек |
| `ll` | Параллельные процессы | `Task1 ll Task2` | Задачи 1 и 2 выполняются параллельно |

## Технические термины / аббревиатуры

| Аббр. | Расшифровка | Пример |
|-------|-------------|--------|
| `PG` | PostgreSQL | `Stack: PG` |
| `DRF` | Django REST Framework | `Stack: DRF` |
| `µserv` | Microservices | `Arch: µserv` |
| `PyDev` | Python Developer | `Role: PyDev` |
| `FSDev` | Full-Stack Developer | `Role: FSDev` |
| `DevOps` | DevOps Engineer | `Role: DevOps` |
| `2+y` | 2+ years | `Exp: 2+y` |
| `5K` | 5000 | `Users: 5K` |
| `LLM` | Large Language Model | `Stack: LLM:Qwen3.6` |
| `RAG` | Retrieval-Augmented Generation | `Offer: RAG→Qdrant` |
| `Emb` | Embeddings | `ML: Emb:openai/small` |
| `Tok` | Tokens | `Metric: Tok:in/out` |
| `CtxWin` | Context Window | `Req: CtxWin:32K` |
| `Temp` | Temperature | `ML: Temp:0.3` |
| `TopP` | Top-p sampling | `ML: TopP:0.9` |
| `PromptEng` | Prompt Engineering | `Dev: →PromptEng:chain-of-thought` |
| `FuncCall` | Function Calling | `Req: ✓✓FuncCall` |
| `JSONMode` | JSON-режим вывода | `Out: →JSONMode` |
| `Stream` | Streaming-ответ | `Out: →Stream:SSE` |
| `Batch` | Пакетная обработка | `Ops: Batch:50req/batch` |
| `Q` | Message Queue | `Arch: Q:RabbitMQ/Kafka` |
| `CDN` | Content Delivery Network | `Ops: CDN:Cloudflare` |
| `WAF` | Web Application Firewall | `Sec: ✓WAF` |
| `RBAC` | Role-Based Access Control | `Sec: ✓✓RBAC` |
| `KV` | Key-Value storage | `Stack: KV:Redis` |
| `OLTP/OLAP` | Transactional/Analytical DB | `DB: OLTP:PG; OLAP:ClickHouse` |
| `IaC` | Infrastructure as Code | `Ops: IaC:Terraform` |
| `GitOps` | Git-based Operations | `Ops: GitOps:ArgoCD` |
| `ctx` | Context | `Req: ctx management` |
| `proc` | Processing | `Task: data proc` |
| `cfg` | Config | `File: app.cfg` |

## Спецсимволы

| Символ | Значение | Пример | Применение |
|--------|----------|--------|------------|
| `↓` | Снижает/уменьшает | `→ ↓Tok:70%` | Сжатие токенов на 70% |
| `↑` | Увеличивает/повышает | `→ ↑RPS:2x` | Удвоение запросов в секунду |
| `✓` | Требуется/обязательно | `Req: ✓Validate` | Обязательная валидация |
| `✓✓` | Критическое требование | `Req: ✓✓Idempotent` | Критично: идемпотентность |
| `~` | Примерно/около или опционально | `Opt: ~Cache; Time: ~200ms` | Опциональный кэш, время ~200 мс |
| `@` | Локация/место | `Deploy @prod-eu; User @mappex` | Развёртывание в EU, пользователь mappex |
| `⚠` | Предупреждение | `Status: ⚠RateLimit:90%` | Предупреждение: лимит исчерпан на 90% |
| `✅` | Подтверждено/работает | `Test: ✅Passed:100%` | Тесты пройдены на 100% |
| `❌` | Ошибка/не работает | `Build: ❌Failed:lint` | Сборка не прошла линтинг |
| `🔄` | В процессе/нужно обновить | `Task: 🔄Retry:3` | Задача: повторить до 3 раз |
| `🗑` | Удалить/проигнорировать | `Opt: 🗑LegacyCode` | Опционально: удалить легаси-код |
| `🔒` | Защищено/шифрование | `Sec: ✓✓🔒PII` | Критично: шифрование персональных данных |
| `🔓` | Открыто/публично | `Access: 🔓Read; 🔒Write` | Чтение публичное, запись защищена |
| `📦` | Упаковка/артефакт | `Build: 📦Docker:slim` | Сборка в минимальный Docker-образ |
| `🚀` | Продакшен/запуск | `Deploy: 🚀prod; →now` | Немедленно запустить в прод |

## Модификаторы вывода и поведения

| Модификатор | Описание | Пример | Эффект |
|-------------|----------|--------|--------|
| `→no-meta` | Убирает самореференции ИИ | `→no-meta; Explain: RAG` | Ответ без «как ИИ, я считаю...» |
| `→no-safe` | Убирает смягчающие наречия | `→no-safe; Answer: Yes` | Прямой ответ «Да» |
| `→prob-N` | Оценка вероятности (0-1) | `→prob-0.97; Fact: Valid` | Факту присвоена вероятность 97% |
| `→byte-X` | Лимит токенов на ответ | `→byte-150; Summary: Text` | Саммари не более 150 токенов |
| `→spo` | Только SPO-триплеты | `→spo; Extract: Entities` | Извлечение сущностей в SPO |
| `→tab` | Вывод в виде таблицы | `→tab; Compare: PG/MySQL` | Сравнение в виде таблицы |
| `→code` | Чистый код без пояснений | `→code; Func: compress()` | Вывод только кода функции |
| `→math` | LaTeX-формулы | `→math; Formula: BLEU` | Формулы в LaTeX |
| `→steps` | Нумерованные шаги | `→steps; Task: Deploy` | Инструкция по шагам |
| `→arrow` | Стрелочные списки | `→arrow; Pipeline: A→B→C` | Визуализация пайплайна |
| `→eng` | Force-English | `→eng; Explain: Token` | Объяснение на английском |
| `→ru` | Force-Russian | `→ru; Explain: Токен` | Объяснение на русском |
| `→ans` | One-line ответ | `→ans; Q: Capital of Belarus?` | Ответ: «Минск» (без пояснений) |
| `→why` | Только причины/доказательства | `→why; Issue: Slow API` | Причины медленной работы |
| `→rev` | Сначала вывод, потом объяснение | `→rev; Result: 42; →why` | Сначала ответ «42», затем объяснение |
| `→time` | Timestamp для фактов | `→time; Fact: Updated 2026-04-17` | Факту присвоен timestamp |
| `→risk` | Риск-оценка (low/mid/high) | `→risk; Action: Deploy` | Оценка рисков развёртывания |
| `→cost` | Подсчёт затрат (USD/токены) | `→cost; Task: API calls` | Расчёт стоимости вызовов API |
| `→cmp` | Сравнение «до/после» | `→cmp; Metric: Latency` | Сравнение задержки до и после |
| `→data` | Только данные (без анализа) | `→data; →tab; Metrics: CPU` | Таблица с метриками CPU без анализа |
| `→stat` | Статистический анализ | `→stat; Metric: Response time` | Анализ времени ответа (среднее, медиана) |
| `→trend` | Анализ трендов | `→trend; Period: 7d` | Тренды за последние 7 дней |
| `→outlier` | Поиск аномалий | `→outlier; Threshold: 3σ` | Поиск выбросов с порогом 3 сигмы |
| `→corr` | Корреляционный анализ | `→corr; Vars: X/Y` | Анализ корреляции между X и Y |
| `→arch` | Фокус на архитектуре | `→arch; Review: µserv` | Архитектурный разбор микросервисов |
| `→perf` | Акцент на производительности | `→perf; Target: P99<100ms` | Оптимизация под P99 <100 мс |
| `→now` | Срочный запрос | `→now; Fix: CriticalBug` | Немедленно исправить критичный баг |
| `→later` | Можно отложить | `→later; Task: Refactor` | Рефакторинг можно отложить |
| `→deadline:YYYY-MM-DD` | Дедлайн | `→deadline:2026-06-01; Task: MVP` | Дедлайн запуска MVP |
| `→team` | Упоминание ролей или ответственных | `→team; Owner: @FSDev` | Ответственный: фулстек-разработчик |
| `→review` | Запрос на ревью | `→review; Code: PR#42` | Запрос на ревью pull request #42 |
| `→meet` | Предложение обсудить офлайн | `→meet; Topic: Architecture` | Обсудить архитектуру офлайн |
| `→api:[имя]` | Запрос к API | `→api:Stripe; Task: Payment` | Интеграция с Stripe API для платежей |
| `→db:[имя]` | Работа с базой данных | `→db:PG; Query: SELECT * FROM users` | Запрос к PostgreSQL |
| `→tool:[имя]` | Использование инструмента | `→tool:Docker; Task: Build` | Сборка через Docker |
| `→compress` | Применить сжатие | `→compress; Algo:v2` | Применить алгоритм сжатия версии 2 |
| `→decompress` | Декодировать | `→decompress; Verify: ✓` | Декодировать и проверить целостность |
| `→roundtrip` | Проверить обратимость | `→roundtrip; Test: ✓✓` | Критично: исходный текст восстанавливается полностью |
| `→fingerprint` | Хэш для проверки целостности | `Meta: fp:sha256:abc123` | Контрольная сумма сжатого блока |
| `→diff` | Показать разницу до/после | `→diff; Format: unified` | Вывести diff исходного и сжатого текста |

## Тональность

| Модификатор | Описание | Пример |
|-------------|----------|--------|
| `→formal` | Официальный стиль | `→formal; Report: Quarterly` |
| `→casual` | Неформальный стиль | `→casual; Explain: Blockchain` |
| `→tech` | Максимально технический стиль | `→tech; Topic: Kubernetes` |
| `→simple` | Упрощённое объяснение | `→simple; Topic: Neural Networks` |

## Доменные префиксы

| Префикс | Область | Пример | Применение |
|---------|---------|--------|------------|
| `ML:` | Машинное обучение | `ML: →perf; Model: Qwen3.6` | Оптимизация инференса модели |
| `Sec:` | Безопасность | `Sec: ✓✓🔒Auth; ⚠BruteForce` | Критичная авторизация, защита от брутфорса |
| `Biz:` | Бизнес-логика/аналитика | `Biz: ROI↑15%; →cmp` | Рост ROI на 15%, сравнение сценариев |
| `Dev:` | Разработка | `Dev: →steps; Task: CI/CD` | Пошаговая настройка пайплайна |
| `UX:` | Пользовательский опыт | `UX: ↓Steps:3→1; →why` | Сокращение шагов с 3 до 1: причины |
| `Ops:` | DevOps/инфраструктура | `Ops: →perf; Target: 99.99%` | Целевой аптайм 99.99% |
| `AI:` | LLM-агенты | `AI: →FuncCall; ✓✓JSONMode` | Агент: обязателен вызов функций и JSON-вывод |
| `Query:` | Компрессия запросов | `Query: →compress; Lang:ru; Domain:auto` | Сжать запрос на русском в домене автозапчастей |
| `Session:` | Управление сессиями | `Session: id:abc123; ttl:3600s` | ID сессии и время жизни |
| `Cache:` | Стратегии кэширования | `Cache: ✓✓; Strategy:LRU; TTL:300s` | Кэширование обязательно, LRU, 5 минут |
| `RateLimit:` | Ограничение запросов | `RateLimit: User:100/min; Global:10K/min` | Лимиты на пользователя и глобально |
| `Audit:` | Логирование и аудит | `Audit: ✓; Fields: user;action;ts` | Логировать пользователя, действие, время |

## Примеры комплексных запросов

### 1. Анализ производительности микросервиса
**Вход:**
```
Проанализировать падение производительности микросервиса на Python с PostgreSQL за последние 7 дней. Сравнить метрики до и после обновления. Вывести данные в виде таблицы, указать риски и предложить решения. Ответ должен быть техническим, без самореференций, и умещаться в 50 токенов.
```
**Выход:**
```
Ops: →perf; Stack: Py/µserv/PG; Metric: latency; →trend; Period: 7d; →cmp; →tab; →risk; →tech; →byte-50; →no-meta
```

### 2. Запрос на ревью кода
**Вход:**
```
Нужно провести ревью pull request #42 с изменениями в архитектуре авторизации. Ответственный — фулстек-разработчик. Ответ должен быть кратким и без самореференций.
```
**Выход:**
```
Dev: →review; Code: PR#42; Topic: Auth; →team; Owner: @FSDev; →ans; →no-meta
```

### 3. Сравнение инструментов
**Вход:**
```
Сравнить PostgreSQL и MySQL по производительности и стоимости для проекта с 10K пользователей. Вывести результаты в виде таблицы.
```
**Выход:**
```
Biz: →cmp; DB: PG/MySQL; Metric: perf/cost; Users: 10K; →tab
```

### 4. Архитектурное решение (e-commerce)
**Вход:**
```
Предложить архитектуру для системы рекомендаций на основе RAG с использованием Qdrant и LLM Qwen. Бюджет: $500/мес на API. Масштабирование: до 100K пользователей. Дедлайн: 1 июня 2026. Безопасность: критична (PII).
```
**Выход:**
```
Offer: RAG→Qdrant/LLM:Qwen; →prob-0.95
Arch: µserv; Cache: Redis; Q: RabbitMQ
Req: ✓✓🔒PII → Sec: →encrypt
Users: 100K; Budget: $500/mo
→deadline:2026-06-01
→team; Role: FSDev/PyDev
Out: →tech; →steps
```

## 🔧 Алгоритмический протокол сжатия (Deterministic Parser)

Данный раздел расширяет протокол для реализации без-LLM парсера на основе правил и шаблонов.

### 📐 1. Пайплайн сжатия
`RawText` → `Normalize` (удаление MD, ↓whitespace) → `Segment` (разбивка по блокам) → `Map` (словарь) → `Assemble` (сборка по шаблону)

### 🔍 2. Детерминированный словарь (Priority: Longest Match)
| Исходная фраза | Маркер |
|----------------|--------|
| postgresql | PG |
| microservices | µserv |
| large language model | LLM |
| retrieval-augmented generation | RAG |
| requirements | Req: |
| tech stack | Stack: |
| architecture | Arch: |
| performance | →perf |
| in progress / wip | 🔄 |
| ready / done | ✅ |

### 🧩 3. Шаблон сборки (Assembly Template)
Строгий порядок блоков:
1. `Proj:[Name]; Ver:[N]; Status:[✅/⬜️/🔄]; →tech`
2. `Stack:[Tech/...]; Arch:[Pattern]; Mode:[lossy/lossless/hybrid]`
3. `Done:[Tasks...]; Plan:[Tasks...]`
4. `Meta: fp:auto; ver:2.0; →roundtrip:✓✓`
