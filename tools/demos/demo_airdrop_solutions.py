#!/usr/bin/env python3
"""
Demo Airdrop Solutions - Демонстрация всех решений для обхода 429 ошибок
"""

import subprocess
import time
import sys
import os

def print_header(title):
    print(f"\n{'='*60}")
    print(f"🧪 {title}")
    print(f"{'='*60}")

def print_step(step, description):
    print(f"\n{step}. {description}")

def run_command(command, description):
    print(f"\n🔄 {description}")
    print(f"Команда: {command}")
    
    try:
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"✅ Успешно: {result.stdout.strip()}")
            return True
        else:
            print(f"❌ Ошибка: {result.stderr.strip()}")
            return False
    except Exception as e:
        print(f"❌ Исключение: {e}")
        return False

def check_solana_cli():
    """Проверка Solana CLI"""
    print_step("1", "Проверка Solana CLI")
    
    if run_command("solana --version", "Проверка версии Solana"):
        print("✅ Solana CLI установлен")
        return True
    else:
        print("❌ Solana CLI не установлен")
        print("Установите: sh -c \"$(curl -sSfL https://release.solana.com/stable/install)\"")
        return False

def setup_testnet():
    """Настройка testnet"""
    print_step("2", "Настройка Solana Testnet")
    
    commands = [
        ("solana config set --url https://api.testnet.solana.com", "Переключение на testnet"),
        ("solana cluster-version", "Проверка подключения к testnet")
    ]
    
    for command, description in commands:
        if not run_command(command, description):
            return False
    
    return True

def check_wallet():
    """Проверка кошелька"""
    print_step("3", "Проверка testnet кошелька")
    
    if os.path.exists("testnet-wallet.json"):
        print("✅ Testnet кошелек найден")
        run_command("solana config set --keypair testnet-wallet.json", "Настройка кошелька")
        run_command("solana address", "Показ адреса кошелька")
        return True
    else:
        print("⚠️  Testnet кошелек не найден")
        if run_command("solana-keygen new --outfile testnet-wallet.json --no-bip39-passphrase", "Создание нового кошелька"):
            run_command("solana config set --keypair testnet-wallet.json", "Настройка кошелька")
            return True
        return False

def check_balance():
    """Проверка баланса"""
    print_step("4", "Проверка текущего баланса")
    
    result = subprocess.run("solana balance", shell=True, capture_output=True, text=True)
    if result.returncode == 0:
        balance = result.stdout.strip()
        print(f"💰 Текущий баланс: {balance}")
        
        # Парсинг баланса
        try:
            balance_value = float(balance.replace(" SOL", ""))
            if balance_value < 0.1:
                print("⚠️  Низкий баланс! Рекомендуется пополнение")
                return False
            else:
                print("✅ Достаточно SOL для тестирования")
                return True
        except:
            print("⚠️  Не удалось определить баланс")
            return False
    else:
        print("❌ Ошибка получения баланса")
        return False

def demo_smart_airdrop():
    """Демонстрация умного airdrop"""
    print_step("5", "Демонстрация Smart Airdrop")
    
    print("🧠 Smart Airdrop - Умное пополнение с защитой от 429")
    print("Особенности:")
    print("  ✅ Экспоненциальные задержки")
    print("  ✅ Ротация RPC endpoints")
    print("  ✅ Случайные User-Agent")
    print("  ✅ Throttling запросов")
    print("  ✅ Автоматические повторы")
    
    amount = input("\nВведите сумму для демонстрации (0.1-2.0): ").strip()
    if not amount:
        amount = "0.1"
    
    try:
        amount_float = float(amount)
        if 0.1 <= amount_float <= 2.0:
            run_command(f"python3 smart_airdrop.py {amount}", f"Smart Airdrop {amount} SOL")
        else:
            print("❌ Сумма должна быть от 0.1 до 2.0 SOL")
    except ValueError:
        print("❌ Неверный формат суммы")

def demo_improved_faucet():
    """Демонстрация улучшенного faucet"""
    print_step("6", "Демонстрация Improved Faucet")
    
    print("🔄 Improved Faucet - Улучшенный faucet с повторами")
    print("Особенности:")
    print("  ✅ Автоматические повторы")
    print("  ✅ Умные задержки")
    print("  ✅ Переключение RPC endpoints")
    
    amount = input("\nВведите сумму для демонстрации (0.1-2.0): ").strip()
    if not amount:
        amount = "0.1"
    
    try:
        amount_float = float(amount)
        if 0.1 <= amount_float <= 2.0:
            run_command(f"python3 testnet_faucet.py {amount}", f"Improved Faucet {amount} SOL")
        else:
            print("❌ Сумма должна быть от 0.1 до 2.0 SOL")
    except ValueError:
        print("❌ Неверный формат суммы")

def demo_quick_airdrop():
    """Демонстрация быстрого airdrop"""
    print_step("7", "Демонстрация Quick Airdrop")
    
    print("⚡ Quick Airdrop - Быстрый скрипт")
    print("Особенности:")
    print("  ✅ Простота использования")
    print("  ✅ Быстрое выполнение")
    print("  ✅ Автоматическая настройка")
    
    amount = input("\nВведите сумму для демонстрации (0.1-2.0): ").strip()
    if not amount:
        amount = "0.1"
    
    try:
        amount_float = float(amount)
        if 0.1 <= amount_float <= 2.0:
            run_command(f"./quick_airdrop.sh {amount}", f"Quick Airdrop {amount} SOL")
        else:
            print("❌ Сумма должна быть от 0.1 до 2.0 SOL")
    except ValueError:
        print("❌ Неверный формат суммы")

def show_web_alternatives():
    """Показ веб-альтернатив"""
    print_step("8", "Веб-альтернативы для обхода 429")
    
    print("🌐 Если все скрипты не работают, используйте веб-интерфейсы:")
    print("\n1. Официальный Solana Faucet:")
    print("   https://faucet.solana.com")
    print("   Лимит: 2 SOL за запрос")
    
    print("\n2. SolFaucet:")
    print("   https://solfaucet.com")
    print("   Лимит: 1 SOL за запрос")
    
    print("\n3. Discord боты:")
    print("   https://discord.gg/solana")
    print("   Канал: #testnet-faucet")
    print("   Команда: !airdrop <address>")
    
    print("\n4. Telegram боты:")
    print("   @SolanaFaucetBot")
    print("   @TestnetFaucetBot")

def show_troubleshooting():
    """Показ решения проблем"""
    print_step("9", "Решение проблем с 429 ошибками")
    
    print("🛠️  Частые проблемы и решения:")
    
    print("\n❌ HTTP 429 - Too Many Requests:")
    print("   Решение 1: Подождать 10 минут")
    print("   Решение 2: Использовать другой RPC endpoint")
    print("   Решение 3: Веб-интерфейс")
    
    print("\n❌ Rate limit exceeded:")
    print("   Решение: Увеличить интервалы между запросами")
    print("   Рекомендация: 20+ минут между запросами")
    
    print("\n❌ Network timeout:")
    print("   Решение 1: Проверить интернет-соединение")
    print("   Решение 2: Использовать VPN")
    print("   Решение 3: Попробовать другой RPC endpoint")
    
    print("\n💡 Общие рекомендации:")
    print("   • Не превышайте лимиты (2 SOL за запрос)")
    print("   • Соблюдайте интервалы (5+ минут)")
    print("   • Используйте разные User-Agent")
    print("   • Ротируйте RPC endpoints")

def show_statistics():
    """Показ статистики"""
    print_step("10", "Статистика и мониторинг")
    
    print("📊 Мониторинг успешности airdrop:")
    
    # Проверка логов
    if os.path.exists("airdrop.log"):
        print("\n📋 Анализ логов:")
        run_command("grep -c '429' airdrop.log", "Количество 429 ошибок")
        run_command("grep -c 'Успешный' airdrop.log", "Количество успешных airdrop")
        run_command("grep -c 'Timeout' airdrop.log", "Количество таймаутов")
    
    print("\n📈 Рекомендации по мониторингу:")
    print("   • Логируйте все запросы: python3 smart_airdrop.py 1.0 2>&1 | tee airdrop.log")
    print("   • Анализируйте статистику: python3 smart_airdrop.py")
    print("   • Мониторьте баланс: solana balance")
    print("   • Отслеживайте транзакции: solana transaction-history $(solana address)")

def main():
    print_header("Demo Airdrop Solutions - Демонстрация решений для обхода 429")
    
    print("🎯 Этот скрипт демонстрирует все способы обхода ошибки 429")
    print("   при пополнении Solana Testnet")
    
    # Проверки
    if not check_solana_cli():
        return
    
    if not setup_testnet():
        return
    
    if not check_wallet():
        return
    
    check_balance()
    
    # Демонстрации
    while True:
        print_header("Выберите демонстрацию")
        
        print("1. 🧠 Smart Airdrop (Рекомендуется)")
        print("2. 🔄 Improved Faucet")
        print("3. ⚡ Quick Airdrop")
        print("4. 🌐 Веб-альтернативы")
        print("5. 🛠️  Решение проблем")
        print("6. 📊 Статистика")
        print("0. Выход")
        
        choice = input("\nВыберите опцию (0-6): ").strip()
        
        if choice == "1":
            demo_smart_airdrop()
        elif choice == "2":
            demo_improved_faucet()
        elif choice == "3":
            demo_quick_airdrop()
        elif choice == "4":
            show_web_alternatives()
        elif choice == "5":
            show_troubleshooting()
        elif choice == "6":
            show_statistics()
        elif choice == "0":
            print("\n👋 До свидания!")
            break
        else:
            print("❌ Неверный выбор")
        
        input("\nНажмите Enter для продолжения...")

if __name__ == "__main__":
    main()
