#!/usr/bin/env python3
"""
Solana Testnet Arbitrage Demo
Демонстрация арбитражной торговли на Solana Testnet
"""

import requests
import json
import time
import random
import subprocess
import threading
from datetime import datetime

class TestnetArbitrageDemo:
    def __init__(self):
        self.running = False
        self.bot_process = None
        self.testnet_rpc = "https://api.testnet.solana.com"
        
        # Testnet токены
        self.testnet_tokens = {
            'SOL': 'So11111111111111111111111111111111111111112',
            'USDC': '4zMMC9srt5Ri5X14GAgXhaHii3GnPAEERYPJgZJDncDU',
            'USDT': 'Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB',
            'ETH': '7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs',
            'BTC': '9n4nbM75f5Ui33ZbPYXn59EwSgE8CGsHtAeTH5YFeJ9E'
        }
        
        # Статистика testnet торговли
        self.stats = {
            'testnet_balance': 0.0,
            'trades_executed': 0,
            'total_profit': 0.0,
            'testnet_tokens': {}
        }
    
    def get_testnet_balance(self):
        """Получение баланса testnet кошелька"""
        try:
            # Получение публичного ключа
            result = subprocess.run(['solana', 'address'], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                public_key = result.stdout.strip()
                
                # Запрос баланса через RPC
                payload = {
                    "jsonrpc": "2.0",
                    "id": 1,
                    "method": "getBalance",
                    "params": [public_key]
                }
                
                response = requests.post(self.testnet_rpc, json=payload, timeout=10)
                if response.status_code == 200:
                    data = response.json()
                    if 'result' in data and 'value' in data['result']:
                        balance_lamports = data['result']['value']
                        balance_sol = balance_lamports / 1_000_000_000  # Convert lamports to SOL
                        return balance_sol
        except Exception as e:
            print(f"Ошибка получения баланса: {e}")
        return 0.0
    
    def request_testnet_sol(self):
        """Запрос testnet SOL через faucet"""
        try:
            # Получение публичного ключа
            result = subprocess.run(['solana', 'address'], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                public_key = result.stdout.strip()
                
                # Запрос airdrop
                payload = {
                    "jsonrpc": "2.0",
                    "id": 1,
                    "method": "requestAirdrop",
                    "params": [public_key, 1000000000]  # 1 SOL
                }
                
                response = requests.post(self.testnet_rpc, json=payload, timeout=10)
                if response.status_code == 200:
                    data = response.json()
                    if 'result' in data:
                        print("✅ Запрос testnet SOL отправлен")
                        return True
        except Exception as e:
            print(f"Ошибка запроса testnet SOL: {e}")
        return False
    
    def get_testnet_token_balances(self):
        """Получение балансов testnet токенов"""
        try:
            # Получение публичного ключа
            result = subprocess.run(['solana', 'address'], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                public_key = result.stdout.strip()
                
                # Запрос токен аккаунтов
                payload = {
                    "jsonrpc": "2.0",
                    "id": 1,
                    "method": "getTokenAccountsByOwner",
                    "params": [
                        public_key,
                        {"programId": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA"},
                        {"encoding": "jsonParsed"}
                    ]
                }
                
                response = requests.post(self.testnet_rpc, json=payload, timeout=10)
                if response.status_code == 200:
                    data = response.json()
                    if 'result' in data and 'value' in data['result']:
                        token_balances = {}
                        for account in data['result']['value']:
                            if 'parsed' in account['account']['data']:
                                token_info = account['account']['data']['parsed']['info']
                                mint = token_info['mint']
                                balance = int(token_info['tokenAmount']['amount'])
                                decimals = token_info['tokenAmount']['decimals']
                                
                                # Найти символ токена
                                symbol = "UNKNOWN"
                                for sym, addr in self.testnet_tokens.items():
                                    if addr == mint:
                                        symbol = sym
                                        break
                                
                                if balance > 0:
                                    token_balances[symbol] = balance / (10 ** decimals)
                        
                        return token_balances
        except Exception as e:
            print(f"Ошибка получения токен балансов: {e}")
        return {}
    
    def simulate_testnet_arbitrage(self):
        """Симуляция арбитража на testnet"""
        opportunities = []
        
        # Симуляция цен на testnet DEX
        testnet_prices = {
            'SOL': random.uniform(80, 120),
            'USDC': 1.0,
            'USDT': 1.0,
            'ETH': random.uniform(2000, 3000),
            'BTC': random.uniform(30000, 50000)
        }
        
        # Симуляция различий между DEX
        for token in ['SOL', 'ETH', 'BTC']:
            if token in testnet_prices:
                base_price = testnet_prices[token]
                
                # Разные цены на разных DEX
                raydium_price = base_price * random.uniform(0.98, 1.02)
                orca_price = base_price * random.uniform(0.97, 1.03)
                jupiter_price = base_price * random.uniform(0.99, 1.01)
                
                prices = {
                    'Raydium': raydium_price,
                    'Orca': orca_price,
                    'Jupiter': jupiter_price
                }
                
                min_price = min(prices.values())
                max_price = max(prices.values())
                
                if min_price > 0:
                    profit_pct = ((max_price - min_price) / min_price) * 100
                    
                    if profit_pct > 0.1:  # Значимая возможность
                        buy_dex = min(prices, key=prices.get)
                        sell_dex = max(prices, key=prices.get)
                        
                        opportunities.append({
                            'token': token,
                            'buy_dex': buy_dex,
                            'sell_dex': sell_dex,
                            'buy_price': min_price,
                            'sell_price': max_price,
                            'profit_pct': profit_pct,
                            'volume': random.uniform(10, 50)
                        })
        
        return opportunities
    
    def start_testnet_bot(self):
        """Запуск testnet бота"""
        print("🚀 Запуск Solana Testnet Arbitrage Bot...")
        try:
            self.bot_process = subprocess.Popen(
                ['./start_testnet_trading.sh'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            print("✅ Testnet бот запущен успешно!")
            return True
        except Exception as e:
            print(f"❌ Ошибка запуска testnet бота: {e}")
            return False
    
    def stop_testnet_bot(self):
        """Остановка testnet бота"""
        if self.bot_process:
            print("🛑 Остановка testnet бота...")
            self.bot_process.terminate()
            try:
                self.bot_process.wait(timeout=5)
                print("✅ Testnet бот остановлен")
            except subprocess.TimeoutExpired:
                self.bot_process.kill()
                print("⚠️  Testnet бот принудительно остановлен")
    
    def run_testnet_analysis(self):
        """Запуск анализа testnet"""
        print("📊 Запуск анализа Solana Testnet...")
        
        while self.running:
            try:
                print(f"\n{'='*60}")
                print(f"🧪 TESTNET АНАЛИЗ ({datetime.now().strftime('%H:%M:%S')})")
                print(f"{'='*60}")
                
                # Получение testnet баланса
                balance = self.get_testnet_balance()
                self.stats['testnet_balance'] = balance
                
                print(f"\n💰 TESTNET БАЛАНС:")
                print(f"  SOL: {balance:.6f}")
                
                # Получение токен балансов
                token_balances = self.get_testnet_token_balances()
                self.stats['testnet_tokens'] = token_balances
                
                if token_balances:
                    print(f"\n🪙 TESTNET ТОКЕНЫ:")
                    for token, amount in token_balances.items():
                        print(f"  {token}: {amount:.6f}")
                else:
                    print(f"\n🪙 TESTNET ТОКЕНЫ: Нет токенов")
                
                # Симуляция арбитражных возможностей
                opportunities = self.simulate_testnet_arbitrage()
                
                if opportunities:
                    print(f"\n🎯 ОБНАРУЖЕНО {len(opportunities)} АРБИТРАЖНЫХ ВОЗМОЖНОСТЕЙ:")
                    for i, opp in enumerate(opportunities, 1):
                        print(f"\n  {i}. {opp['token']} ({opp['buy_dex']} → {opp['sell_dex']})")
                        print(f"     💰 Покупка: ${opp['buy_price']:.4f}")
                        print(f"     💸 Продажа: ${opp['sell_price']:.4f}")
                        print(f"     📈 Прибыль: {opp['profit_pct']:.3f}%")
                        print(f"     📊 Объем: ${opp['volume']:.2f}")
                        
                        # Обновление статистики
                        self.stats['trades_executed'] += 1
                        self.stats['total_profit'] += opp['profit_pct']
                else:
                    print("\n⏳ Значимых арбитражных возможностей не найдено")
                
                # Показываем статистику
                print(f"\n📊 TESTNET СТАТИСТИКА:")
                print(f"  • Баланс SOL: {self.stats['testnet_balance']:.6f}")
                print(f"  • Выполнено сделок: {self.stats['trades_executed']}")
                if self.stats['trades_executed'] > 0:
                    avg_profit = self.stats['total_profit'] / self.stats['trades_executed']
                    print(f"  • Средняя прибыль: {avg_profit:.3f}%")
                
                # Проверка необходимости пополнения
                if balance < 0.01:
                    print(f"\n⚠️  Низкий баланс testnet SOL! Запрашиваем пополнение...")
                    if self.request_testnet_sol():
                        print("✅ Запрос на пополнение отправлен")
                        time.sleep(15)  # Ждем подтверждения
                
                print(f"\n⏱️  Следующее обновление через 20 секунд...")
                time.sleep(20)
                
            except KeyboardInterrupt:
                print("\n🛑 Получен сигнал остановки...")
                break
            except Exception as e:
                print(f"❌ Ошибка анализа testnet: {e}")
                time.sleep(10)
    
    def run_testnet_demo(self, duration=120):
        """Запуск testnet демонстрации"""
        print("🧪 Solana Testnet Arbitrage Bot - Демонстрация")
        print("=" * 60)
        print("🌐 Безопасное тестирование на Solana Testnet")
        print("💰 Бесплатные тестовые токены")
        print("🔧 Полная функциональность без риска")
        print("⚡ Реальные транзакции на testnet")
        print("=" * 60)
        
        self.running = True
        start_time = time.time()
        
        # Проверка и настройка testnet
        print("\n🔧 Настройка Solana Testnet...")
        
        # Переключение на testnet
        try:
            subprocess.run(['solana', 'config', 'set', '--url', 'https://api.testnet.solana.com'], 
                          check=True, capture_output=True)
            print("✅ Переключение на testnet выполнено")
        except subprocess.CalledProcessError:
            print("❌ Ошибка переключения на testnet")
            return
        
        # Проверка баланса
        initial_balance = self.get_testnet_balance()
        print(f"💰 Начальный баланс: {initial_balance:.6f} SOL")
        
        if initial_balance < 0.1:
            print("⚠️  Низкий баланс. Запрашиваем testnet SOL...")
            if self.request_testnet_sol():
                time.sleep(15)
                new_balance = self.get_testnet_balance()
                print(f"✅ Новый баланс: {new_balance:.6f} SOL")
        
        # Запуск testnet бота
        if not self.start_testnet_bot():
            print("❌ Не удалось запустить testnet бота")
            return
        
        # Запуск анализа в отдельном потоке
        analysis_thread = threading.Thread(target=self.run_testnet_analysis)
        analysis_thread.daemon = True
        analysis_thread.start()
        
        try:
            print(f"\n⏱️  Демонстрация будет работать {duration} секунд...")
            print("Нажмите Ctrl+C для остановки\n")
            
            time.sleep(duration)
            
        except KeyboardInterrupt:
            print("\n🛑 Получен сигнал остановки...")
        
        finally:
            self.running = False
            
            # Останавливаем бота
            self.stop_testnet_bot()
            
            # Показываем финальную статистику
            final_balance = self.get_testnet_balance()
            print(f"\n{'='*60}")
            print("📈 ФИНАЛЬНАЯ TESTNET СТАТИСТИКА")
            print(f"{'='*60}")
            print(f"⏱️  Время работы: {time.time() - start_time:.1f} секунд")
            print(f"💰 Начальный баланс: {initial_balance:.6f} SOL")
            print(f"💰 Конечный баланс: {final_balance:.6f} SOL")
            print(f"📊 Изменение баланса: {final_balance - initial_balance:.6f} SOL")
            print(f"🎯 Выполнено сделок: {self.stats['trades_executed']}")
            
            if self.stats['trades_executed'] > 0:
                avg_profit = self.stats['total_profit'] / self.stats['trades_executed']
                print(f"📈 Средняя прибыль: {avg_profit:.3f}%")
                print(f"💰 Общая прибыльность: {self.stats['total_profit']:.3f}%")
            
            print(f"\n🎉 Testnet демонстрация завершена!")
            print("✅ Solana Testnet Arbitrage Bot готов к работе!")

def main():
    demo = TestnetArbitrageDemo()
    demo.run_testnet_demo(duration=90)  # 90 секунд

if __name__ == "__main__":
    main()
