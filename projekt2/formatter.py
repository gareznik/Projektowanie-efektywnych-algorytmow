import os
import csv

INPUT_DIR = "output"
OUTPUT_DIR = "output_formatted"

print("="*50)
print(" ПРЕОБРАЗОВАНИЕ CSV В ШИРОКИЙ ФОРМАТ ")
print("="*50)

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# Ищем все csv файлы в папке output
for filename in os.listdir(INPUT_DIR):
    if not filename.endswith(".csv"):
        continue
        
    input_path = os.path.join(INPUT_DIR, filename)
    output_path = os.path.join(OUTPUT_DIR, "formatted_" + filename)
    
    # Словарь для группировки данных
    # Ключ: (N, Номер_инстанции, Стоимость)
    # Значение: словарь с временем и памятью алгоритмов
    grouped_data = {}
    
    with open(input_path, "r", encoding="utf-8") as f:
        for line in f:
            parts = line.strip().split(',')
            # Пропускаем пустые или битые строки
            if len(parts) < 6: 
                continue
                
            algo = parts[0].strip()
            
            # Если скрипт убил процесс, пропускаем эту строку
            if "KILLED" in algo:
                continue
                
            n = parts[1].strip()
            instance = parts[2].strip()
            cost = parts[3].strip()
            time_val = parts[4].strip()
            mem_val = parts[5].strip()
            
            key = (n, instance, cost)
            
            if key not in grouped_data:
                grouped_data[key] = {
                    "BFS_Time": "", "BFS_Mem": "",
                    "DFS_Time": "", "DFS_Mem": "",
                    "LC_Time": "", "LC_Mem": ""
                }
            
            if algo == "BFS":
                grouped_data[key]["BFS_Time"] = time_val
                grouped_data[key]["BFS_Mem"] = mem_val
            elif algo == "DFS":
                grouped_data[key]["DFS_Time"] = time_val
                grouped_data[key]["DFS_Mem"] = mem_val
            elif algo == "LowestCost":
                grouped_data[key]["LC_Time"] = time_val
                grouped_data[key]["LC_Mem"] = mem_val

    # Записываем переформатированные данные в новый файл
    with open(output_path, "w", encoding="utf-8", newline='') as f:
        writer = csv.writer(f)
        # Пишем красивую шапку (заголовки колонок)
        writer.writerow(["N", "Instance", "Cost", 
                         "BFS_Time(ms)", "DFS_Time(ms)", "LowestCost_Time(ms)", 
                         "BFS_Mem(KB)", "DFS_Mem(KB)", "LowestCost_Mem(KB)"])
        
        for key, vals in grouped_data.items():
            n, instance, cost = key
            writer.writerow([
                n, instance, cost,
                vals["BFS_Time"], vals["DFS_Time"], vals["LC_Time"],
                vals["BFS_Mem"], vals["DFS_Mem"], vals["LC_Mem"]
            ])
            
    print(f"Файл {filename} успешно преобразован!")

print("="*50)
print(f" ГОТОВО! Все новые файлы лежат в папке {OUTPUT_DIR}/")
print("="*50)