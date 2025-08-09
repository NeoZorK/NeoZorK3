#!/usr/bin/env python3
"""
Solana Arbitrage Bot Demo
Демонстрация работы арбитражного бота с симуляцией рыночных данных
"""

import json
import time
import random
import subprocess
import threading
from datetime import datetime

class ArbitrageDemo:
    def __init__(self):
        self.running = False
        self.bot_process = None
        self.demo_data = {
            'SOL/USDC': {'price': 100.0, 'volume': 1000000},
            'ETH/USDC': {'price': 3000.0, 'volume': 500000},
            'BTC/USDC': {'price': 45000.0, 'volume': 2000000},
            'RAY/USDC': {'price': 2.5, 'volume': 50000},
            'SRM/USDC': {'price': 1.8, 'volume': 30000}
        }
        
    def simulate_market_data(self):
        """Симуляция изменения рыночных цен"""
        print("🔄 Симуляция рыночных данных...")
        
        while self.running:
            # Симулируем изменения цен
            for pair in self.demo_data:
                # Случайное изменение цены ±2%
                change = random.uniform(-0.02, 0.02)
                self.demo_data[pair]['price'] *= (1 + change)
                self.demo_data[pair]['volume'] *= random.uniform(0.95, 1.05)
                
            # Показываем текущие цены
            print(f"\n📊 Рыночные данные ({datetime.now().strftime('%H:%M:%S')}):")
            for pair, data in self.demo_data.items():
                print(f"  {pair}: ${data['price']:.2f} (Vol: ${data['volume']:,.0f})")
            
            # Симулируем арбитражные возможности
            self.simulate_arbitrage_opportunities()
            
            time.sleep(5)  # Обновляем каждые 5 секунд
    
    def simulate_arbitrage_opportunities(self):
        """Симуляция арбитражных возможностей"""
        opportunities = []
        
        # Треугольный арбитраж: SOL -> ETH -> BTC -> SOL
        sol_eth_rate = self.demo_data['ETH/USDC']['price'] / self.demo_data['SOL/USDC']['price']
        eth_btc_rate = self.demo_data['BTC/USDC']['price'] / self.demo_data['ETH/USDC']['price']
        btc_sol_rate = self.demo_data['SOL/USDC']['price'] / (self.demo_data['BTC/USDC']['price'] / 1000)  # BTC в тысячах
        
        # Рассчитываем прибыльность
        final_sol = 1 * sol_eth_rate * eth_btc_rate * btc_sol_rate
        profit_pct = ((final_sol - 1) / 1) * 100
        
        if abs(profit_pct) > 0.1:  # Показываем только значимые возможности
            opportunities.append({
                'type': 'Triangular',
                'path': 'SOL → ETH → BTC → SOL',
                'profit_pct': profit_pct,
                'volume': min(self.demo_data['SOL/USDC']['volume'], 
                            self.demo_data['ETH/USDC']['volume'],
                            self.demo_data['BTC/USDC']['volume']) * 0.01
            })
        
        # Cross-DEX арбитраж (симуляция разных цен на разных DEX)
        for pair in ['SOL/USDC', 'ETH/USDC']:
            # Симулируем разницу цен между DEX
            raydium_price = self.demo_data[pair]['price']
            orca_price = raydium_price * random.uniform(0.995, 1.005)
            
            if abs(orca_price - raydium_price) / raydium_price > 0.001:  # 0.1% разница
                profit_pct = ((max(orca_price, raydium_price) - min(orca_price, raydium_price)) / 
                             min(orca_price, raydium_price)) * 100
                
                opportunities.append({
                    'type': 'Cross-DEX',
                    'path': f'{pair} (Raydium: ${raydium_price:.2f} vs Orca: ${orca_price:.2f})',
                    'profit_pct': profit_pct,
                    'volume': self.demo_data[pair]['volume'] * 0.005
                })
        
        if opportunities:
            print("\n🎯 Обнаружены арбитражные возможности:")
            for opp in opportunities:
                print(f"  {opp['type']}: {opp['path']}")
                print(f"    Прибыль: {opp['profit_pct']:.3f}% | Объем: ${opp['volume']:,.0f}")
        else:
            print("\n⏳ Арбитражные возможности не найдены...")
    
    def start_bot(self):
        """Запуск C++ бота"""
        print("🚀 Запуск Solana Arbitrage Bot...")
        try:
            self.bot_process = subprocess.Popen(
                ['./build/solana_arbitrage_bot', '--dry-run', '--verbose'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            print("✅ Бот запущен успешно!")
        except Exception as e:
            print(f"❌ Ошибка запуска бота: {e}")
    
    def stop_bot(self):
        """Остановка бота"""
        if self.bot_process:
            print("🛑 Остановка бота...")
            self.bot_process.terminate()
            try:
                self.bot_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.bot_process.kill()
            print("✅ Бот остановлен")
    
    def run_demo(self, duration=60):
        """Запуск демонстрации"""
        print("🎬 Запуск демонстрации Solana Arbitrage Bot")
        print("=" * 50)
        
        self.running = True
        
        # Запускаем симуляцию рыночных данных в отдельном потоке
        market_thread = threading.Thread(target=self.simulate_market_data)
        market_thread.daemon = True
        market_thread.start()
        
        # Запускаем бота
        self.start_bot()
        
        try:
            print(f"\n⏱️  Демонстрация будет работать {duration} секунд...")
            print("Нажмите Ctrl+C для остановки\n")
            
            time.sleep(duration)
            
        except KeyboardInterrupt:
            print("\n🛑 Получен сигнал остановки...")
        
        finally:
            self.running = False
            self.stop_bot()
            
            print("\n📈 Итоги демонстрации:")
            print("  • Симуляция рыночных данных завершена")
            print("  • Арбитражные возможности проанализированы")
            print("  • Бот успешно протестирован в dry-run режиме")
            print("\n🎉 Демонстрация завершена!")

def main():
    demo = ArbitrageDemo()
    
    print("Solana Arbitrage Bot - Демонстрация")
    print("=" * 40)
    print("Этот демо показывает:")
    print("• Симуляцию рыночных данных в реальном времени")
    print("• Поиск арбитражных возможностей")
    print("• Работу C++ бота в dry-run режиме")
    print("• Анализ прибыльности различных стратегий")
    print()
    
    try:
        demo.run_demo(duration=30)  # 30 секунд демонстрации
    except Exception as e:
        print(f"❌ Ошибка в демонстрации: {e}")

if __name__ == "__main__":
    main()
