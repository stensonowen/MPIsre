// mpicc mpi_primes.c -Wall -Wextra -lm
// timeout -s 10 10s mpiexec -n 4 a.out
// Isn't it great when lines just align
// The benefit of a monospaced typeface
// is the illusion of order it gives me
// For six lines, the world makes sense

#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

#define UPPER_BOUND UINT_MAX

int end_now = 0;

typedef struct {
    uint32_t lower;
    uint32_t upper;
} range;

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        end_now = 1;
    }
}

int is_prime(uint32_t n) {
    // NOTE: it's not that efficient to run this repeatedly
    // can speed up by not checking multiples of 2
    if (n == 2 || n == 3 || n == 5 || n == 7) return 1;
    uint32_t i, sr = (uint32_t)sqrt((double)n) + 1;
    for (i = 2; i < sr; i++) {
        if ((n % i) == 0) {
            return 0;
        }
    }
    return 1;
}

range next_local_tasks(range global, int id, int count) {
    range new_local_tasks;
    uint32_t tasks = (global.upper - global.lower) / count;
    new_local_tasks.lower = global.lower + tasks * id;
    if(id == count - 1) {
        // give a little extra work to the final proc
        // if the number of primes to check is not a multiple
        // of the number of threads
        new_local_tasks.upper = global.upper;
    } else {
        new_local_tasks.upper = new_local_tasks.lower + tasks;
    }
    return new_local_tasks;
}

range next_global_range(range old) {
    range new_global;
    new_global.lower = old.upper;
    if (10 * (uint64_t)old.upper >= UPPER_BOUND) {
        new_global.upper = UPPER_BOUND;
    }
	else {
        new_global.upper = 10 * old.upper;
    }
    return new_global;
}

int main(int argc, char **argv) {
    int count, id;
    range global = { 2, 10 };
    range local = global;
    uint32_t i;
    // keep track of the number of primes found by this thread
    uint32_t local_num_primes = 0;
    // track total primes found by all threads within `global` range
    uint32_t global_num_primes = 0;
    // track total primes found by all threads before `global` range
    uint32_t global_old_primes = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &count);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    signal(SIGUSR1, sig_handler);

    if(id == 0) {
        printf("\t\tN\t\tPrimes\n");
    }

    // proc 0 sends signal to begin
    // each thread computes its numbers to test and checks each
    //  if end_now is tripped, they return early
    // once they exhaust their numbers, they percolate that into to proc 0
    // once proc 0 has everyone's work, it prints the results 
    //  then it adjusts the global range to be computing and signals to resume
    
    // this means a fair amount of downtime because the last thread will have more work
    //  and every other thread will be waiting on it
    
    // also note this is a pretty naive way of finding primes
    // a sieve approach owuld mean much less work and better order notation
    // but who cares
    
    while (1) {
        // each thread computes which numbers to check (e.g. 100-125)
        local = next_local_tasks(global, id, count);
        local_num_primes = 0;
        global_num_primes = 0;

        for(i = local.lower; i < local.upper; ++i) {
            // check if each of my numbers is prime
            local_num_primes += is_prime(i);

            if (end_now == 1) {
                if (id == 0) {
                    printf("<Signal received>\n");
                }
                break;
            }
        }

        // communicate local counts to thread 0
        MPI_Reduce(&local_num_primes, &global_num_primes, 1, MPI_UNSIGNED, 
                MPI_SUM, 0, MPI_COMM_WORLD);

        if(end_now == 1) {
            if(id == 0) {
                printf("\t\t%d\t\t%d\n", i, global_old_primes + local_num_primes);
            }
            break;
        } else if(global.lower >= UPPER_BOUND) {
            break;
        }

        global_old_primes += global_num_primes;
        if (id == 0) {
            printf("\t\t%d\t\t%d\n", global.upper, global_old_primes);
        }

        // start on the next range of numbers (e.g. 1000-10000)
        global = next_global_range(global);
    }

    MPI_Finalize();
    return 0;
}
