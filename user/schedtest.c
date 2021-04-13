#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// use fib recursion for calc to guard against optimizations

// 1000 fib(20)'s takes ~4 seconds
int fib(int n)
{
    return n <= 1 ? n : fib(n - 2) + fib(n - 1);
}

// Useless setprio(0)calcs
void useless(int msg)
{
    for (int i = 0; i < 10000; i++)
    {
        fib(20);
        if (i % 1000 == 0)
        {
            // Print the priority at regular intervals
            if (msg > 60)
                printf("%c", msg);
            else
                printf("%d", msg);
        }
    }
}

void setpriotest(void)
{
    printf("Current priority: %d\n", setprio(0));
    printf("Setting priority to 3...\n");
    setprio(3);
    printf("New priority: %d\n", setprio(0));
    int pid = fork();
    if (pid == 0)
    {
        printf("Child current priority: %d\n", setprio(0));
        exit(0);
    }
    wait(0);
    printf("Parent current priority: %d\n", setprio(0));

    exit(0);
}

// Shows priority scheduling
void schedtest(void)
{
    int pid;

    // So no process gets blocked at startup
    setprio(5);

    for (int p = 1; p <= 15; p++)
    {
        int plvl = (p % 5) + 1;
        pid = fork();
        if (pid == 0) // child
        {
            setprio(plvl);
            // Start high prios later
            sleep(plvl * 20);
            useless(setprio(0));
            exit(0);
        }
    }
    wait(0);
    exit(0);
}

// Shows CPU equity
void timertest(void)
{
    int pid;

    for (int i = 0; i < 16; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            useless(i + 65);
            exit(0);
        }
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        timertest();
    }
    else
    {
        schedtest();
        // setpriotest();
    }

    exit(0);
}

// CHANGES

// sysproc.c -> added setprio syscall
// proc.h -> added prio lvl in proc struct declaration
// proc.c -> actual implementation of scheduler
// This test runner

/* Show how a process can change its priority
Show how the scheduler selects the next process to run
Show that processes with higher priority always run ahead of processes with lower priority
Show that processes with the same priority share the CPU equitably
Discuss design decisions and tradeoffs that you made */