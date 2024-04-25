#include <stdio.h>
#include <math.h>
#include <time.h>
#include <omp.h>

const double PI = 3.14159265358979323846;
const double a = -4.0;
const double b = 4.0;
const int nsteps = 40000000;

double cpuSecond()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9);
}
// Численное интегрирование (метод прямоугольников)
double func(double x)
{
    return exp(-x * x);
}

double integrate(double (*func)(double), double a, double b, int n)
{
    double h = (b - a) / n;
    double sum = 0.0;

    for (int i = 0; i < n; i++)
        sum += func(a + h * (i + 0.5));

    sum *= h;

    return sum;
}
// Параллельная версия
double integrate_omp(double (*func)(double), double a, double b, int n, int count)
{
    double h = (b - a) / n;
    double sum = 0.0;

    #pragma omp parallel num_threads(count)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = n / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);
        double sumloc = 0.0;

        for (int i = lb; i <= ub; i++)
            sumloc += func(a + h * (i + 0.5));

        #pragma omp atomic
        sum += sumloc;
    }
    sum *= h;
    return sum;
}

double run_serial()
{
    double t = cpuSecond();
    double res = integrate(func, a, b, nsteps);
    t = cpuSecond() - t;
    printf("Result (serial): %.12f; error %.12f\n", res, fabs(res - sqrt(PI)));
    return t;
}
double run_parallel(int count)
{
    double t = cpuSecond();
    double res = integrate_omp(func, a, b, nsteps, count);
    t = cpuSecond() - t;
    printf("Result (parallel): %.12f; error %.12f\n", res, fabs(res - sqrt(PI)));
    return t;
}

int main(int argc, char **argv) {
    int cnt[8] = {1,2,4,7,8,16,20,40};
    printf("Integration f(x) on [%.12f, %.12f], nsteps = %d\n", a, b, nsteps);  // nsteps - число точек интегрирования
    double tserial = run_serial();
    printf("Execution time (serial)  : %.6f\n", tserial);

    for(int i = 0; i < 8; i++){
        printf("%d threads: \n", cnt[i]);
        double tparallel = run_parallel(cnt[i]);
        printf("Execution time (parallel): %.6f\n", tparallel);
        printf("Speedup: %.2f\n", tserial / tparallel);
    }
    // Result (parallel): 1.772453823579; error 0.000000027326
    // Result (serial):   1.772453823579; error 0.000000027326 
    return 0;
}
