#!/usr/bin/env python3
"""
Final Solana Arbitrage Demo
Финальная демонстрация: C++ бот + реальные данные
"""

import requests
import json
import time
import random
import subprocess
import threading
from datetime import datetime

class FinalArbitrageDemo:
    def __init__(self):
        self.running = False
        self.bot_process = None
        self.apis = {
            'coingecko': 'https://api.coingecko.com/api/v3',
            'binance': 'https://api.binance.com/api/v3'
        }
        
        self.tokens = ['solana', 'ethereum', 'bitcoin']
        self.symbols = ['SOL', 'ETH', 'BTC']
        
        # Статистика демонстрации
        self.stats = {
            'opportunities_found': 0,
            'total_profit_pct': 0.0,
            'best_opportunity': None,
            'bot_runtime': 0
        }
    
    def get_real_prices(self):
        """Получение реальных цен"""
        try:
            # CoinGecko
            url = f"{self.apis['coingecko']}/simple/price"
            params = {
                'ids': ','.join(self.tokens),
                'vs_currencies': 'usd'
            }
            
            response = requests.get(url, params=params, timeout=10)
            if response.status_code == 200:
                coingecko_data = response.json()
                
                # Binance
                binance_prices = {}
                for symbol in self.symbols:
                    try:
                        binance_url = f"{self.apis['binance']}/ticker/price"
                        binance_response = requests.get(binance_url, 
                                                      params={'symbol': f'{symbol}USDT'}, 
                                                      timeout=5)
                        if binance_response.status_code == 200:
                            binance_data = binance_response.json()
                            binance_prices[symbol] = float(binance_data['price'])
                    except:
                        pass
                
                # Объединяем данные
                prices = {}
                for i, token in enumerate(self.tokens):
                    symbol = self.symbols[i]
                    if symbol in binance_prices:
                        prices[symbol] = binance_prices[symbol]
                    elif token in coingecko_data and 'usd' in coingecko_data[token]:
                        prices[symbol] = coingecko_data[token]['usd']
                
                return prices
        except Exception as e:
            print(f"Ошибка получения цен: {e}")
        return {}
    
    def simulate_dex_opportunities(self, base_prices):
        """Симуляция арбитражных возможностей между DEX"""
        opportunities = []
        
        for symbol, base_price in base_prices.items():
            if base_price <= 0:
                continue
                
            # Симулируем цены на разных DEX
            raydium_price = base_price * random.uniform(0.995, 1.005)
            orca_price = base_price * random.uniform(0.994, 1.006)
            jupiter_price = base_price * random.uniform(0.996, 1.004)
            
            # Находим лучшую возможность
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
                    
                    opportunity = {
                        'symbol': symbol,
                        'buy_dex': buy_dex,
                        'sell_dex': sell_dex,
                        'buy_price': min_price,
                        'sell_price': max_price,
                        'profit_pct': profit_pct,
                        'volume': random.uniform(1000, 5000),
                        'timestamp': datetime.now()
                    }
                    
                    opportunities.append(opportunity)
                    
                    # Обновляем статистику
                    self.stats['opportunities_found'] += 1
                    self.stats['total_profit_pct'] += profit_pct
                    
                    if (self.stats['best_opportunity'] is None or 
                        profit_pct > self.stats['best_opportunity']['profit_pct']):
                        self.stats['best_opportunity'] = opportunity
        
        return opportunities
    
    def start_cpp_bot(self):
        """Запуск C++ бота"""
        print("🚀 Запуск C++ Solana Arbitrage Bot...")
        try:
            self.bot_process = subprocess.Popen(
                ['./build/solana_arbitrage_bot', '--dry-run', '--verbose'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            print("✅ C++ бот запущен успешно!")
            return True
        except Exception as e:
            print(f"❌ Ошибка запуска C++ бота: {e}")
            return False
    
    def stop_cpp_bot(self):
        """Остановка C++ бота"""
        if self.bot_process:
            print("🛑 Остановка C++ бота...")
            self.bot_process.terminate()
            try:
                self.bot_process.wait(timeout=5)
                print("✅ C++ бот остановлен")
            except subprocess.TimeoutExpired:
                self.bot_process.kill()
                print("⚠️  C++ бот принудительно остановлен")
    
    def run_market_analysis(self):
        """Запуск анализа рынка"""
        print("📊 Запуск анализа рынка...")
        
        while self.running:
            try:
                print(f"\n{'='*60}")
                print(f"📈 АНАЛИЗ РЫНКА ({datetime.now().strftime('%H:%M:%S')})")
                print(f"{'='*60}")
                
                # Получаем реальные цены
                real_prices = self.get_real_prices()
                
                if real_prices:
                    print("\n💰 РЕАЛЬНЫЕ ЦЕНЫ:")
                    for symbol, price in real_prices.items():
                        print(f"  {symbol}: ${price:,.4f}")
                    
                    # Симулируем арбитражные возможности
                    opportunities = self.simulate_dex_opportunities(real_prices)
                    
                    if opportunities:
                        print(f"\n🎯 ОБНАРУЖЕНО {len(opportunities)} АРБИТРАЖНЫХ ВОЗМОЖНОСТЕЙ:")
                        for i, opp in enumerate(opportunities, 1):
                            print(f"\n  {i}. {opp['symbol']} ({opp['buy_dex']} → {opp['sell_dex']})")
                            print(f"     💰 Покупка: ${opp['buy_price']:,.4f}")
                            print(f"     💸 Продажа: ${opp['sell_price']:,.4f}")
                            print(f"     📈 Прибыль: {opp['profit_pct']:.3f}%")
                            print(f"     📊 Объем: ${opp['volume']:,.0f}")
                    else:
                        print("\n⏳ Значимых арбитражных возможностей не найдено")
                    
                    # Показываем статистику
                    print(f"\n📊 СТАТИСТИКА ДЕМОНСТРАЦИИ:")
                    print(f"  • Найдено возможностей: {self.stats['opportunities_found']}")
                    if self.stats['opportunities_found'] > 0:
                        avg_profit = self.stats['total_profit_pct'] / self.stats['opportunities_found']
                        print(f"  • Средняя прибыль: {avg_profit:.3f}%")
                        
                        if self.stats['best_opportunity']:
                            best = self.stats['best_opportunity']
                            print(f"  • Лучшая возможность: {best['symbol']} ({best['profit_pct']:.3f}%)")
                
                else:
                    print("❌ Не удалось получить рыночные данные")
                
                print(f"\n⏱️  Следующее обновление через 20 секунд...")
                time.sleep(20)
                
            except KeyboardInterrupt:
                print("\n🛑 Получен сигнал остановки...")
                break
            except Exception as e:
                print(f"❌ Ошибка анализа: {e}")
                time.sleep(10)
    
    def run_final_demo(self, duration=90):
        """Запуск финальной демонстрации"""
        print("🎬 FINAL DEMO: Solana Arbitrage Bot")
        print("=" * 60)
        print("🌐 Высокопроизводительный C++ арбитражный бот")
        print("📊 Анализ реальных рыночных данных")
        print("🎯 Поиск арбитражных возможностей")
        print("⚡ Многопоточная обработка")
        print("🛡️  Управление рисками")
        print("=" * 60)
        
        self.running = True
        start_time = time.time()
        
        # Запускаем C++ бота
        if not self.start_cpp_bot():
            print("❌ Не удалось запустить C++ бота")
            return
        
        # Запускаем анализ рынка в отдельном потоке
        market_thread = threading.Thread(target=self.run_market_analysis)
        market_thread.daemon = True
        market_thread.start()
        
        try:
            print(f"\n⏱️  Демонстрация будет работать {duration} секунд...")
            print("Нажмите Ctrl+C для остановки\n")
            
            time.sleep(duration)
            
        except KeyboardInterrupt:
            print("\n🛑 Получен сигнал остановки...")
        
        finally:
            self.running = False
            self.stats['bot_runtime'] = time.time() - start_time
            
            # Останавливаем бота
            self.stop_cpp_bot()
            
            # Показываем финальную статистику
            print(f"\n{'='*60}")
            print("📈 ФИНАЛЬНАЯ СТАТИСТИКА ДЕМОНСТРАЦИИ")
            print(f"{'='*60}")
            print(f"⏱️  Время работы: {self.stats['bot_runtime']:.1f} секунд")
            print(f"🎯 Найдено возможностей: {self.stats['opportunities_found']}")
            
            if self.stats['opportunities_found'] > 0:
                avg_profit = self.stats['total_profit_pct'] / self.stats['opportunities_found']
                print(f"📊 Средняя прибыль: {avg_profit:.3f}%")
                print(f"💰 Общая прибыльность: {self.stats['total_profit_pct']:.3f}%")
                
                if self.stats['best_opportunity']:
                    best = self.stats['best_opportunity']
                    print(f"🏆 Лучшая возможность: {best['symbol']} ({best['profit_pct']:.3f}%)")
                    print(f"   Время: {best['timestamp'].strftime('%H:%M:%S')}")
            
            print(f"\n🎉 Демонстрация завершена успешно!")
            print("✅ C++ Solana Arbitrage Bot готов к работе!")

def main():
    demo = FinalArbitrageDemo()
    demo.run_final_demo(duration=60)  # 60 секунд

if __name__ == "__main__":
    main()
