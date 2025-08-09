#!/bin/bash

# Solana Arbitrage Bot - Testnet Trading Launcher
# Скрипт для запуска торговли на Solana Testnet

set -e  # Остановка при ошибке

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
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

print_testnet() {
    echo -e "${CYAN}[TESTNET]${NC} $1"
}

# Проверка предварительных условий
check_prerequisites() {
    print_info "Проверка предварительных условий для Testnet..."
    
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
    
    # Проверка наличия testnet конфигурации
    if [ ! -f "./config.testnet.json" ]; then
        print_error "Файл config.testnet.json не найден"
        exit 1
    fi
    
    print_success "Предварительные условия выполнены"
}

# Настройка Solana CLI для Testnet
setup_solana_testnet() {
    print_testnet "Настройка Solana CLI для Testnet..."
    
    # Переключение на testnet
    solana config set --url https://api.testnet.solana.com
    
    # Проверка подключения
    if solana cluster-version > /dev/null 2>&1; then
        print_success "Подключение к Solana Testnet установлено"
    else
        print_error "Не удается подключиться к Solana Testnet"
        exit 1
    fi
}

# Создание testnet кошелька
create_testnet_wallet() {
    print_testnet "Создание кошелька для Testnet..."
    
    WALLET_FILE="testnet-wallet.json"
    
    if [ ! -f "$WALLET_FILE" ]; then
        print_info "Создание нового testnet кошелька..."
        solana-keygen new --outfile "$WALLET_FILE" --no-bip39-passphrase
        
        # Получение публичного ключа
        PUBLIC_KEY=$(solana-keygen pubkey "$WALLET_FILE")
        print_success "Создан testnet кошелек: $PUBLIC_KEY"
        
        # Сохранение публичного ключа
        echo "$PUBLIC_KEY" > testnet-wallet.pubkey
    else
        PUBLIC_KEY=$(solana-keygen pubkey "$WALLET_FILE")
        print_info "Используется существующий testnet кошелек: $PUBLIC_KEY"
    fi
    
    # Установка кошелька как default
    solana config set --keypair "$WALLET_FILE"
    
    print_success "Testnet кошелек настроен"
}

# Получение testnet SOL
get_testnet_sol() {
    print_testnet "Проверка баланса testnet SOL..."
    
    BALANCE=$(solana balance 2>/dev/null || echo "0")
    print_info "Текущий баланс: $BALANCE SOL"
    
    # Проверка минимального баланса
    if [[ $(echo "$BALANCE < 0.1" | bc -l 2>/dev/null || echo "1") -eq 1 ]]; then
        print_warning "Низкий баланс testnet SOL. Запускаем интерактивный faucet..."
        
        # Проверка наличия Python faucet
        if [ -f "testnet_faucet.py" ]; then
            print_info "Запуск интерактивного faucet..."
            python3 testnet_faucet.py
        else
            print_warning "Faucet не найден. Пополните вручную:"
            print_info "1. Перейдите на: https://faucet.solana.com"
            print_info "2. Введите адрес: $(solana address)"
            print_info "3. Запросите 1 SOL"
            
            read -p "Нажмите Enter после пополнения кошелька..."
        fi
    else
        print_success "Достаточно testnet SOL для торговли"
    fi
}

# Настройка testnet конфигурации
setup_testnet_config() {
    print_testnet "Настройка конфигурации для Testnet..."
    
    # Копирование testnet конфигурации
    cp config.testnet.json config.json
    
    # Получение приватного ключа
    WALLET_FILE="testnet-wallet.json"
    if [ -f "$WALLET_FILE" ]; then
        # Извлечение приватного ключа (в формате base58)
        PRIVATE_KEY=$(solana-keygen pubkey "$WALLET_FILE" --outfile /dev/stdout 2>/dev/null || echo "")
        
        # Обновление конфигурации с приватным ключом
        if [ -n "$PRIVATE_KEY" ]; then
            sed -i.bak "s/YOUR_TESTNET_PRIVATE_KEY_HERE/$PRIVATE_KEY/g" config.json
            print_success "Приватный ключ добавлен в конфигурацию"
        fi
    fi
    
    print_success "Testnet конфигурация настроена"
}

# Проверка testnet DEX
check_testnet_dex() {
    print_testnet "Проверка доступности Testnet DEX..."
    
    # Проверка основных DEX на testnet
    DEX_ENDPOINTS=(
        "https://testnet-api.raydium.io"
        "https://testnet-api.orca.so"
        "https://testnet-api.jup.ag"
    )
    
    AVAILABLE_DEX=()
    
    for endpoint in "${DEX_ENDPOINTS[@]}"; do
        if curl -s --connect-timeout 5 "$endpoint" > /dev/null 2>&1; then
            DEX_NAME=$(echo "$endpoint" | sed 's|https://testnet-api\.||' | sed 's|\.so||' | sed 's|\.io||' | sed 's|\.ag||')
            AVAILABLE_DEX+=("$DEX_NAME")
            print_success "DEX $DEX_NAME доступен"
        else
            DEX_NAME=$(echo "$endpoint" | sed 's|https://testnet-api\.||' | sed 's|\.so||' | sed 's|\.io||' | sed 's|\.ag||')
            print_warning "DEX $DEX_NAME недоступен"
        fi
    done
    
    if [ ${#AVAILABLE_DEX[@]} -eq 0 ]; then
        print_warning "Ни один DEX не доступен на testnet. Используем симуляцию."
    else
        print_success "Доступно DEX: ${AVAILABLE_DEX[*]}"
    fi
}

# Запуск testnet торговли
start_testnet_trading() {
    print_testnet "Запуск торговли на Solana Testnet..."
    
    # Создание директорий для логов
    mkdir -p logs
    
    print_info "Команда запуска: ./build/solana_arbitrage_bot --config config.json --dry-run --verbose"
    
    # Запуск бота в testnet режиме (используем dry-run для безопасности)
    ./build/solana_arbitrage_bot --config config.json --dry-run --verbose
}

# Мониторинг testnet
monitor_testnet() {
    print_testnet "Запуск мониторинга Testnet..."
    
    # Запуск мониторинга в фоне
    (
        while true; do
            sleep 30
            
            # Проверка работы бота
            if ! pgrep -f "solana_arbitrage_bot" > /dev/null; then
                print_error "Testnet бот остановлен!"
                break
            fi
            
            # Проверка баланса
            BALANCE=$(solana balance 2>/dev/null || echo "0")
            if [[ $(echo "$BALANCE < 0.01" | bc -l 2>/dev/null || echo "1") -eq 1 ]]; then
                print_warning "Низкий баланс testnet SOL: $BALANCE"
            fi
            
            # Проверка логов на ошибки
            if [ -f "logs/testnet_trading.log" ] && grep -q "CRITICAL\|FATAL" logs/testnet_trading.log; then
                print_error "Обнаружены критические ошибки в testnet логах!"
            fi
        done
    ) &
    
    MONITOR_PID=$!
    print_success "Testnet мониторинг запущен (PID: $MONITOR_PID)"
}

# Обработка сигналов
cleanup() {
    print_testnet "Получен сигнал остановки..."
    
    # Остановка бота
    if pgrep -f "solana_arbitrage_bot" > /dev/null; then
        print_info "Остановка testnet бота..."
        pkill -f "solana_arbitrage_bot"
        sleep 2
    fi
    
    # Остановка мониторинга
    if [ -n "$MONITOR_PID" ]; then
        print_info "Остановка testnet мониторинга..."
        kill $MONITOR_PID 2>/dev/null || true
    fi
    
    print_success "Testnet очистка завершена"
    exit 0
}

# Установка обработчиков сигналов
trap cleanup SIGINT SIGTERM

# Главная функция
main() {
    echo "🧪 Solana Arbitrage Bot - Testnet Trading Launcher"
    echo "=================================================="
    echo "🌐 Безопасное тестирование на Solana Testnet"
    echo "💰 Бесплатные тестовые токены"
    echo "🔧 Полная функциональность без риска"
    echo "=================================================="
    
    # Проверки и настройка
    check_prerequisites
    setup_solana_testnet
    create_testnet_wallet
    get_testnet_sol
    setup_testnet_config
    check_testnet_dex
    
    # Запуск мониторинга
    monitor_testnet
    
    # Запуск торговли
    start_testnet_trading
}

# Запуск главной функции
main "$@"
