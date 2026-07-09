#!/usr/bin/env python3
"""
NeoZorK3 Tools Runner
Main script to run all tools and demos from the tools directory
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path

class ToolsRunner:
    def __init__(self):
        self.tools_dir = Path(__file__).parent
        self.scripts_dir = self.tools_dir / "scripts"
        self.demos_dir = self.tools_dir / "demos"
        self.configs_dir = self.tools_dir / "configs"
        
    def list_available_tools(self):
        """List all available tools"""
        print("🔧 Available NeoZorK3 Tools:")
        print()
        
        # Scripts
        print("📜 Scripts (tools/scripts/):")
        scripts = list(self.scripts_dir.glob("*.sh"))
        for script in scripts:
            print(f"  • {script.name}")
        print()
        
        # Demos
        print("🎮 Demos (tools/demos/):")
        demos = list(self.demos_dir.glob("*.py"))
        for demo in demos:
            print(f"  • {demo.name}")
        print()
        
        # Configs
        print("⚙️  Configs (tools/configs/):")
        configs = list(self.configs_dir.glob("*"))
        for config in configs:
            print(f"  • {config.name}")
        print()
    
    def run_script(self, script_name):
        """Run a shell script"""
        script_path = self.scripts_dir / script_name
        if not script_path.exists():
            print(f"❌ Script not found: {script_name}")
            return False
        
        print(f"🚀 Running script: {script_name}")
        try:
            result = subprocess.run([str(script_path)], shell=True, check=True)
            return result.returncode == 0
        except subprocess.CalledProcessError as e:
            print(f"❌ Script failed with exit code: {e.returncode}")
            return False
    
    def run_demo(self, demo_name):
        """Run a Python demo"""
        demo_path = self.demos_dir / demo_name
        if not demo_path.exists():
            print(f"❌ Demo not found: {demo_name}")
            return False
        
        print(f"🎮 Running demo: {demo_name}")
        try:
            result = subprocess.run([sys.executable, str(demo_path)], check=True)
            return result.returncode == 0
        except subprocess.CalledProcessError as e:
            print(f"❌ Demo failed with exit code: {e.returncode}")
            return False
    
    def show_config(self, config_name):
        """Show configuration file content"""
        config_path = self.configs_dir / config_name
        if not config_path.exists():
            print(f"❌ Config not found: {config_name}")
            return False
        
        print(f"⚙️  Configuration: {config_name}")
        print("=" * 50)
        try:
            with open(config_path, 'r') as f:
                print(f.read())
            return True
        except Exception as e:
            print(f"❌ Error reading config: {e}")
            return False
    
    def interactive_mode(self):
        """Interactive mode for tool selection"""
        while True:
            print("\n🔧 NeoZorK3 Tools - Interactive Mode")
            print("1. List all tools")
            print("2. Run script")
            print("3. Run demo")
            print("4. Show config")
            print("5. Exit")
            
            choice = input("\nSelect option (1-5): ").strip()
            
            if choice == "1":
                self.list_available_tools()
            elif choice == "2":
                scripts = [s.name for s in self.scripts_dir.glob("*.sh")]
                if scripts:
                    print("Available scripts:")
                    for i, script in enumerate(scripts, 1):
                        print(f"  {i}. {script}")
                    try:
                        idx = int(input("Select script number: ")) - 1
                        if 0 <= idx < len(scripts):
                            self.run_script(scripts[idx])
                    except (ValueError, IndexError):
                        print("❌ Invalid selection")
                else:
                    print("❌ No scripts available")
            elif choice == "3":
                demos = [d.name for d in self.demos_dir.glob("*.py")]
                if demos:
                    print("Available demos:")
                    for i, demo in enumerate(demos, 1):
                        print(f"  {i}. {demo}")
                    try:
                        idx = int(input("Select demo number: ")) - 1
                        if 0 <= idx < len(demos):
                            self.run_demo(demos[idx])
                    except (ValueError, IndexError):
                        print("❌ Invalid selection")
                else:
                    print("❌ No demos available")
            elif choice == "4":
                configs = [c.name for c in self.configs_dir.glob("*")]
                if configs:
                    print("Available configs:")
                    for i, config in enumerate(configs, 1):
                        print(f"  {i}. {config}")
                    try:
                        idx = int(input("Select config number: ")) - 1
                        if 0 <= idx < len(configs):
                            self.show_config(configs[idx])
                    except (ValueError, IndexError):
                        print("❌ Invalid selection")
                else:
                    print("❌ No configs available")
            elif choice == "5":
                print("👋 Goodbye!")
                break
            else:
                print("❌ Invalid option")

def main():
    parser = argparse.ArgumentParser(description="NeoZorK3 Tools Runner")
    parser.add_argument("--list", "-l", action="store_true", help="List all available tools")
    parser.add_argument("--script", "-s", help="Run a specific script")
    parser.add_argument("--demo", "-d", help="Run a specific demo")
    parser.add_argument("--config", "-c", help="Show a specific config")
    parser.add_argument("--interactive", "-i", action="store_true", help="Start interactive mode")
    
    args = parser.parse_args()
    
    runner = ToolsRunner()
    
    if args.list:
        runner.list_available_tools()
    elif args.script:
        runner.run_script(args.script)
    elif args.demo:
        runner.run_demo(args.demo)
    elif args.config:
        runner.show_config(args.config)
    elif args.interactive:
        runner.interactive_mode()
    else:
        # Default: show help and list tools
        parser.print_help()
        print()
        runner.list_available_tools()

if __name__ == "__main__":
    main()
