# Solana Testnet Trading Guide
# Руководство по торговле на Solana Testnet

## 🧪 Что такое Solana Testnet?

**Solana Testnet** - это тестовая сеть Solana, которая позволяет:
- ✅ Тестировать приложения без риска потери реальных средств
- ✅ Получать бесплатные тестовые токены
- ✅ Отрабатывать торговые стратегии
- ✅ Проверять функциональность DEX
- ✅ Изучать Solana экосистему

## 🚀 Быстрый старт Testnet

### 1. Установка Solana CLI
```bash
# Установка Solana CLI
sh -c "$(curl -sSfL https://release.solana.com/stable/install)"

# Проверка установки
solana --version
```

### 2. Настройка Testnet
```bash
# Переключение на testnet
solana config set --url https://api.testnet.solana.com

# Проверка подключения
solana cluster-version
```

### 3. Создание Testnet кошелька
```bash
# Создание нового кошелька
solana-keygen new --outfile testnet-wallet.json --no-bip39-passphrase

# Установка как default
solana config set --keypair testnet-wallet.json

# Проверка адреса
solana address
```

### 4. Получение Testnet SOL
```bash
# Автоматический запрос через faucet
solana airdrop 1

# Или через веб-интерфейс: https://faucet.solana.com
```

## 🔧 Настройка Testnet торговли

### 1. Копирование конфигурации
```bash
# Копирование testnet конфигурации
cp config.testnet.json config.json

# Редактирование конфигурации
nano config.json
```

### 2. Настройка приватного ключа
```bash
# Получение приватного ключа
solana-keygen pubkey testnet-wallet.json

# Добавление в конфигурацию
# Замените YOUR_TESTNET_PRIVATE_KEY_HERE на полученный ключ
```

### 3. Запуск Testnet торговли
```bash
# Автоматический запуск с проверками
./start_testnet_trading.sh

# Или ручной запуск
./build/solana_arbitrage_bot --config config.json --testnet
```

## 📊 Testnet DEX

### Доступные DEX на Testnet:
1. **Raydium Testnet**
   - API: `https://testnet-api.raydium.io`
   - WS: `wss://testnet-ws.raydium.io`

2. **Orca Testnet**
   - API: `https://testnet-api.orca.so`
   - WS: `wss://testnet-ws.orca.so`

3. **Jupiter Testnet**
   - API: `https://testnet-api.jup.ag`
   - WS: `wss://testnet-ws.jup.ag`

4. **Serum Testnet**
   - API: `https://testnet-api.serum.markets`
   - WS: `wss://testnet-ws.serum.markets`

### Testnet токены:
```json
{
  "SOL": "So11111111111111111111111111111111111111112",
  "USDC": "4zMMC9srt5Ri5X14GAgXhaHii3GnPAEERYPJgZJDncDU",
  "USDT": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
  "ETH": "7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs",
  "BTC": "9n4nbM75f5Ui33ZbPYXn59EwSgE8CGsHtAeTH5YFeJ9E"
}
```

## 🎯 Testnet стратегии

### 1. Cross-DEX Arbitrage
```json
{
  "cross_dex_arbitrage": {
    "enabled": true,
    "min_profit_percentage": 0.5,
    "max_trade_size": 10.0,
    "min_volume": 100.0
  }
}
```

### 2. Triangular Arbitrage
```json
{
  "triangular_arbitrage": {
    "enabled": true,
    "min_profit_percentage": 0.8,
    "max_trade_size": 5.0,
    "max_path_length": 3
  }
}
```

### 3. Statistical Arbitrage
```json
{
  "statistical_arbitrage": {
    "enabled": true,
    "min_profit_percentage": 1.0,
    "max_trade_size": 15.0,
    "lookback_period": 50,
    "confidence_threshold": 0.7
  }
}
```

## 🛡️ Testnet безопасность

### 1. Риски Testnet
- ⚠️ **Нет реальных потерь** - все токены бесплатные
- ⚠️ **Нестабильность сети** - возможны сбои
- ⚠️ **Ограниченная ликвидность** - меньше торговых пар
- ⚠️ **Тестовые данные** - цены могут отличаться от mainnet

### 2. Защитные механизмы
```json
{
  "safety": {
    "enable_circuit_breaker": true,
    "circuit_breaker_threshold": 5,
    "circuit_breaker_timeout_seconds": 600,
    "enable_rate_limiting": true,
    "max_requests_per_minute": 30,
    "enable_slippage_protection": true,
    "max_slippage_percentage": 2.0
  }
}
```

## 📈 Мониторинг Testnet

### 1. Логирование
```json
{
  "logging": {
    "level": "DEBUG",
    "file": "logs/testnet_trading.log",
    "max_file_size_mb": 50,
    "max_files": 5,
    "console_output": true,
    "log_trades": true,
    "log_errors": true,
    "log_network": true
  }
}
```

### 2. Мониторинг в реальном времени
```bash
# Просмотр логов
tail -f logs/testnet_trading.log

# Проверка баланса
solana balance

# Проверка токенов
spl-token accounts
```

### 3. Статистика
```bash
# Просмотр статистики бота
./build/solana_arbitrage_bot --show-stats

# Анализ логов
grep "PROFIT" logs/testnet_trading.log
grep "LOSS" logs/testnet_trading.log
grep "ERROR" logs/testnet_trading.log
```

## 🔄 Автоматическое пополнение

### 1. Настройка Faucet
```json
{
  "testnet_specific": {
    "enable_faucet_requests": true,
    "faucet_url": "https://faucet.solana.com",
    "auto_refill_threshold": 0.05,
    "refill_amount": 0.1,
    "max_refill_requests_per_day": 5
  }
}
```

### 2. Ручное пополнение
```bash
# Запрос SOL
solana airdrop 1

# Запрос токенов
# Перейдите на: https://solfaucet.com
```

## 🧪 Тестирование

### 1. Демонстрация
```bash
# Запуск testnet демонстрации
python3 testnet_demo.py

# Длительность: 90 секунд
# Показывает: балансы, арбитражные возможности, статистику
```

### 2. Стресс-тестирование
```bash
# Запуск с высокими нагрузками
./build/solana_arbitrage_bot --config config.json --testnet --stress-test

# Параметры стресс-теста:
# - Высокая частота сделок
# - Большие объемы
# - Множественные DEX
```

### 3. Тестирование стратегий
```bash
# Тестирование конкретной стратегии
./build/solana_arbitrage_bot --config config.json --testnet --strategy cross_dex

# Доступные стратегии:
# - cross_dex
# - triangular
# - statistical
```

## 📊 Анализ результатов

### 1. Метрики производительности
- Общее количество сделок
- Процент успешных сделок
- Средняя прибыль на сделку
- Время выполнения транзакций
- Комиссии за транзакции

### 2. Анализ логов
```bash
# Общая прибыль
grep "PROFIT" logs/testnet_trading.log | awk '{sum+=$NF} END {print "Total profit:", sum}'

# Общие убытки
grep "LOSS" logs/testnet_trading.log | awk '{sum+=$NF} END {print "Total loss:", sum}'

# Ошибки
grep "ERROR" logs/testnet_trading.log | wc -l
```

### 3. Оптимизация параметров
```json
{
  "optimization": {
    "min_profit_percentage": 0.5,  // Уменьшить для большего количества сделок
    "max_trade_size": 10.0,        // Увеличить для большей прибыли
    "slippage_tolerance": 1.0,     // Увеличить для лучшего исполнения
    "order_timeout_ms": 30000      // Увеличить для стабильности
  }
}
```

## 🔄 Переход на Mainnet

### 1. Подготовка
```bash
# Переключение на mainnet
solana config set --url https://api.mainnet-beta.solana.com

# Проверка mainnet кошелька
solana balance

# Настройка mainnet конфигурации
cp config.production.json config.json
```

### 2. Тестирование на Mainnet
```bash
# Dry-run режим
./start_real_trading.sh

# Проверка в течение 24 часов
# Анализ результатов
# Оптимизация параметров
```

### 3. Запуск реальной торговли
```bash
# Установка dry_run: false
# Запуск с реальными деньгами
./start_real_trading.sh
```

## 🆘 Устранение неполадок

### 1. Частые проблемы Testnet
```bash
# Низкий баланс
solana airdrop 1

# Ошибки RPC
solana config set --url https://api.testnet.solana.com

# Проблемы с кошельком
solana config set --keypair testnet-wallet.json
```

### 2. Отладка
```bash
# Проверка подключения
solana cluster-version

# Проверка баланса
solana balance

# Проверка логов
tail -f logs/testnet_trading.log
```

### 3. Сброс настроек
```bash
# Сброс конфигурации Solana
rm -rf ~/.config/solana

# Создание нового кошелька
solana-keygen new --outfile testnet-wallet.json
```

## 📞 Поддержка

### Полезные ресурсы:
- **Solana Docs**: https://docs.solana.com/
- **Testnet Faucet**: https://faucet.solana.com
- **Solana Explorer**: https://explorer.solana.com/?cluster=testnet
- **Community**: https://discord.gg/solana

### При проблемах:
1. Проверьте логи: `tail -f logs/testnet_trading.log`
2. Проверьте баланс: `solana balance`
3. Проверьте подключение: `solana cluster-version`
4. Обратитесь к документации
5. Создайте issue в репозитории

---

**УДАЧИ В TESTNET ТОРГОВЛЕ! 🧪🚀**
