#!/usr/bin/env python3
"""
Simple Solana Arbitrage Demo
Упрощенная демонстрация с публичными API
"""

import requests
import json
import time
import random
from datetime import datetime

class SimpleSolanaDemo:
    def __init__(self):
        # Используем публичные API без SSL проблем
        self.apis = {
            'coingecko': 'https://api.coingecko.com/api/v3',
            'binance': 'https://api.binance.com/api/v3',
            'coinbase': 'https://api.coinbase.com/v2'
        }
        
        # Популярные токены для анализа
        self.tokens = ['solana', 'ethereum', 'bitcoin', 'usd-coin', 'tether']
        self.symbols = ['SOL', 'ETH', 'BTC', 'USDC', 'USDT']
    
    def get_coingecko_prices(self):
        """Получение цен с CoinGecko"""
        try:
            url = f"{self.apis['coingecko']}/simple/price"
            params = {
                'ids': ','.join(self.tokens),
                'vs_currencies': 'usd',
                'include_24hr_change': 'true'
            }
            
            response = requests.get(url, params=params, timeout=10)
            if response.status_code == 200:
                return response.json()
        except Exception as e:
            print(f"Ошибка CoinGecko: {e}")
        return {}
    
    def get_binance_prices(self):
        """Получение цен с Binance"""
        try:
            prices = {}
            for symbol in self.symbols:
                if symbol in ['USDC', 'USDT']:
                    prices[symbol] = 1.0  # Стейблкоины
                    continue
                    
                url = f"{self.apis['binance']}/ticker/price"
                params = {'symbol': f'{symbol}USDT'}
                
                response = requests.get(url, params=params, timeout=10)
                if response.status_code == 200:
                    data = response.json()
                    prices[symbol] = float(data['price'])
        except Exception as e:
            print(f"Ошибка Binance: {e}")
        return prices
    
    def simulate_dex_prices(self, base_prices):
        """Симуляция цен на разных DEX"""
        dex_prices = {}
        
        for symbol, base_price in base_prices.items():
            if base_price > 0:
                # Симулируем небольшие различия между DEX
                raydium_price = base_price * random.uniform(0.998, 1.002)
                orca_price = base_price * random.uniform(0.997, 1.003)
                jupiter_price = base_price * random.uniform(0.999, 1.001)
                
                dex_prices[symbol] = {
                    'base': base_price,
                    'raydium': raydium_price,
                    'orca': orca_price,
                    'jupiter': jupiter_price
                }
        
        return dex_prices
    
    def analyze_arbitrage(self, dex_prices):
        """Анализ арбитражных возможностей"""
        opportunities = []
        
        for symbol, prices in dex_prices.items():
            if symbol in ['USDC', 'USDT']:
                continue  # Пропускаем стейблкоины
                
            # Находим минимальную и максимальную цену
            min_price = min(prices.values())
            max_price = max(prices.values())
            
            if min_price > 0:
                profit_pct = ((max_price - min_price) / min_price) * 100
                
                if profit_pct > 0.05:  # Показываем только значимые возможности
                    buy_dex = min(prices, key=prices.get)
                    sell_dex = max(prices, key=prices.get)
                    
                    opportunities.append({
                        'symbol': symbol,
                        'buy_dex': buy_dex,
                        'sell_dex': sell_dex,
                        'buy_price': min_price,
                        'sell_price': max_price,
                        'profit_pct': profit_pct,
                        'volume': random.uniform(1000, 10000)  # Симулируем объем
                    })
        
        return opportunities
    
    def run_simple_demo(self, duration=60):
        """Запуск упрощенной демонстрации"""
        print("🌐 Solana Arbitrage Bot - Упрощенная демонстрация")
        print("=" * 55)
        print("Подключение к публичным API...")
        print("• CoinGecko API")
        print("• Binance API")
        print("• Симуляция DEX цен")
        print()
        
        start_time = time.time()
        
        while time.time() - start_time < duration:
            try:
                print(f"\n📊 Рыночные данные ({datetime.now().strftime('%H:%M:%S')}):")
                print("-" * 40)
                
                # Получаем базовые цены
                coingecko_prices = self.get_coingecko_prices()
                binance_prices = self.get_binance_prices()
                
                # Объединяем данные
                base_prices = {}
                for i, token in enumerate(self.tokens):
                    symbol = self.symbols[i]
                    
                    # Приоритет Binance, затем CoinGecko
                    if symbol in binance_prices:
                        base_prices[symbol] = binance_prices[symbol]
                    elif token in coingecko_prices and 'usd' in coingecko_prices[token]:
                        base_prices[symbol] = coingecko_prices[token]['usd']
                
                # Показываем базовые цены
                print("💰 Базовые цены:")
                for symbol, price in base_prices.items():
                    print(f"  {symbol}: ${price:.4f}")
                
                # Симулируем DEX цены
                dex_prices = self.simulate_dex_prices(base_prices)
                
                # Показываем DEX цены
                print("\n🏪 DEX цены:")
                for symbol, prices in dex_prices.items():
                    if symbol not in ['USDC', 'USDT']:
                        print(f"  {symbol}:")
                        for dex, price in prices.items():
                            print(f"    {dex.capitalize()}: ${price:.4f}")
                
                # Анализируем арбитражные возможности
                opportunities = self.analyze_arbitrage(dex_prices)
                
                if opportunities:
                    print("\n🎯 Обнаружены арбитражные возможности:")
                    for opp in opportunities:
                        print(f"  {opp['symbol']}: {opp['buy_dex'].capitalize()} → {opp['sell_dex'].capitalize()}")
                        print(f"    Покупка: ${opp['buy_price']:.4f} | Продажа: ${opp['sell_price']:.4f}")
                        print(f"    Прибыль: {opp['profit_pct']:.3f}% | Объем: ${opp['volume']:,.0f}")
                else:
                    print("\n⏳ Значимых арбитражных возможностей не найдено")
                
                print(f"\n⏱️  Следующее обновление через 15 секунд...")
                time.sleep(15)
                
            except KeyboardInterrupt:
                print("\n🛑 Демонстрация остановлена пользователем")
                break
            except Exception as e:
                print(f"❌ Ошибка: {e}")
                time.sleep(5)
        
        print("\n📈 Итоги упрощенной демонстрации:")
        print("  • Подключение к публичным API")
        print("  • Симуляция DEX цен")
        print("  • Анализ арбитражных возможностей")
        print("  • Мониторинг в реальном времени")
        print("\n🎉 Упрощенная демонстрация завершена!")

def main():
    demo = SimpleSolanaDemo()
    demo.run_simple_demo(duration=45)  # 45 секунд

if __name__ == "__main__":
    main()
