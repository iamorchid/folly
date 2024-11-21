#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <linux/futex.h>
#include <limits.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

int my_futex = 0; // The futex word

// Futex system call wrapper
int futex(int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, int val3) {
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr, val3);
}

// Lock the futex
void futex_lock(int *futexp) {
    int one = 1;
    if (__sync_bool_compare_and_swap(futexp, 0, one)) {
        return; // Successfully acquired the lock
    }

    // Futex is already locked by another thread, need to wait
    while (1) {
        // FUTEX_WAIT: Wait if the futex value is still 1
        if (futex(futexp, FUTEX_WAIT, one, NULL, NULL, 0) == -1 && errno != EAGAIN) {
            perror("FUTEX_WAIT");
            exit(EXIT_FAILURE);
        }

        // Atomically try to acquire the lock again
        if (__sync_bool_compare_and_swap(futexp, 0, one)) {
            return; // Successfully acquired the lock
        }
    }
}

// Unlock the futex
void futex_unlock(int *futexp) {
    int zero = 0;
    if (__sync_bool_compare_and_swap(futexp, 1, zero)) {
        // FUTEX_WAKE: Wake one of the threads waiting for the futex
        if (futex(futexp, FUTEX_WAKE, 1, NULL, NULL, 0) == -1) {
            perror("FUTEX_WAKE");
            exit(EXIT_FAILURE);
        }
    }
}

// Worker function
void *worker(void *arg) {
    (void)arg; // Unused parameter

    printf("Thread trying to lock futex...\n");
    futex_lock(&my_futex);
    printf("Thread has the lock.\n");
    sleep(1); // Simulate work by sleeping for 1 second
    futex_unlock(&my_futex);
    printf("Thread released the lock.\n");

    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    // Create two threads that will contend for the futex
    pthread_create(&thread1, NULL, worker, NULL);
    pthread_create(&thread2, NULL, worker, NULL);

    // Wait for both threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}
