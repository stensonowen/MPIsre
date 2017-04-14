// mpicc mpi_primes.c -Wall -Wextra -lm
// timeout -s 10 10s mpiexec -n 4 a.out
// Isn't it great when lines just align
// The benefit of a monospaced typeface
// is the illusion of order it gives me
// For six lines, the world makes sense

#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

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
    uint32_t i, sr = (uint32_t)sqrt((double)n)+1;
    for(i=2; i<sr; ++i) {
        if(n % i == 0) {
            return 0;
        }
    }
    return 1;
}

range next_local_tasks(range global, int id, int count) {
    range new_local_tasks;
    uint32_t tasks = (global.upper - global.lower) / count;
    new_local_tasks.lower = global.lower + tasks * id;
    new_local_tasks.upper = new_local_tasks.lower + tasks;
    // TODO: chance of off by one at the end?
    return new_local_tasks;
}

range next_global_range(range old) {
    range new_global;
    new_global.lower = old.upper;
    if(10*(uint64_t)old.upper >= 4294967295LL) {
        new_global.upper = 4294967295;
    } else {
        new_global.upper = 10*old.upper;
    }
    return new_global;
}

int main(int argc, char **argv) {
    int count, id;
    range global = { 2, 10 };
    range local = global;
    uint32_t i; 
    uint32_t local_num_primes = 0;
    uint32_t global_num_primes = 0;

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

        for(i=local.lower; i<local.upper; ++i) {
            // check if each of my numbers is prime
            local_num_primes += is_prime(i);

            if (end_now == 1) { // TODO: handle interrupt properly? 
                // spec pretty ambiguous, so can probably just do nothing
                // need to print output one last time though
                // maybe need some tweaking to get this to make sense
                if(id == 0) {
                    printf("<Signal received>\n");
                    //printf("\t\t%d\t\t%d\n", i, global_num_primes);
                }
                break;
            }
        }
        
        // communicate local counts to thread 0
        MPI_Reduce(&local_num_primes, &global_num_primes, 1, MPI_UNSIGNED, 
                MPI_SUM, 0, MPI_COMM_WORLD);

        if(id == 0) {
            printf("\t\t%d\t\t%d\n", global.upper, global_num_primes);
        }

        // start on the next range of numbers (e.g. 1000-10000)
        global = next_global_range(global);

        // sync threads
        // I don't think this is actually necessary
        // because it's already being done by MPI_Reduce
        // and thread 0 doesn't need to communicate with the others
        //MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
