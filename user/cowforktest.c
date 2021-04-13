#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void assert(int pid, char* msg, int assertion)
{
    if(!assertion)
    {
        printf("\x1b[31mError in process pid=%d\n", pid);
        printf("%s\n\x1b[0m", msg);
        exit(1);
    }
}

void do_read(char* buffer, char* filename, int bytes)
{
    printf("Reading some input from the %s...\n", filename);
    int fd = open(filename, O_RDONLY);
    close(fd);
    printf("Read %d bytes:\n%s\n", read(fd, buffer, bytes), buffer);
}

void cowfork_test(int showpages)
{
    printf("copy-on-write fork test\n\n");
    int startpages = freepages();
    printf("Starting free pages: %d\n", startpages);

    if (showpages) pages();

    int n = 0, pid = 0, err = 0;

    
    char buffer[40] = "start";
    char start[40] = "start";
    printf("Parent memory starting string: %s\n", start);

    // Parent process => Checking for nested child writing
    printf("\n==== Parent fork => Child 1 ====\n");
    pid = fork();

    if (pid == 0) // Child 1
    {
        if (showpages) pages();
        printf("Number of free pages in child 1: %d\n", freepages());
        printf("Writing something...(n = 100)\n");
        n = 100;
        printf("n = %d, address: %p\n", n, &n);
        printf("Number of free pages in child 1 after writing: %d\n", freepages());
        if (showpages) pages();


        // Fork again
        printf("\n==== Child 1 fork => Child 1's child ====\n");
        pid = fork();

        if (pid == 0) // Child 1's child
        {
            if (showpages) pages();
            printf("Number of free pages in child 1's child: %d\n", freepages());

            printf("Writing something...(n = n + 1)\n");
            n++;
            printf("n = %d, address: %p\n", n, &n);
            printf("n should be 101 if memory is copied (n = %d)\n", n);
            assert(pid, "Memory not copied correctly...", n == 101);

            printf("Number of free pages in child 1's child after writing: %d\n", freepages());
            if (showpages) pages();
            exit(0);
        }
        // Child 1
        wait(&err);
        assert(pid, "Child 1's child bad exit...", err == 0);
        exit(0);
    }
    // Parent process => Checking for single child writing
    wait(&err);
    assert(pid, "Child 1 bad exit...", err == 0);

    printf("\n==== Back in parent ====\n");
    printf("Number of free pages in parent: %d\n", freepages());
    if (showpages) pages();

    printf("\n==== Parent fork again => Child 2 ====\n");
    pid = fork();

    if (pid == 0) // Child 2
    {
        if (showpages) pages();
        printf("Number of free pages in child 2: %d\n", freepages());

        printf("Writing something...(n = 69)\n");
        n = 69;
        printf("n = %d\n", n);

        printf("Number of free pages in child 2 after writing: %d\n", freepages());
        if (showpages) pages();

        exit(0);
    }
    // Parent process => Checking for syscall read
    wait(&err);
    assert(pid, "Child 2 bad exit...", err == 0);

    printf("\n==== Back in parent ====\n");
    printf("Number of free pages in parent: %d\n", freepages());
    if (showpages) pages();

    printf("\n==== Parent fork again => Child 3 (Using syscall read to test copyout)====\n");
    int fd = open("README", O_RDONLY);
    pid = fork();

    if (pid == 0) // Child 3
    {
        // if (showpages) pages();

        // Test system call writes through copyout
        // This was ridiculously hard to make sure there was no attempt to write
        // Apparently printf is a write action, so I can't print any info...
        read(fd, buffer, sizeof(buffer));
        close(fd);

        // if (showpages) pages();
        exit(0);
    }
    // Parent process => Final checks
    wait(&err);
    assert(pid, "Child 3 bad exit...", err == 0);

    printf("\n==== Back in parent ====\n");
    printf("Number of free pages in parent: %d\n", freepages());
    if (showpages) pages();

    printf("\n==== Final checks ====\n");

    assert(pid, "Children leftover...", wait(0) == -1);

    printf("Starting free pages: %d\n", startpages);
    printf("Ending free pages: %d\n", freepages());
    
    assert(pid, "Not all pages returned...", startpages == freepages());

    printf("Parent memory starting string: %s\n", start);
    printf("Parent memory ending string: %s\n", buffer);
    assert(pid, "BAD SYSCALL WRITING: Parent memory being overwritten by child", !strcmp(buffer, "start"));

    printf("\n\x1b[32mcopy-on-write fork test OK\x1b[0m\n");
}

int
main(int argc, char *argv[])
{
    int printpages = 0;

    if(argc == 2 && strcmp(argv[1], "-p") == 0)
        printpages = 1;
    else if(argc > 1)
    {
        printf("Usage: cowforktest [-p]\n");
        printf("-p flag will print pages throughout the test\n");
        exit(1);
    }

    cowfork_test(printpages);
    exit(0);
}


/* CHANGES
- syscall for printing pages and getting num free pages
- Ref counts in kalloc.c | kmem struct, kfree(), kalloc(), some new functions
- added macro to get pagetable ref index in memlayout.h
- added PTE_COW in riscv.h
- fork -> uvmcopy
- copyout
- usertests...
 */

// Demonstrate your code with tests that show that it works. 
// How do you know that it is successfully forking and creating read-only memory pages? 
// How do you know that it is successfully cloning pages in response to a write request? 
// How do you know that does not clone a page when there is only one remaining reference to it? 
// How do you know that the pages are freed at the appropriate time (and not before)?

// Walk through the code that you wrote and explain how you implemented each mechanism.

// Explain anything that changed between your design and your implementation. What do you wish you had known that would have made the process easier?