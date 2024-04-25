#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <iostream>
#include <time.h>
#include <omp.h>
#include <cmath>
#include <vector>
#include <limits.h>
#include <iterator>


double loss_prev = INT_MAX;
double E = 0.00001;
int n = 13870;
// int n = 13650;
std::vector<double> simple_iteration_method(const std::vector<double> &A, const std::vector<double> &b,
                                        int count, double b_znam) {
    std::vector<double> x(n, 0.0);
    loss_prev = INT_MAX;
    double tet = 0.0001;
    double err = 1;
    
    while (err > E) {
        std::vector<double> x_new(n, 0.0);
        std::vector<double> err_chisl(n, 0.0);
        double chisl = 0;
        #pragma omp parallel num_threads(count)  // сами делаем работу omp parallel for (+ охватываем проверку)
        {
            int nthreads = omp_get_num_threads();
            int threadid = omp_get_thread_num();
            int items_per_thread = n / nthreads;
            int lb = threadid * items_per_thread;
            int ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);

            for (int i = lb; i <= ub; i++) {
                x_new[i] = 0;
                for (int j = 0; j < n; j++)
                {
                    x_new[i] += A[i * n + j] * x[j];
                }
                x_new[i] -= b[i];
                err_chisl[i] = pow(x_new[i], 2);
                x_new[i] = x[i] - tet * x_new[i];

                #pragma omp atomic
                chisl += err_chisl[i];
            }
        #pragma omp barrier  // ждем пока все потоки суммируют chisl, чтобы проверить условие выхода
        #pragma omp single  // выполняется на одном потоке, т.к. вычисления одинаковые
        {
        err = sqrt(chisl) / sqrt(b_znam);
        if ((loss_prev - err) < 0.0001)
            tet = tet * -1;
        loss_prev = err;
        x = x_new;  // присваивание работает за О(1), чем copy O(n)
        }
        //#pragma omp barrier  // как-будто нет смысла, поэтому закомментировано
        }
    }
    return x;
}


// int main(int argc, char* argv[]) {
//     int cnt[8] = {1,2,4,7,8,16,20,40};
//     double result[8] = {40, 40, 40, 40, 40, 40, 40, 40};
//     std::vector<double> A(n * n, 1.0);
    
//     for(int g = 0; g < 3; g++){
//         for(int k = 0; k < 8; k++){

//             #pragma omp parallel for num_threads(cnt[k])
//             for (int i = 0; i < n; i++)
//                 A[i * n + i] = 2.0;
//             std::vector<double> b(n, 1 + n);
//             double b_znam = pow(n + 1, 2) * n;

        
//             double t1 = omp_get_wtime();
//             std::vector<double> solution = simple_iteration_method(A, b, cnt[k], b_znam);
//             t1 = omp_get_wtime() - t1;
            
//             // std::cout << "Решение системы:" << std::endl;
//             // for (int i = 0; i < std::min(10, n); ++i)
//             //         std::cout << "x[" << i << "] = " << solution[i] << std::endl;
//             if(result[k] > t1)
//                 result[k] = t1;
            
//             // printf("%.6f\n", t1);
//         }
//         printf(">>>>iteration:%d\n", g);

//     }

//     for(int i = 0; i < 8; i++){
//         printf("%d threads: \n", cnt[i]);
//         printf("%.6f\n", result[i]);
//     }

//     return 0;
// }


int main(int argc, char* argv[]) {
    int num_threads = 1;
    if (argc > 1)
        num_threads = atoi(argv[1]);
    std::vector<double> A(n * n, 1.0);
    
    #pragma omp parallel for num_threads(num_threads)
    for (int i = 0; i < n; i++)
        A[i * n + i] = 2.0;
    std::vector<double> b(n, 1 + n);
    double b_znam = pow(n + 1, 2) * n;

    double t1 = omp_get_wtime();
    std::vector<double> solution = simple_iteration_method(A, b, num_threads, b_znam);
    t1 = omp_get_wtime() - t1;

    // std::cout << "Решение системы:" << std::endl;
    // for (int i = 0; i < std::min(10, n); ++i)
    //     std::cout << "x[" << i << "] = " << solution[i] << std::endl;
    std::cout << t1 << "\n";
    return 0;
}
