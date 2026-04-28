import subprocess
import os
import glob
import time

EXE_PATH = "./projekt2.exe"
CONFIG_FILE = "config.txt"
TSPLIB_DIR = "input/tsplib" # Папка, куда ты положил скачанные файлы

# Тестируем все 5 стратегий (или убери 0, если хочешь только быстрые)
strategies_to_test = [4]

def update_config(file_path, ub_strat, out_filename):
    """Обновляет конфиг так, чтобы он читал конкретный файл TSPLIB"""
    with open(CONFIG_FILE, "r", encoding="utf-8") as f:
        lines = f.readlines()
        
    with open(CONFIG_FILE, "w", encoding="utf-8") as f:
        for line in lines:
            if line.startswith("use_generator="):
                f.write("use_generator=0\n") # Отключаем генератор внутри C++!
            elif line.startswith("input_file="):
                f.write(f"input_file={file_path}\n") # Даем путь к файлу TSPLIB
            elif line.startswith("upper_bound_strategy="):
                f.write(f"upper_bound_strategy={ub_strat}\n")
            elif line.startswith("output_file="):
                f.write(f"output_file=output/{out_filename}\n")
            elif line.startswith("repetitions="):
                f.write("repetitions=1\n") # Достаточно 1 раза для готового файла
            else:
                f.write(line)

print("="*65)
print(" ЗАПУСК ТЕСТОВ ДЛЯ ФАЙЛОВ TSPLIB ")
print("="*65)

# Ищем все файлы .tsp и .atsp в папке
tsplib_files = glob.glob(os.path.join(TSPLIB_DIR, "*.tsp")) + glob.glob(os.path.join(TSPLIB_DIR, "*.atsp"))

if not tsplib_files:
    print(f"[ОШИБКА] Не найдено ни одного файла в папке {TSPLIB_DIR}!")
    exit()

for file_path in tsplib_files:
    filename = os.path.basename(file_path)
    print(f"\n[{time.strftime('%H:%M:%S')}] Файл: {filename}")
    
    for strat in strategies_to_test:
        print(f"  -> Стратегия {strat}...", end="", flush=True)
        
        csv_name = f"TSPLIB_{filename}_strat_{strat}.csv"
        update_config(file_path, strat, csv_name)
        
        try:
            # Тайм-аут 15 минут спасает от огромных файлов TSPLIB
            subprocess.run([EXE_PATH], stdout=subprocess.DEVNULL, timeout=900)
            print(" [OK]")
        except subprocess.TimeoutExpired:
            print(" [УБИТ (ПРЕВЫШЕНО 15 МИН)]")
            
            # Записываем в файл метку о таймауте
            with open(f"output/{csv_name}", "a", encoding="utf-8") as f:
                f.write(f"TSPLIB_KILLED, {filename}, 1, TIMEOUT, >900000, 0\n")

print("\n" + "="*65)
print(" ТЕСТЫ TSPLIB ЗАВЕРШЕНЫ!")
print("="*65)