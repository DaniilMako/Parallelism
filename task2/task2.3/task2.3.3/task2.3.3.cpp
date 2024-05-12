#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <iostream>
#include <cmath>
#include <malloc.h>
#include <vector>
#include <limits.h>

double loss_prev = INT_MAX;

std::vector<double> simpleIterationMethod(const std::vector<double> &A, const std::vector<double> &b,
                                          double eps, int nm, int n, double b_znam)
{
    std::vector<double> x(n, 0.0);
    double tet = 0.0001;
    double err = 1;
    while (err > eps) {
        std::vector<double> x_new(n, 0.0);
        std::vector<double> err_chisl(n, 0.0);
        double chisl = 0;

        #pragma omp parallel for schedule(dynamic, n/nm) num_threads(nm)  // оставляем часть не занятыми, чтобы по готовности поток брал
        for (int i = 0; i < n; i++) {
            double sum = 0;
            for (int j = 0; j < n; j++)
                sum += A[i * n + j] * x[j];
            x_new[i] = sum - b[i];
            err_chisl[i] = pow(x_new[i], 2);
            x_new[i] = x[i] - tet * x_new[i];
            #pragma omp atomic
            chisl += err_chisl[i];
        }

        err = sqrt(chisl) / sqrt(b_znam);
        if ((loss_prev - err) < 0.0001)
            tet = tet * -1;
        loss_prev = err;
        x = x_new;
    }
    return x;
}

int main(int argc, char* argv[]) {
    int num_threads = 1;
    if (argc > 1)
        num_threads = atoi(argv[1]);
    int n = 13700;

    // Создание и заполнение одномерного массива для матрицы A
    std::vector<double> A(n * n, 1.0);
    #pragma omp parallel for num_threads(num_threads)
    for (int i = 0; i < n; i++)
        A[i * n + i] = 2.0;

    std::vector<double> b(n, 1 + n);
    double b_znam = pow(n + 1, 2) * n;
    double tolerance = 0.00001;
    double t1 = omp_get_wtime();
    std::vector<double> solution = simpleIterationMethod(A, b, tolerance, num_threads, n, b_znam);
    t1 = omp_get_wtime() - t1;
    // for (int i = 0; i < std::min(10, n); ++i)
    //     std::cout << "x[" << i << "] = " << solution[i] << std::endl;
    std::cout << t1 << std::endl;
    return 0;
}
