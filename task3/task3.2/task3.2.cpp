#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <fstream>
#include <random>

// структура для описания задачи
struct Task {
    int id;
    int operation_type;  // 1: sin, 2: sqrt, 3: pow
    double arg;          // значение для вычисления
    double result;
};

// шаблонный класс сервера
template<typename T>
class Server {
private:
    std::queue<Task> taskQueue;  // очередь задач
    std::vector<Task> results;   // результаты выполненных задач
    std::mutex mtx;              // мьютекс для обеспечения безопасности доступа к данным
    std::condition_variable cv;  // условная переменная для синхронизации потоков
    bool isRunning = true;       // флаг работы сервера
    std::thread serverThread;    // поток для выполнения задач

public:
    // запуск сервера
    void start() {
        serverThread = std::thread(&Server::process_tasks, this);
    }

    // остановка сервера
    void stop() {
        isRunning = false;
        cv.notify_all();      // уведомление всех потоков о завершении работы
        serverThread.join();  // ожидание завершения потока
    }

    // добавление задачи в очередь и возврат ее id
    size_t add_task(Task task) {
        std::unique_lock<std::mutex> lock(mtx);
        task.id = results.size();
        taskQueue.push(task);
        cv.notify_one();  // уведомление одного потока о наличии задачи
        return task.id;
    }

    // запрос результата выполнения задачи по ее id
    Task request_result(int id_res) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return results.size() > id_res; });  // ожидание, пока результат не станет доступным
        return results[id_res];
    }

private:
    // метод выполнения задач
    void process_tasks() {
        while (isRunning) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&] { return !taskQueue.empty() || !isRunning; });
                task = taskQueue.front();
                taskQueue.pop();
            }

            // выполнение операций в зависимости от типа задачи
            if (task.operation_type == 1) {
                task.result = static_cast<T>(std::sin(task.arg));
            } else if (task.operation_type == 2) {
                task.result = static_cast<T>(std::sqrt(task.arg));
            } else if (task.operation_type == 3) {
                task.result = static_cast<T>(std::pow(task.arg, 2));
            }

            {
                std::lock_guard<std::mutex> lock(mtx);
                results.push_back(task);
            }
        }
    }
};

// функция для создания клиента и добавления задач на сервер
template<typename T>
void client(Server<T>& server, int num_tasks, int type) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<T> dist(1, 100);

    for (int i = 0; i < num_tasks; ++i) {
        Task task;
        task.operation_type = type;
        task.arg = dist(gen);
        server.add_task(task);
    }
}


int main(int argc, char **argv) {
    Server<double> server;
    server.start();  // запуск сервера
    int N = 10;
    if (argc > 1)
        N = atoi(argv[1]);

    // создание клиентов и добавление задач
    std::thread client1(client<double>, std::ref(server), N, 1);  // sin
    std::thread client2(client<double>, std::ref(server), N, 2);  // sqrt
    std::thread client3(client<double>, std::ref(server), N, 3);  // pow

    // ожидание завершения потоков
    client1.join();
    client2.join();
    client3.join();

    // запись результатов в файлы
    std::ofstream file1("sin_results.txt");
    std::ofstream file2("sqrt_results.txt");
    std::ofstream file3("pow_results.txt");

    for (int i = 0; i < N; ++i) {
        auto request_result1 = server.request_result(i);
        auto request_result2 = server.request_result(i + N * 1);
        auto request_result3 = server.request_result(i + N * 2);
        file1 << "sin(" << request_result1.arg << ") = " << request_result1.result << std::endl;
        file2 << "sqrt(" << request_result2.arg << ") = " << request_result2.result << std::endl;
        file3 << request_result3.arg << "^2 = " << request_result3.result << std::endl;
    }

    file1.close();
    file2.close();
    file3.close();

    server.stop();  // остановка сервера
    return 0;
}
