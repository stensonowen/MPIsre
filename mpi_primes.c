#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

int32_t end_now = 0;

typedef struct {
    uint32_t lower;
    uint32_t upper;
} range;

range global = { 2, 10 };
//uint32_t global_lower = 2;
//uint32_t global_upper = 10;

void sig_handler(int signo) {
    printf("<Signal received>\n");
    if (signo == SIGUSR1) {
        end_now = 1;
    }
}

int is_prime(uint32_t n) {
    uint32_t i;
    uint32_t sr = (uint32_t)sqrt((double)n) + 1;
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
    // TODO: off by one at the end?
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
    range local = global;
    uint32_t tmp, i; 
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
    //  MPI_Barrier
    // each thread computes its numbers to test and checks each
    //  if end_now is tripped, they return early
    // once they exhaust their numbers, they percolate that into to proc 0
    // once proc 0 has everyone's work, it prints the results 
    //  then it adjusts the global range to be computing and signals to resume
    
    // this means a fair amount of downtime because the last thread will have more work
    //  and every other thread will be waiting on it
    //  this can be tweaked with 

    while (1) {
        if (end_now == 1) {
            break;
        }

        local = next_local_tasks(global, id, count);

        for(i=local.lower; i<local.upper; ++i) {
            local_num_primes += is_prime(i);
        }
        
        MPI_Reduce(&local_num_primes, &global_num_primes, 1, MPI_UNSIGNED, 
                MPI_SUM, 0, MPI_COMM_WORLD);

        if(id == 0) {
            printf("Percolated to id 0! The total num of primes is %d\n", global_num_primes);
            global = next_global_range(global);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        //if(local_lower == local_upper) {
        //    local_upper *= 10;
        //}
    }

    MPI_Finalize();

    return 0;
}
