#!/usr/bin/env python3
"""
Solana Testnet Faucet - Interactive Deposit Tool
Интерактивный инструмент для пополнения Solana Testnet
"""

import requests
import json
import time
import subprocess
import sys
from datetime import datetime

class TestnetFaucet:
    def __init__(self):
        # Множественные RPC endpoints для обхода ограничений
        self.testnet_rpc_endpoints = [
            "https://api.testnet.solana.com",
            "https://testnet.solana.rpcpool.com",
            "https://solana-testnet.rpc.extrnode.com",
            "https://rpc.testnet.solana.com"
        ]
        self.current_rpc_index = 0
        self.testnet_rpc = self.testnet_rpc_endpoints[0]
        self.faucet_url = "https://faucet.solana.com"
        self.wallet_file = "testnet-wallet.json"
        
        # Доступные суммы для airdrop
        self.available_amounts = {
            "1": 1.0,
            "2": 2.0,
            "5": 5.0,
            "10": 10.0,
            "custom": "custom"
        }
        
        # Лимиты faucet
        self.max_airdrop_per_request = 2.0  # SOL
        self.max_airdrop_per_day = 10.0     # SOL
        
    def get_wallet_address(self):
        """Получение адреса кошелька"""
        try:
            result = subprocess.run(['solana', 'address'], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip()
            else:
                print("❌ Ошибка получения адреса кошелька")
                return None
        except Exception as e:
            print(f"❌ Ошибка: {e}")
            return None
    
    def get_current_balance(self):
        """Получение текущего баланса"""
        try:
            address = self.get_wallet_address()
            if not address:
                return 0.0
            
            payload = {
                "jsonrpc": "2.0",
                "id": 1,
                "method": "getBalance",
                "params": [address]
            }
            
            response = requests.post(self.testnet_rpc, json=payload, timeout=10)
            if response.status_code == 200:
                data = response.json()
                if 'result' in data and 'value' in data['result']:
                    balance_lamports = data['result']['value']
                    balance_sol = balance_lamports / 1_000_000_000
                    return balance_sol
        except Exception as e:
            print(f"❌ Ошибка получения баланса: {e}")
        return 0.0
    
    def request_airdrop(self, amount, max_retries=3):
        """Запрос airdrop через RPC с защитой от 429"""
        address = self.get_wallet_address()
        if not address:
            return False
        
        # Конвертация SOL в lamports
        lamports = int(amount * 1_000_000_000)
        
        payload = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "requestAirdrop",
            "params": [address, lamports]
        }
        
        print(f"🔄 Запрос {amount} SOL на адрес: {address}")
        
        for attempt in range(max_retries):
            try:
                # Добавляем случайную задержку перед запросом
                if attempt > 0:
                    delay = 5 + (attempt * 10) + (hash(str(time.time())) % 30)
                    print(f"⏳ Повтор {attempt + 1}/{max_retries}. Ожидание {delay} секунд...")
                    time.sleep(delay)
                
                # Пробуем разные RPC endpoints при ошибках
                if attempt > 0 and attempt % 2 == 0:
                    self.switch_rpc_endpoint()
                
                response = requests.post(self.testnet_rpc, json=payload, timeout=30)
                
                if response.status_code == 200:
                    data = response.json()
                    if 'result' in data:
                        signature = data['result']
                        print(f"✅ Airdrop запрошен! Подпись: {signature}")
                        return True
                    elif 'error' in data:
                        error_msg = data['error'].get('message', 'Unknown error')
                        error_code = data['error'].get('code', 0)
                        
                        if error_code == -32005:  # Rate limit error
                            print(f"⚠️  Rate limit (429). Повтор через 30 секунд...")
                            time.sleep(30)
                            continue
                        else:
                            print(f"❌ Ошибка airdrop: {error_msg}")
                            return False
                
                elif response.status_code == 429:
                    print(f"⚠️  HTTP 429 - Too Many Requests. Повтор {attempt + 1}/{max_retries}")
                    if attempt < max_retries - 1:
                        delay = 60 + (attempt * 30)  # Увеличиваем задержку с каждым повтором
                        print(f"⏳ Ожидание {delay} секунд перед повтором...")
                        time.sleep(delay)
                    continue
                
                else:
                    print(f"❌ HTTP ошибка {response.status_code}: {response.text}")
                    if attempt < max_retries - 1:
                        time.sleep(10)
                    continue
                    
            except requests.exceptions.Timeout:
                print(f"⚠️  Таймаут запроса. Повтор {attempt + 1}/{max_retries}")
                if attempt < max_retries - 1:
                    time.sleep(15)
                continue
                
            except requests.exceptions.ConnectionError:
                print(f"⚠️  Ошибка подключения. Повтор {attempt + 1}/{max_retries}")
                if attempt < max_retries - 1:
                    time.sleep(20)
                continue
                
            except Exception as e:
                print(f"❌ Неожиданная ошибка: {e}")
                if attempt < max_retries - 1:
                    time.sleep(10)
                continue
        
        print(f"❌ Не удалось выполнить airdrop после {max_retries} попыток")
        return False
    
    def switch_rpc_endpoint(self):
        """Переключение на следующий RPC endpoint"""
        self.current_rpc_index = (self.current_rpc_index + 1) % len(self.testnet_rpc_endpoints)
        self.testnet_rpc = self.testnet_rpc_endpoints[self.current_rpc_index]
        print(f"🔄 Переключение на RPC: {self.testnet_rpc}")
    
    def test_rpc_connection(self, endpoint):
        """Тестирование подключения к RPC endpoint"""
        try:
            payload = {
                "jsonrpc": "2.0",
                "id": 1,
                "method": "getHealth"
            }
            response = requests.post(endpoint, json=payload, timeout=10)
            return response.status_code == 200
        except:
            return False
    
    def wait_for_confirmation(self, timeout=60):
        """Ожидание подтверждения транзакции"""
        print("⏳ Ожидание подтверждения транзакции...")
        
        start_time = time.time()
        initial_balance = self.get_current_balance()
        
        while time.time() - start_time < timeout:
            time.sleep(2)
            current_balance = self.get_current_balance()
            
            if current_balance > initial_balance:
                print(f"✅ Транзакция подтверждена! Новый баланс: {current_balance:.6f} SOL")
                return True
        
        print("⚠️  Таймаут ожидания подтверждения")
        return False
    
    def show_balance_info(self):
        """Показ информации о балансе"""
        balance = self.get_current_balance()
        address = self.get_wallet_address()
        
        print(f"\n💰 ИНФОРМАЦИЯ О КОШЕЛЬКЕ:")
        print(f"   Адрес: {address}")
        print(f"   Баланс: {balance:.6f} SOL")
        
        if balance < 0.01:
            print("   ⚠️  Низкий баланс! Рекомендуется пополнение")
        elif balance < 0.1:
            print("   ⚠️  Баланс достаточен для тестирования")
        else:
            print("   ✅ Баланс достаточен для активной торговли")
        
        return balance
    
    def show_available_amounts(self):
        """Показ доступных сумм"""
        print(f"\n💸 ДОСТУПНЫЕ СУММЫ ДЛЯ ПОПОЛНЕНИЯ:")
        for key, value in self.available_amounts.items():
            if key == "custom":
                print(f"   {key}. Ввести произвольную сумму")
            else:
                print(f"   {key}. {value} SOL")
        
        print(f"\n📋 ЛИМИТЫ FAUCET:")
        print(f"   Максимум за запрос: {self.max_airdrop_per_request} SOL")
        print(f"   Максимум в день: {self.max_airdrop_per_day} SOL")
    
    def get_user_amount(self):
        """Получение суммы от пользователя"""
        while True:
            try:
                choice = input(f"\n🎯 Выберите сумму для пополнения (1-10, custom): ").strip().lower()
                
                if choice in self.available_amounts:
                    if choice == "custom":
                        while True:
                            try:
                                custom_amount = float(input("Введите произвольную сумму SOL: "))
                                if 0.1 <= custom_amount <= self.max_airdrop_per_request:
                                    return custom_amount
                                else:
                                    print(f"❌ Сумма должна быть от 0.1 до {self.max_airdrop_per_request} SOL")
                            except ValueError:
                                print("❌ Введите корректное число")
                    else:
                        return self.available_amounts[choice]
                else:
                    print("❌ Неверный выбор. Попробуйте снова.")
            except KeyboardInterrupt:
                print("\n\n🛑 Операция отменена пользователем")
                sys.exit(0)
    
    def multiple_airdrop(self, total_amount):
        """Множественный airdrop для больших сумм"""
        if total_amount <= self.max_airdrop_per_request:
            return self.request_airdrop(total_amount)
        
        print(f"🔄 Большая сумма. Выполняем множественный airdrop...")
        
        remaining = total_amount
        success_count = 0
        
        while remaining > 0:
            current_amount = min(remaining, self.max_airdrop_per_request)
            
            print(f"📤 Запрос {current_amount:.1f} SOL...")
            
            if self.request_airdrop(current_amount):
                success_count += 1
                remaining -= current_amount
                
                # Ожидание между запросами
                if remaining > 0:
                    print("⏳ Ожидание 5 секунд перед следующим запросом...")
                    time.sleep(5)
            else:
                print("❌ Ошибка airdrop. Прерывание.")
                break
        
        if remaining == 0:
            print(f"✅ Успешно запрошено {total_amount} SOL в {success_count} транзакциях")
            return True
        else:
            print(f"⚠️  Запрошено {total_amount - remaining} SOL из {total_amount}")
            return False
    
    def run_interactive_faucet(self):
        """Запуск интерактивного faucet"""
        print("🧪 Solana Testnet Faucet - Интерактивное пополнение")
        print("=" * 60)
        print("💰 Получите бесплатные testnet SOL для торговли")
        print("🔧 Автоматическая настройка и пополнение")
        print("=" * 60)
        
        # Проверка и настройка Solana CLI
        print("\n🔧 Проверка Solana CLI...")
        
        try:
            # Переключение на testnet
            subprocess.run(['solana', 'config', 'set', '--url', 'https://api.testnet.solana.com'], 
                          check=True, capture_output=True)
            print("✅ Переключение на testnet выполнено")
        except subprocess.CalledProcessError:
            print("❌ Ошибка переключения на testnet")
            return
        
        # Проверка кошелька
        if not self.get_wallet_address():
            print("❌ Кошелек не настроен. Запустите: ./start_testnet_trading.sh")
            return
        
        # Показ текущего баланса
        current_balance = self.show_balance_info()
        
        # Показ доступных сумм
        self.show_available_amounts()
        
        # Получение суммы от пользователя
        amount = self.get_user_amount()
        
        print(f"\n🎯 Выбрана сумма: {amount} SOL")
        
        # Подтверждение
        confirm = input("Подтвердить запрос? (y/N): ").strip().lower()
        if confirm not in ['y', 'yes']:
            print("❌ Операция отменена")
            return
        
        # Выполнение airdrop
        print(f"\n🚀 Выполнение airdrop {amount} SOL...")
        
        if self.multiple_airdrop(amount):
            # Ожидание подтверждения
            if self.wait_for_confirmation():
                # Показ нового баланса
                new_balance = self.get_current_balance()
                print(f"\n🎉 ПОПОЛНЕНИЕ УСПЕШНО!")
                print(f"   Старый баланс: {current_balance:.6f} SOL")
                print(f"   Новый баланс: {new_balance:.6f} SOL")
                print(f"   Пополнено: {new_balance - current_balance:.6f} SOL")
                
                # Рекомендации
                if new_balance >= 1.0:
                    print(f"\n✅ Отлично! Теперь можно запускать торговлю:")
                    print(f"   ./start_testnet_trading.sh")
                elif new_balance >= 0.1:
                    print(f"\n⚠️  Баланс достаточен для тестирования")
                    print(f"   Для активной торговли рекомендуется больше SOL")
                else:
                    print(f"\n⚠️  Низкий баланс. Попробуйте еще раз")
            else:
                print("❌ Ошибка подтверждения транзакции")
        else:
            print("❌ Ошибка выполнения airdrop")
    
    def quick_airdrop(self, amount=1.0):
        """Быстрый airdrop без интерактивности"""
        print(f"🚀 Быстрый airdrop {amount} SOL...")
        
        if self.request_airdrop(amount):
            if self.wait_for_confirmation():
                new_balance = self.get_current_balance()
                print(f"✅ Airdrop успешен! Баланс: {new_balance:.6f} SOL")
                return True
        
        print("❌ Airdrop не удался")
        return False

def main():
    faucet = TestnetFaucet()
    
    # Проверка аргументов командной строки
    if len(sys.argv) > 1:
        try:
            amount = float(sys.argv[1])
            faucet.quick_airdrop(amount)
        except ValueError:
            print("❌ Неверный формат суммы")
            print("Использование: python3 testnet_faucet.py [amount]")
    else:
        # Интерактивный режим
        faucet.run_interactive_faucet()

if __name__ == "__main__":
    main()
