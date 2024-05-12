#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>


double cpuSecond()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9);
}

/*
 * matrix_vector_product: Compute matrix-vector product c[m] = a[m][n] * b[n]
 */
void matrix_vector_product(double *a, double *b, double *c, size_t m, size_t n)
{
    for (int i = 0; i < m; i++)
    {
        c[i] = 0.0;
        for (int j = 0; j < n; j++)
            c[i] += a[i * n + j] * b[j];
    }
}
/*
    matrix_vector_product_omp: Compute matrix-vector product c[m] = a[m][n] * b[n]
*/
void matrix_vector_product_omp(double* a, double* b, double* c, int m, int n, int num_of_threads)
{
    #pragma omp parallel num_threads(num_of_threads)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = m / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1);
        for (int i = lb; i <= ub; i++) {
            c[i] = 0.0;
            for (int j = 0; j < n; j++)
                c[i] += a[i * n + j] * b[j];

        }
    }
}

double run_serial(size_t n, size_t m)
{
    double* a, * b, * c;
    a = (double*)malloc(sizeof(*a) * m * n);
    b = (double*)malloc(sizeof(*b) * n);
    c = (double*)malloc(sizeof(*c) * m);

    if (a == NULL || b == NULL || c == NULL)
    {
        free(a);
        free(b);
        free(c);
        printf("Error allocate memory!\n");
        exit(1);
    }

    for (size_t i = 0; i < m; i++)
    {
        for (size_t j = 0; j < n; j++)
            a[i * n + j] = i + j;
    }

    for (size_t j = 0; j < n; j++)
        b[j] = j;

    double t = cpuSecond();
    matrix_vector_product(a, b, c, m, n);
    t = cpuSecond() - t;

    printf("Elapsed time (serial): %.6f sec.\n", t);
    free(a);
    free(b);
    free(c);
    return t;
}


double run_parallel(size_t n, size_t m, int num_of_threads)
{
    double* a, * b, * c;

    a = (double*)malloc(sizeof(*a) * m * n);
    b = (double*)malloc(sizeof(*b) * n);
    c = (double*)malloc(sizeof(*c) * m);

    if (a == NULL || b == NULL || c == NULL)
    {
        free(a);
        free(b);
        free(c);
        printf("Error allocate memory!\n");
        exit(1);
    }
    #pragma omp parallel num_threads(num_of_threads)
    {
        int nthreads = omp_get_num_threads();  // кол-во потоков
        int threadid = omp_get_thread_num();  // id/номер потока
        int items_per_thread = m / nthreads;  // кол-во итераций за один поток
        int lb = threadid * items_per_thread;  // нижная граница
        int ub = (threadid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1);  // верхняя граница

        for (size_t i = lb; i <= ub; i++)
        {
            for (size_t j = 0; j < n; j++)
                a[i * n + j] = i + j;
        }

        items_per_thread = n / nthreads;
        lb = threadid * items_per_thread;
        ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);

        for (size_t j = lb; j <= ub; j++)
            b[j] = j;
    }
    double t = cpuSecond();
    matrix_vector_product_omp(a, b, c, m, n, num_of_threads);
    t = cpuSecond() - t;

    printf("Elapsed time (parallel): %.6f sec.\n", t);
    free(a);
    free(b);
    free(c);
    return t;
}

int main(int argc, char* argv[])
{
    size_t M = 20000;
    size_t N = 20000;
    // int num_of_threads = 2;

    if (argc > 1)
        M = atoi(argv[1]);
    if (argc > 2)
        N = atoi(argv[2]);
    // if (argc > 3)
    //     count = atoi(argv[3]);

    int cnt[8] = {1,2,4,7,8,16,20,40};
    printf("M=N=%d\n", M);
    double res_serial = run_serial(M, N);
    for(int i = 0; i < 8; i++){
        printf("%d threads: \n", cnt[i]);
        printf("Speedup = %.6f \n", res_serial/run_parallel(M, N, cnt[i]));
    }

    return 0;
}
