import os
import subprocess
import time

# =====================================================================
# KONFIGURACJA ŚCIEŻEK
# =====================================================================
EXE_PATH = r".\src\main.exe"  # Ścieżka do Twojego skompilowanego programu
CONFIG_PATH = "config.txt"    # Plik, z którego czyta C++
OUTPUT_DIR = "output_experiments/" # Folder na dedykowane wyniki

# Tworzymy folder na wyniki, jeśli nie istnieje
if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# =====================================================================
# PRZEŁĄCZNIKI EKSPERYMENTÓW (Zmień na True, aby uruchomić dany test)
# =====================================================================
RUN_SIZE_TEST     = True   # [3.0] Zależność czasu i błędu w funkcji rozmiaru
RUN_TSPLIB_TEST   = False  # TSPLIB (5 TSP, 5 ATSP) + test 15 minut
RUN_UB_LB_TEST    = False  # [3.5] Wpływ UB vs Losowy start
RUN_COOLING_TEST  = False  # [4.0] Schematy chłodzenia (Geometric vs Linear)
RUN_EPOCH_TEST    = False  # [4.5] Wpływ długości epoki
RUN_TEMP_TEST     = False  # [5.0] Wpływ temperatury początkowej

# =====================================================================
# FUNKCJE POMOCNICZE
# =====================================================================
def create_config(use_gen, size, infile, outfile, reps, t_limit, epoch, alpha, cool, ub, init_t):
    """Generuje plik config.txt przed każdym uruchomieniem .exe"""
    config_content = f"""use_generator={use_gen}
instance_size={size}
input_file={infile}
output_file={outfile}
repetitions={reps}
time_limit_s={t_limit}
show_progress=1
epoch_length={epoch}
alpha={alpha}
cooling_scheme={cool}
use_ub={ub}
initial_temp={init_t}
"""
    with open(CONFIG_PATH, "w") as f:
        f.write(config_content)

def run_experiment(exp_name, use_gen, size=10, infile="", reps=5, t_limit=300,
                   epoch=1000, alpha=0.99, cool=0, ub=0, init_t=0.0):
    """Uruchamia program i zapisuje wynik do unikalnego pliku CSV"""
    
    # Tworzenie mądrej nazwy pliku na podstawie parametrów
    if use_gen == 1:
        out_name = f"{OUTPUT_DIR}{exp_name}_GEN_ATSP_{size}.csv"
    elif use_gen == 2:
        out_name = f"{OUTPUT_DIR}{exp_name}_GEN_STSP_{size}.csv"
    else:
        base_name = os.path.basename(infile).split('.')[0]
        out_name = f"{OUTPUT_DIR}{exp_name}_TSPLIB_{base_name}.csv"

    # Usuń stary plik z tą samą nazwą, aby wyniki się nie dopisywały w nieskończoność
    if os.path.exists(out_name):
        os.remove(out_name)

    create_config(use_gen, size, infile, out_name, reps, t_limit, epoch, alpha, cool, ub, init_t)

    print(f"--> Uruchamianie testu: {os.path.basename(out_name)} ...")
    start_time = time.time()
    
    # Uruchomienie C++ (stdout przekierowany do DEVNULL, aby nie zaśmiecać konsoli)
    subprocess.run([EXE_PATH], stdout=subprocess.DEVNULL)
    
    elapsed = time.time() - start_time
    print(f"    Zakończono w {elapsed:.2f} s. Wyniki w: {out_name}\n")

# =====================================================================
# URUCHAMIANIE SCENARIUSZY
# =====================================================================

if __name__ == "__main__":
    print("ROZPOCZĘCIE AUTOMATYCZNYCH TESTÓW BENCHMARKOWYCH...\n")

    # -----------------------------------------------------------------
    # [3.0] Zależność czasu i błędu od rozmiaru (Generowane 7 instancji)
    # -----------------------------------------------------------------
    if RUN_SIZE_TEST:
        print("=== TEST ROZMIARÓW [3.0] ===")
        sizes = [10, 20, 30, 40, 50, 60, 70] # 7 rozmiarów wg zadania
        for s in sizes:
            # Optymalne parametry dla stabilnego wyżarzania
            run_experiment("SIZE", use_gen=1, size=s, reps=10, epoch=1000, ub=0, init_t=0.0) # ATSP
            run_experiment("SIZE", use_gen=2, size=s, reps=10, epoch=1000, ub=0, init_t=0.0) # STSP

    # -----------------------------------------------------------------
    # Zadanie TSPLIB (5 TSP, 5 ATSP + instancja 15-minutowa)
    # -----------------------------------------------------------------
    if RUN_TSPLIB_TEST:
        print("=== TEST TSPLIB (Błąd < 15%) ===")
        # Zdefiniuj swoje pliki z folderu input/
        tsplib_atsp = ["input/atsp/br17.atsp", "input/atsp/ftv33.atsp", "input/atsp/ftv47.atsp", "input/atsp/ftv64.atsp", "input/atsp/kro124p.atsp"]
        tsplib_tsp = ["input/tsp/berlin52.tsp", "input/tsp/st70.tsp", "input/tsp/pr76.tsp", "input/tsp/kroA100.tsp", "input/tsp/ch150.tsp"]
        
        # Ekstremalne wyżarzanie, by błąd był niski
        for file in tsplib_atsp + tsplib_tsp:
            run_experiment("TSPLIB", use_gen=0, infile=file, reps=5, epoch=2000, alpha=0.99, ub=0)

        # Miejsca na Twoją instancję 15-minutową (np. a280, pcb442)
        # run_experiment("TSPLIB_MAX", use_gen=0, infile="input/tsp/a280.tsp", t_limit=900, reps=1, epoch=5000, ub=0)

    # -----------------------------------------------------------------
    # [3.5] Wpływ UB (Nearest Neighbor) i LB
    # -----------------------------------------------------------------
    if RUN_UB_LB_TEST:
        print("=== TEST WPLYWU UB vs LOSOWY [3.5] ===")
        # Test na jednej średniej instancji (np. wielkość 40)
        # Z UB = 1
        run_experiment("UBLB_TakUB", use_gen=1, size=40, reps=10, ub=1)
        # Bez UB (Losowy start) = 0
        run_experiment("UBLB_BrakUB", use_gen=1, size=40, reps=10, ub=0)

    # -----------------------------------------------------------------
    # [4.0] Schematy Chłodzenia
    # -----------------------------------------------------------------
    if RUN_COOLING_TEST:
        print("=== TEST SCHEMATÓW CHŁODZENIA [4.0] ===")
        # 0 = Geometryczne (T *= alpha), 1 = Linowe (T -= alpha)
        # Ustawiamy dużą stałą temperaturę, żeby zobaczyć dokładny proces spadku
        run_experiment("COOL_Geom", use_gen=1, size=30, reps=10, cool=0, alpha=0.99, init_t=1000.0)
        
        # Dla liniowego, alpha musi być kompletnie inna (np. odejmujemy 0.5 stopnia na epokę)
        run_experiment("COOL_Lin", use_gen=1, size=30, reps=10, cool=1, alpha=0.5, init_t=1000.0)

    # -----------------------------------------------------------------
    # [4.5] Wpływ Długości Epoki
    # -----------------------------------------------------------------
    if RUN_EPOCH_TEST:
        print("=== TEST DŁUGOŚCI EPOKI [4.5] ===")
        epochs = [10, 100, 1000, 5000]
        for ep in epochs:
            run_experiment(f"EPOCH_{ep}", use_gen=1, size=30, reps=10, epoch=ep, ub=0)

    # -----------------------------------------------------------------
    # [5.0] Wpływ Temperatury Początkowej
    # -----------------------------------------------------------------
    if RUN_TEMP_TEST:
        print("=== TEST TEMPERATURY POCZĄTKOWEJ [5.0] ===")
        # 0.0 = wyliczanie automatyczne (najlepsze)
        temps = [0.0, 10.0, 1000.0, 100000.0]
        for t in temps:
            run_experiment(f"TEMP_{t}", use_gen=1, size=30, reps=10, init_t=t, ub=1) # UB=1, żeby pokazać jak T rozbija zachłanny wynik

    print("\nWSZYSTKIE WŁĄCZONE TESTY ZOSTAŁY ZAKOŃCZONE!")