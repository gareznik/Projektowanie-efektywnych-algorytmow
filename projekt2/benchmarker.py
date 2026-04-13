import subprocess
import os

EXE_PATH = "./projekt2.exe"
CONFIG_FILE = "config.txt"

# Lista plikow do przetestowania (musza byc w folderze input!)
test_files = [
    "input/br17.atsp",
    "input/ftv33.atsp",
    "input/att48.tsp"
]

def update_config(file_path):
    with open(CONFIG_FILE, "r") as f:
        lines = f.readlines()
        
    with open(CONFIG_FILE, "w") as f:
        for line in lines:
            if line.startswith("use_generator="):
                f.write("use_generator=0\n")
            elif line.startswith("input_file="):
                f.write(f"input_file={file_path}\n")
            else:
                f.write(line)

print("--- Rozpoczecie Auto-Testow ---")

# Tworzymy folder output, jesli go nie ma
if not os.path.exists("output"):
    os.makedirs("output")

for file in test_files:
    if os.path.exists(file):
        print(f"\n[Testowanie pliku]: {file}")
        update_config(file)
        subprocess.run([EXE_PATH])
    else:
        print(f"Plik {file} nie istnieje. Pomin.")
        
print("\n--- Testy zakonczone. Wyniki w output/results.csv ---")