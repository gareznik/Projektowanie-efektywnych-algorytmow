import subprocess
import os

EXE_PATH = "./projekt2.exe"
CONFIG_FILE = "config.txt"

# Фаза 1: Файлы (твои и библиотечные)
test_files = [
    "input/matrix_11x11.atsp", # твой кастомный формат
    "input/br17.atsp",         # формат TSPLIB
    "input/att48.tsp"
]

# Фаза 2: Тесты генератора (use_generator, size)
# 1 = ATSP, 2 = STSP
generator_tests = [
    (1, 10), # Сгенерирует ATSP 10x10
    (1, 12), # Сгенерирует ATSP 12x12
    (2, 10), # Сгенерирует STSP 10x10
    (2, 12)  # Сгенерирует STSP 12x12
]

def update_config(use_gen, file_path="", size=0):
    with open(CONFIG_FILE, "r") as f:
        lines = f.readlines()
        
    with open(CONFIG_FILE, "w") as f:
        for line in lines:
            if line.startswith("use_generator="):
                f.write(f"use_generator={use_gen}\n")
            elif line.startswith("input_file=") and use_gen == 0:
                f.write(f"input_file={file_path}\n")
            elif line.startswith("instance_size=") and use_gen != 0:
                f.write(f"instance_size={size}\n")
            else:
                f.write(line)

print("--- Rozpoczecie Auto-Testow ---")

if not os.path.exists("output"):
    os.makedirs("output")

print("\n>>> FAZA 1: Testy z plikow (Wlasne i TSPLib) <<<")
for file in test_files:
    if os.path.exists(file):
        print(f"\n[Testowanie pliku]: {file}")
        update_config(use_gen=0, file_path=file)
        subprocess.run([EXE_PATH])
    else:
        print(f"Brak pliku {file}. Pomin.")

print("\n>>> FAZA 2: Testy losowego generatora <<<")
for gen_type, size in generator_tests:
    typ_str = "ATSP" if gen_type == 1 else "STSP"
    print(f"\n[Testowanie Generatora]: {typ_str} N={size}")
    update_config(use_gen=gen_type, size=size)
    subprocess.run([EXE_PATH])
        
print("\n--- Testy zakonczone. Wyniki w output/results.csv ---")