import os
import random
import time

# =====================================================================
# НАСТРОЙКИ ГЕНЕРАТОРА
# =====================================================================
OUTPUT_DIR = "input/generated/"  # Папка, куда сохранятся матрицы
MAX_WEIGHT = 1000                # Максимальное расстояние между городами

# Какие графы сгенерировать: список кортежей (размер, is_symmetric, количество)
# False = ATSP, True = STSP
BATCH_CONFIG = [
    (20, False, 1),   # 1 шт. ATSP на 20 городов
    (50, False, 1),   # 1 шт. ATSP на 50 городов
    (100, False, 1),  # 1 шт. ATSP на 100 городов
    (50, True, 1),    # 1 шт. STSP на 50 городов
]

# Создаем папку, если её нет
if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# =====================================================================
# АЛГОРИТМЫ
# =====================================================================
def generate_matrix(size, is_symmetric):
    """Генерирует матрицу смежности (ATSP или STSP)"""
    matrix = [[-1 for _ in range(size)] for _ in range(size)]
    
    for i in range(size):
        for j in range(size):
            if i == j:
                matrix[i][j] = -1 # Расстояние до самого себя
            else:
                if is_symmetric:
                    if i < j:
                        dist = random.randint(1, MAX_WEIGHT)
                        matrix[i][j] = dist
                        matrix[j][i] = dist
                else:
                    matrix[i][j] = random.randint(1, MAX_WEIGHT)
    return matrix

def solve_rnn(matrix):
    """Вычисляет UB с помощью Repetitive Nearest Neighbor (RNN)"""
    n = len(matrix)
    best_overall_cost = float('inf')

    for start_city in range(n):
        visited = [False] * n
        current = start_city
        visited[current] = True
        current_cost = 0

        # Проходим все остальные города
        for _ in range(1, n):
            next_city = -1
            min_dist = float('inf')
            
            for j in range(n):
                if not visited[j] and matrix[current][j] != -1 and matrix[current][j] < min_dist:
                    min_dist = matrix[current][j]
                    next_city = j
                    
            visited[next_city] = True
            current_cost += min_dist
            current = next_city
        
        # Возврат в стартовый город
        current_cost += matrix[current][start_city]
        
        if current_cost < best_overall_cost:
            best_overall_cost = current_cost
            
    return best_overall_cost

def save_to_tsplib(matrix, is_symmetric, size, rnn_cost, filename):
    """Сохраняет матрицу в формате TSPLIB с внедренным результатом RNN"""
    graph_type = "TSP" if is_symmetric else "ATSP"
    
    with open(filename, "w") as f:
        f.write(f"NAME: {os.path.basename(filename).split('.')[0]}\n")
        f.write(f"TYPE: {graph_type}\n")
        # ПРЯЧЕМ РЕЗУЛЬТАТ В КОММЕНТАРИИ:
        f.write(f"COMMENT: GENERATED MATRIX | RNN_UB={rnn_cost}\n")
        f.write(f"DIMENSION: {size}\n")
        f.write("EDGE_WEIGHT_TYPE: EXPLICIT\n")
        f.write("EDGE_WEIGHT_FORMAT: FULL_MATRIX\n")
        f.write("EDGE_WEIGHT_SECTION\n")
        
        for row in matrix:
            # Форматируем вывод так же, как в оригинальных файлах TSPLIB
            row_str = " ".join(f"{val:>4}" for val in row)
            f.write(f" {row_str}\n")
        f.write("EOF\n")

# =====================================================================
# ГЛАВНЫЙ ЦИКЛ
# =====================================================================
if __name__ == "__main__":
    print(f"--- Запуск генератора графов ---")
    
    for size, is_symmetric, count in BATCH_CONFIG:
        g_type = "STSP" if is_symmetric else "ATSP"
        
        for i in range(1, count + 1):
            # Генерируем красивое имя, например: ATSP_50_v1.atsp
            ext = "tsp" if is_symmetric else "atsp"
            filename = f"{OUTPUT_DIR}{g_type}_{size}_v{i}.{ext}"
            
            print(f"Генерация {g_type} на {size} городов... ", end="", flush=True)
            
            start_time = time.time()
            matrix = generate_matrix(size, is_symmetric)
            
            # Прогон RNN на Python
            rnn_cost = solve_rnn(matrix)
            
            # Сохранение файла
            save_to_tsplib(matrix, is_symmetric, size, rnn_cost, filename)
            
            elapsed = time.time() - start_time
            print(f"Готово! (RNN: {rnn_cost}) [{elapsed:.2f} сек] -> {filename}")

    print("--- Все графы успешно сгенерированы! ---")