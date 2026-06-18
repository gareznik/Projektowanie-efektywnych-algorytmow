import os
import subprocess
import time

# --- USTAWIENIA ---
EXE_PATH = "projekt4.exe"      
CONFIG_PATH = "config.txt"     

# Lista instancji pokrywajaca wymagania z zadania:
# 14, 20, 25, 50, 75, 120, 170, 250, 450, 600, 900, 1300, 1800, 2500
instances = [
    # {"file": "input/burma14.tsp", "size": 14},
    # {"file": "input/gr21.tsp", "size": 21},
    # {"file": "input/fri26.tsp", "size": 26},
    # {"file": "input/berlin52.tsp", "size": 52},
    # {"file": "input/pr76.tsp", "size": 76},
    # {"file": "input/gr120.tsp", "size": 120},
    # {"file": "input/si175.tsp", "size": 175},
    # {"file": "input/gil262.tsp", "size": 262},
    # {"file": "input/pcb442.tsp", "size": 442},
    # {"file": "input/u574.tsp", "size": 574},
    # {"file": "input/rat783.tsp", "size": 783},    # Zamiast 900
    {"file": "input/pr1002.tsp", "size": 1002},   # Zamiast 1300
    {"file": "input/rl1304.tsp", "size": 1304},
    {"file": "input/u1817.tsp", "size": 1817},
    {"file": "input/pr2392.tsp", "size": 2392}   # Zamiast 2500
]

# # Zestawy parametrow (Grid Search)
# parameter_sets = [
#     # 1. Zbalansowany
#     {"ant_mult": 1.0, "alpha": 1.0, "beta": 3.0, "rho": 0.5},
#     # 2. Silna heurystyka (zachlanny, dobry dla malych grafow n < 24)
#     {"ant_mult": 1.0, "alpha": 1.0, "beta": 5.0, "rho": 0.5},
#     # 3. Szybki (mniej mrowek, slaba heurystyka, dla duzych grafow)
#     {"ant_mult": 0.5, "alpha": 1.0, "beta": 2.0, "rho": 0.1}
# ]

# Zestawy parametrow (Grid Search)
parameter_sets = [
    # --- BAZOWE (Pokazuja idealne trafienia) ---
    # 1. Zbalansowany
    {"ant_mult": 1.0, "alpha": 1.0, "beta": 3.0, "rho": 0.5},
    # 2. Silna heurystyka (zachlanny, dla n < 24)
    {"ant_mult": 1.0, "alpha": 1.0, "beta": 5.0, "rho": 0.5},
    # 3. Szybki / Badawczy (dla duzych grafow)
    {"ant_mult": 0.5, "alpha": 1.0, "beta": 2.0, "rho": 0.1},

    # --- NOWE DO GLEBOKIEJ ANALIZY ---
    # 4. Dominacja feromonu (mrówki ufają śladom, a nie oczom)
    {"ant_mult": 1.0, "alpha": 2.0, "beta": 1.0, "rho": 0.5},
    # 5. Szybkie parowanie (ekstremalne zapominanie)
    {"ant_mult": 1.0, "alpha": 1.0, "beta": 3.0, "rho": 0.9},
    # 6. Bardzo mało mrówek, ale silna heurystyka (do oszczędzania czasu)
    {"ant_mult": 0.1, "alpha": 1.0, "beta": 4.0, "rho": 0.5}
]

def update_config(input_file, num_ants, alpha, beta, rho):
    config_content = f"""# Wygenerowane automatycznie przez Python
use_generator=0
instance_size=20
input_file={input_file}
output_file=output/test2.csv
repetitions=1
time_limit_s=60
show_progress=0
num_ants={int(num_ants)}
alpha={alpha}
beta={beta}
rho={rho}
q_val=100.0
save_trace=0
"""
    with open(CONFIG_PATH, "w", encoding="utf-8") as f:
        f.write(config_content)

def run_tests():
    print("[START] Rozpoczecie automatycznego testowania ACO...")
    
    os.makedirs("output", exist_ok=True)
    
    total_runs = len(instances) * len(parameter_sets)
    current_run = 0

    for inst in instances:
        file_path = inst["file"]
        size = inst["size"]
        
        if not os.path.exists(file_path):
            print(f"[UWAGA] Brak pliku: {file_path}. Pomijam...")
            continue

        print(f"\n--- Testowanie grafu: {file_path} (n={size}) ---")
        
        for params in parameter_sets:
            current_run += 1
            
            num_ants = max(5, int(size * params["ant_mult"]))
            alpha = params["alpha"]
            beta = params["beta"]
            rho = params["rho"]
            
            print(f"[{current_run}/{total_runs}] Parametry: Ants={num_ants}, a={alpha}, b={beta}, rho={rho} ...", end="", flush=True)
            
            update_config(file_path, num_ants, alpha, beta, rho)
            
            start_time = time.time()
            try:
                subprocess.run(f".\\{EXE_PATH}", shell=True, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                elapsed = time.time() - start_time
                print(f" Gotowe! ({elapsed:.2f} s)")
            except subprocess.CalledProcessError:
                print(" [BLAD] Wykonanie pliku C++ nie powiodlo sie!")
                
    print("\n[KONIEC] Wszystkie testy zakonczone pomyslnie!")
    print("Wyniki zostaly zapisane w: output/results_aco.csv")

if __name__ == "__main__":
    run_tests()