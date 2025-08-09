#!/usr/bin/env python3
"""
Smart Airdrop - Advanced Solana Testnet Faucet
Умный airdrop с защитой от 429 ошибок и множественными стратегиями
"""

import requests
import json
import time
import subprocess
import sys
import random
from datetime import datetime, timedelta

class SmartAirdrop:
    def __init__(self):
        # Множественные RPC endpoints
        self.rpc_endpoints = [
            "https://api.testnet.solana.com",
            "https://testnet.solana.rpcpool.com", 
            "https://solana-testnet.rpc.extrnode.com",
            "https://rpc.testnet.solana.com",
            "https://testnet.genesysgo.net"
        ]
        
        # Альтернативные faucet endpoints
        self.faucet_endpoints = [
            "https://faucet.solana.com",
            "https://solfaucet.com",
            "https://faucet.solana.com/request"
        ]
        
        # Стратегии обхода 429
        self.strategies = {
            "exponential_backoff": True,
            "random_delays": True,
            "multiple_endpoints": True,
            "user_agent_rotation": True,
            "request_throttling": True
        }
        
        # User agents для ротации
        self.user_agents = [
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36",
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
            "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36",
            "curl/7.68.0",
            "python-requests/2.25.1"
        ]
        
        # Статистика запросов
        self.request_history = []
        self.success_count = 0
        self.failure_count = 0
        
    def get_random_delay(self, base_delay=5, max_jitter=10):
        """Генерация случайной задержки"""
        jitter = random.uniform(0, max_jitter)
        return base_delay + jitter
    
    def exponential_backoff(self, attempt, base_delay=5):
        """Экспоненциальная задержка"""
        return base_delay * (2 ** attempt)
    
    def get_wallet_address(self):
        """Получение адреса кошелька"""
        try:
            result = subprocess.run(['solana', 'address'], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip()
        except Exception as e:
            print(f"❌ Ошибка получения адреса: {e}")
        return None
    
    def test_endpoint_health(self, endpoint):
        """Проверка здоровья endpoint"""
        try:
            payload = {
                "jsonrpc": "2.0",
                "id": 1,
                "method": "getHealth"
            }
            headers = {"User-Agent": random.choice(self.user_agents)}
            response = requests.post(endpoint, json=payload, headers=headers, timeout=10)
            return response.status_code == 200
        except:
            return False
    
    def smart_request_airdrop(self, amount, max_retries=5):
        """Умный запрос airdrop с множественными стратегиями"""
        address = self.get_wallet_address()
        if not address:
            return False
        
        lamports = int(amount * 1_000_000_000)
        
        print(f"🧠 Умный airdrop {amount} SOL на {address}")
        print(f"📊 Стратегии: {', '.join([k for k, v in self.strategies.items() if v])}")
        
        for attempt in range(max_retries):
            try:
                # 1. Экспоненциальная задержка
                if attempt > 0 and self.strategies["exponential_backoff"]:
                    delay = self.exponential_backoff(attempt)
                    print(f"⏳ Экспоненциальная задержка: {delay:.1f} сек")
                    time.sleep(delay)
                
                # 2. Случайная задержка
                if self.strategies["random_delays"]:
                    random_delay = self.get_random_delay()
                    print(f"🎲 Случайная задержка: {random_delay:.1f} сек")
                    time.sleep(random_delay)
                
                # 3. Ротация RPC endpoints
                if self.strategies["multiple_endpoints"] and attempt > 0:
                    endpoint = random.choice(self.rpc_endpoints)
                    print(f"🔄 Переключение на RPC: {endpoint}")
                else:
                    endpoint = self.rpc_endpoints[0]
                
                # 4. Ротация User-Agent
                user_agent = random.choice(self.user_agents) if self.strategies["user_agent_rotation"] else "python-requests"
                
                # 5. Throttling запросов
                if self.strategies["request_throttling"]:
                    self.throttle_requests()
                
                # Выполнение запроса
                payload = {
                    "jsonrpc": "2.0",
                    "id": 1,
                    "method": "requestAirdrop",
                    "params": [address, lamports]
                }
                
                headers = {"User-Agent": user_agent}
                
                print(f"📤 Попытка {attempt + 1}/{max_retries} через {endpoint}")
                
                response = requests.post(endpoint, json=payload, headers=headers, timeout=30)
                
                # Обработка ответа
                if response.status_code == 200:
                    data = response.json()
                    if 'result' in data:
                        signature = data['result']
                        print(f"✅ Успешный airdrop! Подпись: {signature}")
                        self.success_count += 1
                        self.record_request(True, endpoint, amount)
                        return True
                    elif 'error' in data:
                        error_msg = data['error'].get('message', 'Unknown error')
                        error_code = data['error'].get('code', 0)
                        
                        if error_code == -32005:  # Rate limit
                            print(f"⚠️  Rate limit обнаружен. Увеличиваем задержку...")
                            time.sleep(60)
                            continue
                        else:
                            print(f"❌ RPC ошибка: {error_msg}")
                            self.failure_count += 1
                            self.record_request(False, endpoint, amount, error_msg)
                
                elif response.status_code == 429:
                    print(f"⚠️  HTTP 429 - Too Many Requests")
                    print(f"🔄 Переключение стратегии...")
                    
                    # Пробуем другой endpoint
                    if self.strategies["multiple_endpoints"]:
                        endpoint = random.choice([e for e in self.rpc_endpoints if e != endpoint])
                        print(f"🔄 Новый endpoint: {endpoint}")
                    
                    # Увеличиваем задержку
                    delay = 120 + (attempt * 60)
                    print(f"⏳ Длительная задержка: {delay} сек")
                    time.sleep(delay)
                    
                    self.failure_count += 1
                    self.record_request(False, endpoint, amount, "HTTP 429")
                    continue
                
                else:
                    print(f"❌ HTTP {response.status_code}: {response.text[:100]}")
                    self.failure_count += 1
                    self.record_request(False, endpoint, amount, f"HTTP {response.status_code}")
                
            except requests.exceptions.Timeout:
                print(f"⚠️  Таймаут запроса")
                self.failure_count += 1
                self.record_request(False, endpoint, amount, "Timeout")
                
            except requests.exceptions.ConnectionError:
                print(f"⚠️  Ошибка подключения")
                self.failure_count += 1
                self.record_request(False, endpoint, amount, "Connection Error")
                
            except Exception as e:
                print(f"❌ Неожиданная ошибка: {e}")
                self.failure_count += 1
                self.record_request(False, endpoint, amount, str(e))
        
        print(f"❌ Не удалось выполнить airdrop после {max_retries} попыток")
        return False
    
    def throttle_requests(self):
        """Ограничение частоты запросов"""
        now = datetime.now()
        recent_requests = [req for req in self.request_history 
                          if now - req['timestamp'] < timedelta(minutes=5)]
        
        if len(recent_requests) >= 3:
            delay = 30
            print(f"🚦 Throttling: {delay} сек (слишком много запросов)")
            time.sleep(delay)
    
    def record_request(self, success, endpoint, amount, error=None):
        """Запись статистики запроса"""
        self.request_history.append({
            'timestamp': datetime.now(),
            'success': success,
            'endpoint': endpoint,
            'amount': amount,
            'error': error
        })
        
        # Ограничиваем историю последними 100 запросами
        if len(self.request_history) > 100:
            self.request_history = self.request_history[-100:]
    
    def show_statistics(self):
        """Показ статистики"""
        print(f"\n📊 СТАТИСТИКА AIRDROP:")
        print(f"   Успешных: {self.success_count}")
        print(f"   Неудачных: {self.failure_count}")
        
        if self.request_history:
            success_rate = (self.success_count / (self.success_count + self.failure_count)) * 100
            print(f"   Успешность: {success_rate:.1f}%")
            
            # Анализ ошибок
            errors = {}
            for req in self.request_history:
                if not req['success'] and req['error']:
                    errors[req['error']] = errors.get(req['error'], 0) + 1
            
            if errors:
                print(f"   Частые ошибки:")
                for error, count in sorted(errors.items(), key=lambda x: x[1], reverse=True)[:3]:
                    print(f"     {error}: {count} раз")
    
    def alternative_faucet_request(self, amount):
        """Запрос через альтернативные faucet"""
        print(f"🌐 Попытка через альтернативные faucet...")
        
        for faucet_url in self.faucet_endpoints:
            try:
                print(f"🔄 Пробуем: {faucet_url}")
                
                # Имитация веб-запроса
                headers = {
                    "User-Agent": random.choice(self.user_agents),
                    "Accept": "application/json",
                    "Content-Type": "application/json"
                }
                
                # Простой GET запрос для проверки доступности
                response = requests.get(faucet_url, headers=headers, timeout=10)
                
                if response.status_code == 200:
                    print(f"✅ Faucet доступен: {faucet_url}")
                    print(f"💡 Рекомендация: Используйте веб-интерфейс {faucet_url}")
                    return True
                    
            except Exception as e:
                print(f"❌ Faucet недоступен: {faucet_url} - {e}")
                continue
        
        return False
    
    def run_smart_airdrop(self, amount):
        """Запуск умного airdrop"""
        print("🧠 Smart Airdrop - Умное пополнение Solana Testnet")
        print("=" * 60)
        print("🛡️  Защита от 429 ошибок")
        print("🔄 Множественные RPC endpoints")
        print("⏱️  Умные задержки")
        print("=" * 60)
        
        # Проверка Solana CLI
        if not self.get_wallet_address():
            print("❌ Solana CLI не настроен")
            return False
        
        # Выполнение умного airdrop
        success = self.smart_request_airdrop(amount)
        
        if not success:
            print(f"\n🔄 Попытка альтернативных методов...")
            self.alternative_faucet_request(amount)
        
        # Показ статистики
        self.show_statistics()
        
        return success

def main():
    smart_airdrop = SmartAirdrop()
    
    if len(sys.argv) > 1:
        try:
            amount = float(sys.argv[1])
            smart_airdrop.run_smart_airdrop(amount)
        except ValueError:
            print("❌ Неверный формат суммы")
            print("Использование: python3 smart_airdrop.py [amount]")
    else:
        # Интерактивный режим
        print("🧠 Smart Airdrop - Интерактивный режим")
        print("=" * 40)
        
        try:
            amount = float(input("Введите сумму SOL для пополнения: "))
            smart_airdrop.run_smart_airdrop(amount)
        except ValueError:
            print("❌ Неверный формат суммы")
        except KeyboardInterrupt:
            print("\n\n🛑 Операция отменена")

if __name__ == "__main__":
    main()
