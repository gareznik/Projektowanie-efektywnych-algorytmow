import subprocess
import os
import time
import random
import sys
sys.stdout.reconfigure(encoding='utf-8')

EXE_PATH = "./projekt2.exe"
CONFIG_FILE = "config.txt"
TEMP_FILE = "input/temp_matrix.atsp"

# 1. Размеры для генератора
generator_sizes = [9, 10, 11, 12, 13, 14, 15] 

# 2. Все 5 стратегий Upper Bound
strategies_to_test = [0, 1, 2, 3, 4]

def generate_matrix_file(filename, size, is_symmetric=False):
    """Создает случайную матрицу и сохраняет в формате TSPLIB"""
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    
    with open(filename, 'w') as f:
        f.write(f"NAME: temp_matrix\n")
        f.write(f"TYPE: {'TSP' if is_symmetric else 'ATSP'}\n")
        f.write(f"DIMENSION: {size}\n")
        f.write(f"EDGE_WEIGHT_TYPE: EXPLICIT\n")
        f.write(f"EDGE_WEIGHT_FORMAT: FULL_MATRIX\n")
        f.write(f"EDGE_WEIGHT_SECTION\n")
        
        matrix = [[0]*size for _ in range(size)]
        for i in range(size):
            for j in range(size):
                if i != j:
                    if is_symmetric and i > j:
                        matrix[i][j] = matrix[j][i]
                    elif not is_symmetric or i < j:
                        matrix[i][j] = random.randint(1, 200)
                        
        for row in matrix:
            f.write(" ".join(map(str, row)) + "\n")
        f.write("EOF\n")

def update_config(use_gen=0, file_path=TEMP_FILE, size=0, repetitions=1, ub_strat=0, out_filename="results.csv"):
    """Обновляет config.txt перед запуском .exe"""
    with open(CONFIG_FILE, "r", encoding="utf-8") as f:
        lines = f.readlines()
        
    with open(CONFIG_FILE, "w", encoding="utf-8") as f:
        for line in lines:
            if line.startswith("use_generator="):
                f.write(f"use_generator={use_gen}\n")
            elif line.startswith("input_file="):
                f.write(f"input_file={file_path}\n")
            elif line.startswith("instance_size="):
                f.write(f"instance_size={size}\n")
            elif line.startswith("repetitions="):
                f.write(f"repetitions={repetitions}\n")
            elif line.startswith("upper_bound_strategy="):
                f.write(f"upper_bound_strategy={ub_strat}\n")
            elif line.startswith("output_file="):
                f.write(f"output_file=output/{out_filename}\n")
            else:
                f.write(line)

print("="*65)
print(" РОЗПОЧАТИЕ ТЕСТОВ: ATSP -> STSP | Умные лимиты ")
print("="*65)

if not os.path.exists("output"):
    os.makedirs("output")

# СНАЧАЛА полностью ATSP (1), ПОТОМ полностью STSP (2)
for gen_type in [1, 2]: 
    is_sym = (gen_type == 2)
    typ_str = "STSP" if is_sym else "ATSP"
    
    print(f"\n{'='*40}")
    print(f" ПЕРЕХОД К ТИПУ МАТРИЦ: {typ_str}")
    print(f"{'='*40}")
    
    for size in generator_sizes:
        print(f"\n[{time.strftime('%H:%M:%S')}] Старт {typ_str} N={size}")
        
        # МАГИЯ ЗДЕСЬ: Если размер 15, делаем только 5 инстанций, иначе 20
        target_instances = 5 if size >= 14 else 20
        
        for instance_num in range(1, target_instances + 1):
            # flush=True спасает от зависания текста в консоли
            print(f"  -> Инстанция {instance_num}/{target_instances}...", end="\r", flush=True)
            
            generate_matrix_file(TEMP_FILE, size, is_symmetric=is_sym)
            
            for strat in strategies_to_test:
                # Спасаем комп: если Стратегия 0 и размер >= 13, скипаем!
                if strat == 0 and size >= 13:
                    continue

                # Стратегию 0 гоняем только 1 раз на размер
                if strat == 0 and instance_num > 1:
                    continue
                
                # Создаем РАЗНЫЕ файлы для ATSP и STSP
                csv_name = f"results_{typ_str}_strat_{strat}.csv"
                update_config(use_gen=0, file_path=TEMP_FILE, repetitions=1, ub_strat=strat, out_filename=csv_name)
                
                try:
                    # Запускаем экзешник (без дубликатов!) с лимитом 15 минут
                    subprocess.run([EXE_PATH], stdout=subprocess.DEVNULL, timeout=900)
                except subprocess.TimeoutExpired:
                    # Предупреждение в консоль
                    print(f"\n     [ВНИМАНИЕ] {typ_str} Strat {strat} (N={size}, Инст {instance_num}) убита: превышен лимит 15 минут!")
                    
                    # Пишем метку в CSV файл
                    with open(f"output/{csv_name}", "a", encoding="utf-8") as f:
                        f.write(f"KILLED_>15MIN, {size}, {instance_num}, TIMEOUT, >900000, 0\n")
            
        print(f"  -> Инстанции {target_instances}/{target_instances} завершены!                                   ")

print("\n\n" + "="*65)
print(" ТЕСТЫ ПОЛНОСТЬЮ ЗАВЕРШЕНЫ. Результаты лежат в папке output/")
print("="*65)