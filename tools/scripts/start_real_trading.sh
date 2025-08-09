#!/bin/bash

# Solana Arbitrage Bot - Real Trading Launcher
# Скрипт для запуска реальной торговли

set -e  # Остановка при ошибке

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функции для вывода
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Проверка предварительных условий
check_prerequisites() {
    print_info "Проверка предварительных условий..."
    
    # Проверка наличия Solana CLI
    if ! command -v solana &> /dev/null; then
        print_error "Solana CLI не установлен. Установите: https://docs.solana.com/cli/install-solana-cli-tools"
        exit 1
    fi
    
    # Проверка наличия скомпилированного бота
    if [ ! -f "./build/solana_arbitrage_bot" ]; then
        print_error "Бот не скомпилирован. Запустите: ./build.sh"
        exit 1
    fi
    
    # Проверка наличия конфигурации
    if [ ! -f "./config.json" ]; then
        print_error "Файл config.json не найден"
        exit 1
    fi
    
    print_success "Предварительные условия выполнены"
}

# Проверка безопасности
security_check() {
    print_info "Проверка безопасности..."
    
    # Проверка dry-run режима
    if grep -q '"dry_run": true' config.json; then
        print_warning "ВНИМАНИЕ: Бот запускается в dry-run режиме (без реальных сделок)"
        read -p "Продолжить? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            print_info "Запуск отменен"
            exit 0
        fi
    else
        print_warning "ВНИМАНИЕ: Бот запускается с РЕАЛЬНЫМИ ДЕНЬГАМИ!"
        echo "Убедитесь что вы:"
        echo "1. Протестировали бота в dry-run режиме"
        echo "2. Настроили все лимиты рисков"
        echo "3. Используете только те средства, потерю которых можете себе позволить"
        echo "4. Понимаете риски арбитражной торговли"
        echo
        read -p "ПОДТВЕРДИТЕ что вы понимаете риски и хотите продолжить (YES): " -r
        if [[ ! $REPLY == "YES" ]]; then
            print_info "Запуск отменен"
            exit 0
        fi
    fi
    
    print_success "Проверка безопасности пройдена"
}

# Проверка кошелька
check_wallet() {
    print_info "Проверка кошелька..."
    
    # Получение приватного ключа
    if [ -n "$SOLANA_PRIVATE_KEY" ]; then
        PRIVATE_KEY="$SOLANA_PRIVATE_KEY"
    elif [ -f ".env" ]; then
        source .env
        PRIVATE_KEY="$SOLANA_PRIVATE_KEY"
    else
        print_error "Приватный ключ не найден. Установите переменную SOLANA_PRIVATE_KEY или создайте .env файл"
        exit 1
    fi
    
    # Проверка баланса
    BALANCE=$(solana balance 2>/dev/null || echo "0")
    print_info "Баланс кошелька: $BALANCE SOL"
    
    # Проверка минимального баланса
    if [[ $(echo "$BALANCE < 0.1" | bc -l) -eq 1 ]]; then
        print_warning "Низкий баланс! Рекомендуется минимум 0.1 SOL для комиссий"
        read -p "Продолжить? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 0
        fi
    fi
    
    print_success "Кошелек проверен"
}

# Проверка RPC подключения
check_rpc() {
    print_info "Проверка RPC подключения..."
    
    # Получение RPC endpoint из конфигурации
    RPC_ENDPOINT=$(grep -o '"rpc_endpoint": "[^"]*"' config.json | cut -d'"' -f4)
    
    if [ -z "$RPC_ENDPOINT" ]; then
        print_error "RPC endpoint не найден в конфигурации"
        exit 1
    fi
    
    # Проверка подключения
    if curl -s -X POST -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","id":1,"method":"getHealth"}' \
        "$RPC_ENDPOINT" > /dev/null 2>&1; then
        print_success "RPC подключение работает"
    else
        print_error "Не удается подключиться к RPC endpoint: $RPC_ENDPOINT"
        exit 1
    fi
}

# Проверка конфигурации
validate_config() {
    print_info "Валидация конфигурации..."
    
    # Проверка JSON синтаксиса
    if ! python3 -m json.tool config.json > /dev/null 2>&1; then
        print_error "Ошибка в JSON синтаксисе config.json"
        exit 1
    fi
    
    # Проверка обязательных полей
    REQUIRED_FIELDS=("rpc_endpoint" "wallet_private_key" "max_trade_size" "min_profit_percentage")
    for field in "${REQUIRED_FIELDS[@]}"; do
        if ! grep -q "\"$field\":" config.json; then
            print_error "Отсутствует обязательное поле: $field"
            exit 1
        fi
    done
    
    print_success "Конфигурация валидна"
}

# Создание резервной копии
create_backup() {
    print_info "Создание резервной копии..."
    
    BACKUP_DIR="backup/$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$BACKUP_DIR"
    
    cp config.json "$BACKUP_DIR/"
    if [ -f "logs/trading.log" ]; then
        cp logs/trading.log "$BACKUP_DIR/"
    fi
    
    print_success "Резервная копия создана: $BACKUP_DIR"
}

# Запуск бота
start_bot() {
    print_info "Запуск Solana Arbitrage Bot..."
    
    # Создание директорий для логов
    mkdir -p logs
    
    # Запуск бота
    print_info "Команда запуска: ./build/solana_arbitrage_bot --config config.json"
    
    # Проверка режима запуска
    if grep -q '"dry_run": true' config.json; then
        print_info "Запуск в DRY-RUN режиме (без реальных сделок)"
        ./build/solana_arbitrage_bot --config config.json --dry-run --verbose
    else
        print_warning "Запуск с РЕАЛЬНЫМИ ДЕНЬГАМИ!"
        ./build/solana_arbitrage_bot --config config.json --production
    fi
}

# Мониторинг
monitor_bot() {
    print_info "Запуск мониторинга..."
    
    # Запуск мониторинга в фоне
    (
        while true; do
            sleep 30
            if ! pgrep -f "solana_arbitrage_bot" > /dev/null; then
                print_error "Бот остановлен!"
                break
            fi
            
            # Проверка логов на критические ошибки
            if [ -f "logs/trading.log" ] && grep -q "CRITICAL\|FATAL" logs/trading.log; then
                print_error "Обнаружены критические ошибки в логах!"
            fi
        done
    ) &
    
    MONITOR_PID=$!
    print_success "Мониторинг запущен (PID: $MONITOR_PID)"
}

# Обработка сигналов
cleanup() {
    print_info "Получен сигнал остановки..."
    
    # Остановка бота
    if pgrep -f "solana_arbitrage_bot" > /dev/null; then
        print_info "Остановка бота..."
        pkill -f "solana_arbitrage_bot"
        sleep 2
    fi
    
    # Остановка мониторинга
    if [ -n "$MONITOR_PID" ]; then
        print_info "Остановка мониторинга..."
        kill $MONITOR_PID 2>/dev/null || true
    fi
    
    print_success "Очистка завершена"
    exit 0
}

# Установка обработчиков сигналов
trap cleanup SIGINT SIGTERM

# Главная функция
main() {
    echo "🚀 Solana Arbitrage Bot - Real Trading Launcher"
    echo "================================================"
    
    # Проверки
    check_prerequisites
    security_check
    check_wallet
    check_rpc
    validate_config
    create_backup
    
    # Запуск мониторинга
    monitor_bot
    
    # Запуск бота
    start_bot
}

# Запуск главной функции
main "$@"
