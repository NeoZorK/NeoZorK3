# Real Trading Setup Guide
# Руководство по настройке для реальной торговли

## ⚠️ ВАЖНОЕ ПРЕДУПРЕЖДЕНИЕ

**Торговля с реальными деньгами сопряжена с высокими рисками!**
- Арбитражная торговля может привести к потерям
- Всегда начинайте с небольших сумм
- Тщательно тестируйте все настройки
- Используйте только те средства, потерю которых можете себе позволить

## 📋 Предварительные требования

### 1. Solana Wallet Setup
```bash
# Установка Solana CLI
sh -c "$(curl -sSfL https://release.solana.com/stable/install)"

# Создание нового кошелька (РЕКОМЕНДУЕТСЯ для торговли)
solana-keygen new --outfile ~/.config/solana/trading-wallet.json

# Проверка баланса
solana balance ~/.config/solana/trading-wallet.json
```

### 2. Получение SOL для комиссий
```bash
# Покупка SOL на бирже и перевод в кошелек
# Минимум: 0.1 SOL для комиссий
# Рекомендуется: 0.5-1 SOL для активной торговли
```

### 3. Настройка RPC Endpoint
```bash
# Бесплатные RPC (ограниченная пропускная способность)
# https://api.mainnet-beta.solana.com

# Платные RPC (рекомендуется для торговли)
# QuickNode: https://quicknode.com/
# Alchemy: https://www.alchemy.com/
# Helius: https://helius.xyz/
```

## 🔧 Настройка конфигурации

### 1. Создание production конфигурации
```json
{
  "rpc_endpoint": "https://your-paid-rpc-endpoint.com",
  "wallet_private_key": "your_private_key_here",
  "max_trade_size": 50.0,
  "min_profit_percentage": 0.8,
  "max_concurrent_orders": 3,
  "order_timeout_ms": 15000,
  "dry_run": false,
  "risk_limits": {
    "max_position_size": 500.0,
    "max_daily_loss": 50.0,
    "max_drawdown": 5.0,
    "max_orders_per_minute": 5
  },
  "dexes": {
    "raydium": {
      "enabled": true,
      "priority": 1,
      "slippage_tolerance": 0.5
    },
    "orca": {
      "enabled": true,
      "priority": 2,
      "slippage_tolerance": 0.5
    },
    "jupiter": {
      "enabled": true,
      "priority": 3,
      "slippage_tolerance": 0.3
    }
  },
  "strategies": {
    "triangular_arbitrage": {
      "enabled": false,
      "min_profit_percentage": 1.0,
      "max_trade_size": 25.0
    },
    "cross_dex_arbitrage": {
      "enabled": true,
      "min_profit_percentage": 0.8,
      "max_trade_size": 50.0
    },
    "statistical_arbitrage": {
      "enabled": false,
      "min_profit_percentage": 1.5,
      "max_trade_size": 100.0
    }
  },
  "logging": {
    "level": "INFO",
    "file": "logs/trading.log",
    "max_file_size_mb": 100,
    "max_files": 10
  },
  "monitoring": {
    "stats_interval_seconds": 60,
    "health_check_interval_seconds": 30,
    "alert_on_risk_violation": true,
    "telegram_notifications": {
      "enabled": true,
      "bot_token": "your_telegram_bot_token",
      "chat_id": "your_chat_id"
    }
  }
}
```

### 2. Настройка приватного ключа
```bash
# БЕЗОПАСНОЕ хранение приватного ключа
# НИКОГДА не коммитьте приватный ключ в git!

# Создание .env файла (добавить в .gitignore)
echo "SOLANA_PRIVATE_KEY=your_private_key_here" > .env

# Или использование переменной окружения
export SOLANA_PRIVATE_KEY="your_private_key_here"
```

## 🧪 Тестирование перед реальной торговлей

### 1. Dry-run тестирование
```bash
# Запуск в режиме симуляции
./build/solana_arbitrage_bot --dry-run --verbose --config config.json

# Проверка логики без реальных сделок
# Должно работать минимум 24 часа без ошибок
```

### 2. Тестирование с минимальными суммами
```bash
# Настройка минимальных лимитов
{
  "max_trade_size": 1.0,
  "min_profit_percentage": 2.0,
  "risk_limits": {
    "max_position_size": 10.0,
    "max_daily_loss": 5.0
  }
}

# Запуск с реальными деньгами (минимальные суммы)
./build/solana_arbitrage_bot --config config.json --minimal-trading
```

## 🚀 Запуск реальной торговли

### 1. Финальная проверка
```bash
# Проверка баланса кошелька
solana balance

# Проверка подключения к RPC
curl -X POST -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"getHealth"}' \
  https://your-rpc-endpoint.com

# Проверка конфигурации
./build/solana_arbitrage_bot --validate-config config.json
```

### 2. Запуск бота
```bash
# Запуск в production режиме
./build/solana_arbitrage_bot --config config.json --production

# Или с дополнительными параметрами
./build/solana_arbitrage_bot \
  --config config.json \
  --production \
  --log-level INFO \
  --monitoring-interval 30
```

### 3. Мониторинг работы
```bash
# Просмотр логов в реальном времени
tail -f logs/trading.log

# Проверка статистики
./build/solana_arbitrage_bot --show-stats

# Мониторинг через веб-интерфейс (если настроен)
# http://localhost:8080/dashboard
```

## 🛡️ Управление рисками

### 1. Настройка стоп-лоссов
```json
{
  "risk_limits": {
    "max_daily_loss": 50.0,
    "max_drawdown": 5.0,
    "max_consecutive_losses": 3,
    "emergency_stop_loss": 10.0
  }
}
```

### 2. Мониторинг в реальном времени
```bash
# Скрипт мониторинга
#!/bin/bash
while true; do
  # Проверка баланса
  BALANCE=$(solana balance)
  echo "$(date): Balance: $BALANCE"
  
  # Проверка логов на ошибки
  if grep -q "ERROR\|CRITICAL" logs/trading.log; then
    echo "ALERT: Errors detected in logs!"
    # Отправка уведомления
  fi
  
  sleep 60
done
```

### 3. Автоматическое резервное копирование
```bash
# Скрипт резервного копирования
#!/bin/bash
DATE=$(date +%Y%m%d_%H%M%S)
cp config.json backup/config_$DATE.json
cp logs/trading.log backup/trading_$DATE.log
```

## 📊 Анализ производительности

### 1. Метрики для отслеживания
- Общая прибыль/убыток
- Количество успешных сделок
- Средняя прибыль на сделку
- Время выполнения сделок
- Комиссии за транзакции
- Slippage (проскальзывание)

### 2. Оптимизация параметров
```bash
# Анализ логов для оптимизации
grep "PROFIT" logs/trading.log | awk '{sum+=$NF} END {print "Total profit:", sum}'
grep "LOSS" logs/trading.log | awk '{sum+=$NF} END {print "Total loss:", sum}'
```

## 🔄 Масштабирование

### 1. Увеличение капитала
```json
{
  "max_trade_size": 100.0,
  "max_position_size": 1000.0,
  "risk_limits": {
    "max_daily_loss": 100.0
  }
}
```

### 2. Добавление новых DEX
```json
{
  "dexes": {
    "serum": {
      "enabled": true,
      "priority": 4
    },
    "openbook": {
      "enabled": true,
      "priority": 5
    }
  }
}
```

## 🆘 Устранение неполадок

### 1. Частые проблемы
```bash
# RPC ошибки
# Решение: Переключиться на платный RPC

# Недостаточно SOL для комиссий
# Решение: Пополнить кошелек

# Высокий slippage
# Решение: Уменьшить размер сделок

# Частые ошибки транзакций
# Решение: Увеличить timeout и retry
```

### 2. Экстренная остановка
```bash
# Остановка бота
pkill -f solana_arbitrage_bot

# Проверка открытых позиций
./build/solana_arbitrage_bot --show-positions

# Закрытие позиций (если необходимо)
./build/solana_arbitrage_bot --close-all-positions
```

## 📞 Поддержка

При возникновении проблем:
1. Проверьте логи: `tail -f logs/trading.log`
2. Проверьте баланс: `solana balance`
3. Проверьте подключение к RPC
4. Обратитесь к документации
5. Создайте issue в репозитории

## ⚖️ Правовые аспекты

- Убедитесь в соответствии местному законодательству
- Ведите учет всех транзакций для налоговых целей
- Соблюдайте правила бирж и DEX
- Получите консультацию юриста при необходимости

---

**УДАЧИ В ТОРГОВЛЕ! 🚀**
