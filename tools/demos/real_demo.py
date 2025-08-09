#!/usr/bin/env python3
"""
Real Solana Arbitrage Demo
Демонстрация с реальными данными Solana DEX
"""

import requests
import json
import time
import asyncio
import aiohttp
from datetime import datetime

class RealSolanaDemo:
    def __init__(self):
        self.rpc_url = "https://api.mainnet-beta.solana.com"
        self.dex_apis = {
            'raydium': 'https://api.raydium.io/v2/main/price',
            'jupiter': 'https://price.jup.ag/v4/price',
            'birdeye': 'https://public-api.birdeye.so/public/price'
        }
        
        # Популярные токены Solana
        self.tokens = {
            'SOL': 'So11111111111111111111111111111111111111112',
            'USDC': 'EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v',
            'USDT': 'Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB',
            'RAY': '4k3Dyjzvzp8eMZWUXbBCjEvwSkkk59S5iCNLY3QrkX6R',
            'SRM': 'SRMuApVNdxXokk5GT7XD5cUUgXMBCoAz2LHeuAoKWRt',
            'ORCA': 'orcaEKTdK7LKz57vaAYr9QeNsVEPfiu6QeMU1kektZE',
            'BONK': 'DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263'
        }
    
    async def get_raydium_prices(self):
        """Получение цен с Raydium"""
        try:
            async with aiohttp.ClientSession() as session:
                async with session.get(self.dex_apis['raydium']) as response:
                    if response.status == 200:
                        data = await response.json()
                        return data
        except Exception as e:
            print(f"Ошибка получения данных Raydium: {e}")
        return {}
    
    async def get_jupiter_prices(self):
        """Получение цен с Jupiter"""
        try:
            tokens = ','.join(self.tokens.values())
            url = f"{self.dex_apis['jupiter']}?ids={tokens}"
            
            async with aiohttp.ClientSession() as session:
                async with session.get(url) as response:
                    if response.status == 200:
                        data = await response.json()
                        return data.get('data', {})
        except Exception as e:
            print(f"Ошибка получения данных Jupiter: {e}")
        return {}
    
    async def get_birdeye_prices(self):
        """Получение цен с Birdeye"""
        try:
            tokens = ','.join(self.tokens.keys())
            url = f"{self.dex_apis['birdeye']}?address={tokens}"
            
            async with aiohttp.ClientSession() as session:
                async with session.get(url) as response:
                    if response.status == 200:
                        data = await response.json()
                        return data.get('data', {})
        except Exception as e:
            print(f"Ошибка получения данных Birdeye: {e}")
        return {}
    
    def analyze_arbitrage(self, prices_data):
        """Анализ арбитражных возможностей"""
        opportunities = []
        
        # Анализируем цены между разными источниками
        if 'raydium' in prices_data and 'jupiter' in prices_data:
            raydium_prices = prices_data['raydium']
            jupiter_prices = prices_data['jupiter']
            
            for token in ['SOL', 'USDC', 'RAY']:
                if token in raydium_prices and token in jupiter_prices:
                    raydium_price = raydium_prices[token]
                    jupiter_price = jupiter_prices[token]
                    
                    if raydium_price and jupiter_price:
                        price_diff = abs(raydium_price - jupiter_price)
                        profit_pct = (price_diff / min(raydium_price, jupiter_price)) * 100
                        
                        if profit_pct > 0.1:  # Показываем только значимые различия
                            opportunities.append({
                                'type': 'Cross-DEX',
                                'token': token,
                                'raydium_price': raydium_price,
                                'jupiter_price': jupiter_price,
                                'profit_pct': profit_pct,
                                'direction': 'Raydium → Jupiter' if raydium_price < jupiter_price else 'Jupiter → Raydium'
                            })
        
        return opportunities
    
    async def run_real_demo(self, duration=60):
        """Запуск демонстрации с реальными данными"""
        print("🌐 Solana Arbitrage Bot - Реальная демонстрация")
        print("=" * 55)
        print("Подключение к реальным DEX Solana...")
        print("• Raydium API")
        print("• Jupiter API") 
        print("• Birdeye API")
        print()
        
        start_time = time.time()
        
        while time.time() - start_time < duration:
            try:
                print(f"\n📊 Реальные данные Solana ({datetime.now().strftime('%H:%M:%S')}):")
                print("-" * 40)
                
                # Получаем данные параллельно
                raydium_data, jupiter_data, birdeye_data = await asyncio.gather(
                    self.get_raydium_prices(),
                    self.get_jupiter_prices(),
                    self.get_birdeye_prices(),
                    return_exceptions=True
                )
                
                prices_data = {
                    'raydium': raydium_data,
                    'jupiter': jupiter_data,
                    'birdeye': birdeye_data
                }
                
                # Показываем цены
                print("💰 Текущие цены:")
                for source, data in prices_data.items():
                    if isinstance(data, dict) and data:
                        print(f"  {source.upper()}:")
                        for token, price in list(data.items())[:3]:  # Показываем первые 3 токена
                            if isinstance(price, (int, float)) and price > 0:
                                print(f"    {token}: ${price:.4f}")
                
                # Анализируем арбитражные возможности
                opportunities = self.analyze_arbitrage(prices_data)
                
                if opportunities:
                    print("\n🎯 Обнаружены арбитражные возможности:")
                    for opp in opportunities:
                        print(f"  {opp['type']}: {opp['token']}")
                        print(f"    Raydium: ${opp['raydium_price']:.4f} | Jupiter: ${opp['jupiter_price']:.4f}")
                        print(f"    Прибыль: {opp['profit_pct']:.3f}% | Направление: {opp['direction']}")
                else:
                    print("\n⏳ Значимых арбитражных возможностей не найдено")
                
                print(f"\n⏱️  Следующее обновление через 10 секунд...")
                await asyncio.sleep(10)
                
            except KeyboardInterrupt:
                print("\n🛑 Демонстрация остановлена пользователем")
                break
            except Exception as e:
                print(f"❌ Ошибка: {e}")
                await asyncio.sleep(5)
        
        print("\n📈 Итоги реальной демонстрации:")
        print("  • Подключение к реальным DEX API")
        print("  • Анализ реальных рыночных данных")
        print("  • Поиск арбитражных возможностей")
        print("  • Мониторинг цен в реальном времени")
        print("\n🎉 Реальная демонстрация завершена!")

async def main():
    demo = RealSolanaDemo()
    await demo.run_real_demo(duration=30)  # 30 секунд

if __name__ == "__main__":
    asyncio.run(main())
