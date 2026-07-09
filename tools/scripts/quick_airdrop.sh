#!/bin/bash

# Quick Airdrop Script for Solana Testnet
# Быстрый скрипт пополнения Solana Testnet

set -e

# Цвета
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

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

# Функция показа помощи
show_help() {
    echo "🧪 Solana Testnet Quick Airdrop"
    echo "================================"
    echo ""
    echo "Использование:"
    echo "  $0 [amount]"
    echo ""
    echo "Аргументы:"
    echo "  amount    Сумма SOL для пополнения (по умолчанию: 1)"
    echo ""
    echo "Примеры:"
    echo "  $0         # Пополнить 1 SOL"
    echo "  $0 2       # Пополнить 2 SOL"
    echo "  $0 0.5     # Пополнить 0.5 SOL"
    echo ""
    echo "Альтернативы:"
    echo "  python3 testnet_faucet.py    # Интерактивный режим"
    echo "  ./start_testnet_trading.sh   # Полная настройка"
}

# Проверка аргументов
AMOUNT=${1:-1}

# Валидация суммы
if ! [[ "$AMOUNT" =~ ^[0-9]+\.?[0-9]*$ ]] || (( $(echo "$AMOUNT <= 0" | bc -l) )); then
    print_error "Неверная сумма: $AMOUNT"
    show_help
    exit 1
fi

if (( $(echo "$AMOUNT > 2" | bc -l) )); then
    print_warning "Сумма больше 2 SOL. Будет выполнено несколько запросов."
fi

# Проверка Solana CLI
if ! command -v solana &> /dev/null; then
    print_error "Solana CLI не установлен"
    echo "Установите: sh -c \"\$(curl -sSfL https://release.solana.com/stable/install)\""
    exit 1
fi

# Переключение на testnet
print_info "Переключение на Solana Testnet..."
solana config set --url https://api.testnet.solana.com

# Проверка подключения
if ! solana cluster-version > /dev/null 2>&1; then
    print_error "Не удается подключиться к Solana Testnet"
    exit 1
fi

print_success "Подключение к Testnet установлено"

# Проверка кошелька
if [ ! -f "testnet-wallet.json" ]; then
    print_warning "Testnet кошелек не найден. Создаем новый..."
    solana-keygen new --outfile testnet-wallet.json --no-bip39-passphrase
    solana config set --keypair testnet-wallet.json
    print_success "Создан новый testnet кошелек"
else
    solana config set --keypair testnet-wallet.json
    print_success "Используется существующий testnet кошелек"
fi

# Получение адреса
ADDRESS=$(solana address)
print_info "Адрес кошелька: $ADDRESS"

# Показ текущего баланса
CURRENT_BALANCE=$(solana balance 2>/dev/null || echo "0")
print_info "Текущий баланс: $CURRENT_BALANCE SOL"

# Выполнение airdrop
print_info "Запрос $AMOUNT SOL через faucet..."

if python3 testnet_faucet.py "$AMOUNT"; then
    print_success "Airdrop выполнен успешно!"
    
    # Показ нового баланса
    NEW_BALANCE=$(solana balance 2>/dev/null || echo "0")
    print_info "Новый баланс: $NEW_BALANCE SOL"
    
    # Рекомендации
    if (( $(echo "$NEW_BALANCE >= 1" | bc -l) )); then
        print_success "Отлично! Теперь можно запускать торговлю:"
        echo "  ./start_testnet_trading.sh"
    elif (( $(echo "$NEW_BALANCE >= 0.1" | bc -l) )); then
        print_warning "Баланс достаточен для тестирования"
        echo "  Для активной торговли рекомендуется больше SOL"
    else
        print_warning "Низкий баланс. Попробуйте еще раз"
    fi
else
    print_error "Airdrop не удался"
    echo ""
    echo "Альтернативные способы пополнения:"
    echo "1. Интерактивный режим: python3 testnet_faucet.py"
    echo "2. Веб-интерфейс: https://faucet.solana.com"
    echo "3. Полная настройка: ./start_testnet_trading.sh"
    exit 1
fi
