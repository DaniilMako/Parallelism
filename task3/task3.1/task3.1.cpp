#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

void matrix_vector_product(const std::vector<std::vector<int>>& matrix, const std::vector<int>& vector, std::vector<int>& result, int lb, int ub) {
    for (int i = lb; i < ub; ++i) {
        result[i] = 0;
        for (int j = 0; j < matrix[i].size(); ++j)
            result[i] += matrix[i][j] * vector[j];
    }
}

void parallelArrayInit(std::vector<int>& array, int start, int end) {
    for (int i = start; i < end; ++i)
        array[i] = i;
}

int main(int argc, char* argv[])
{  // N = M
    size_t M = 20000;

    if (argc > 1)
        M = atoi(argv[1]);

    std::vector<int> list_num_threads = {1, 2, 4, 7, 8, 16, 20, 40};
    std::vector<double> runtimes(list_num_threads.size());

    for (int i = 0; i < list_num_threads.size(); i++) {
        int num_threads = list_num_threads[i];

        std::vector<std::vector<int>> matrix(M, std::vector<int>(M, 1));
        std::vector<int> vector(M, 2);
        std::vector<int> result(M);


        std::vector<std::thread> threads;  // контейнер для объектов потоков
        int range_size = M / num_threads;  // сколько строчек возьмет на себя один поток

        for (int k = 0; k < num_threads; ++k)  // параллельная инициализация, добавляя объекты потоков в конец вектора
            threads.emplace_back(parallelArrayInit, std::ref(vector), k * range_size, (k + 1) * range_size);
            // threads.push_back(std::thread(parallelArrayInit, std::ref(vector), k * range_size, (k + 1) * range_size));  // то же самое

        for (auto& thread : threads)
            thread.join();  // ожидание завершения потока


        auto start_time = std::chrono::high_resolution_clock::now();  // время старта

        for (int k = 0; k < num_threads; ++k)  // каждый поток вызывает функцию matrix_vector_product для своего диапазона значений
            threads[k] = std::thread(matrix_vector_product, std::ref(matrix), std::ref(vector), std::ref(result), k * range_size, (k + 1) * range_size);

        for (auto& thread : threads)
            thread.join();  // ожидание завершения потока

        auto end_time = std::chrono::high_resolution_clock::now();  // время финиша

        double runtime = std::chrono::duration<double>(end_time - start_time).count();  // время работы на num_threads[i] потоках

        runtimes[i] = runtime;

        double speedup = runtimes[0] / runtime;

        std::cout << "Runtime with " << num_threads << " threads and matrix size " << M << ": " << runtime << " seconds\n";
        std::cout << "Speedup with " << num_threads << " threads and matrix size " << M << ": " << speedup << std::endl << std::endl;
    }

    return 0;
}
