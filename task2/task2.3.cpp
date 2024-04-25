#include <iostream>
#include <string.h>
#include <omp.h>
#include <malloc.h>
#include <time.h>
#include <math.h>


using namespace std;
int N = 2200;
long long max_iters = 2400;
const double EPSILON = 0.000001; // требуемая точность

// Вариант 1: для каждого распараллеливаемого цикла создается отдельная параллельная секция #pragma omp parallel for,
double pragma_with_for(double* A, double* x, int num_of_threads) {
    for (int i = 0; i < N; ++i)
        x[i] = 0;
    double g = 0, sum = 0; 
    int i = 0, j = 0;
    double t = omp_get_wtime();  // замеряем время
    for (int iter = 0; iter < max_iters; ++iter) {
        g = 0;
        #pragma omp parallel for num_threads(num_of_threads)
        for (i = 0; i < N; ++i) {
            sum = 0;
            for (j = 0; j < N; ++j) {
                sum += A[i * N + j] * x[j];
            }
            double b = A[i * N + N];
            g += ((sum - b) * (sum - b)) / (b * b);
            x[i] -= 0.01 * (sum - b);
        }
        if (g < EPSILON * EPSILON)  // условие ОСТАНОВКИ
            break;
    }
    t = omp_get_wtime() - t;
    printf("Elapsed time (for):     %.6f\n", t);
    // cout << "with for:    "<< t << "\n";
    return t;
}

// Вариант 2: создается одна параллельная секция #pragma omp parallel, охватывающая весь итерационный алгоритм.
double pragma_without_for(double* A, double* x, int num_of_threads) {
    for (int i = 0; i < N; ++i)
        x[i] = 0;
    double g = 0, sum = 0; 
    int i = 0, j = 0;
    double t = omp_get_wtime();  // замеряем время
    #pragma omp parallel num_threads (num_of_threads)
    {
        for (int iter = 0; iter < max_iters; ++iter) {
            g = 0;
            for (i = 0; i < N; ++i) {
                sum = 0;
                for (j = 0; j < N; ++j) {
                    sum += A[i * N + j] * x[j];
            }
            double b = A[i * N + N];
            g += ((sum - b) * (sum - b)) / (b * b);
            x[i] -= 0.01 * (sum - b);
            }
            if (g < EPSILON * EPSILON)  // условие ОСТАНОВКИ
                break;
        }
    }
    t = omp_get_wtime() - t;
    printf("Elapsed time (not for): %.6f\n", t);
    // cout << "without for: " << t << "\n";
    return t;
}


int main(int argc, char **argv) {
    double* A = new double [N * N + N];
    // исходные данные для обоих вариантов
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N + 1; ++j) {
            A[i * N + j] = 1;
        }
        A[i * N + i] = 2;
        A[i * N + N] = N + 1;
    }

    double* x = new double[N] {0};
    int cnt[8] = {1,2,4,7,8,16,20,40};  // массив потоков для теста
    for(int i = 0; i < 8; i++) {
        printf("%d threads: \n", cnt[i]);
        double res_for = pragma_with_for(A, x, cnt[i]);
        double res_not_for = pragma_without_for(A, x, cnt[i]);
        printf("Speedup = %.6f \n", res_not_for/res_for);
    }

    // double res_for = pragma_with_for(A, x, 4);
    // double res_not_for = pragma_without_for(A, x, 4);
    // printf("Speedup = %.6f \n", res_not_for/res_for);

    delete[] A;
    delete[] x;

    return 0;
}
