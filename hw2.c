#define _GNU_SOURCE 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <signal.h>


#define FIFO1 "fifo1"
#define FIFO2 "fifo2"
#define LOGFILE "./daemon.log"
volatile sig_atomic_t child_count = 0;

void sigchld_handler();
void signal_handler(int sig);

// Daemon işlemini yapan fonksiyon
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // Parent exits

    if (setsid() < 0) exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // Second parent exits

    umask(0);
    chdir(".");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fd = open(LOGFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
    }
}

void sigchld_handler() {
    int status;
    pid_t pid;

    // Tüm bitmiş çocukları topla
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            dprintf(STDOUT_FILENO, "Child %d exited with status %d\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            dprintf(STDOUT_FILENO, "Child %d terminated by signal %d\n", pid, WTERMSIG(status));
        } else {
            dprintf(STDOUT_FILENO, "Child %d exited abnormally\n", pid);
        }

        child_count += 2;  // Ödev gereği her çocuk için +2
        dprintf(STDOUT_FILENO, "child_count updated: %d\n", child_count);
    }

    fsync(STDOUT_FILENO);  // Log'u diske yaz
}


void signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
            dprintf(STDOUT_FILENO, "SIGTERM received. Exiting daemon.\n");
            exit(0);
        case SIGHUP:
            dprintf(STDOUT_FILENO, "SIGHUP received. (Reload config not implemented)\n");
            break;
        case SIGUSR1:
            dprintf(STDOUT_FILENO, "SIGUSR1 received. (Custom user action)\n");
            break;
    }
    fsync(STDOUT_FILENO);
}

int main(int argc, char *argv[]) {
    pid_t pid1 = -1, pid2 = -1;
    int result = 0;
    
    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./hw2 <int1> <int2>\n", 28);
        exit(EXIT_FAILURE);
    }
    printf("Arguments received: %s, %s\n", argv[1], argv[2]);

    unlink(FIFO1);
    unlink(FIFO2);

    // Create the FIFOs and send data
    if (access(FIFO1, F_OK) == -1) {
        dprintf(STDOUT_FILENO, "FIFO1 does not exist. Creating...\n");
        if (mkfifo(FIFO1, 0666) == -1) {
            perror("Failed to create FIFO1");
            exit(EXIT_FAILURE);
        }
    } else {
        dprintf(STDOUT_FILENO, "FIFO1 already exists.\n");
    }
    
    if (access(FIFO2, F_OK) == -1) {
        dprintf(STDOUT_FILENO, "FIFO2 does not exist. Creating...\n");
        if (mkfifo(FIFO2, 0666) == -1) {
            perror("Failed to create FIFO2");
            exit(EXIT_FAILURE);
        }
    } else {
        dprintf(STDOUT_FILENO, "FIFO2 already exists.\n");
    }

    int num1 = atoi(argv[1]);
    int num2 = atoi(argv[2]);

    
    // 1️⃣ LOG dosyasını temizle (zaten doğru yerde)
    int clearfd = open(LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (clearfd != -1) {
        close(clearfd);
    }

    int tty_backup = dup(STDOUT_FILENO);  // terminal çıktı yedeği
    daemonize();

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGUSR1, signal_handler);

    time_t now = time(NULL);
    dprintf(STDOUT_FILENO, "Daemon started at %s", ctime(&now));
    fsync(STDOUT_FILENO);

    pid1 = fork();
    if (pid1 == 0) {
        sleep(10);
        dprintf(STDOUT_FILENO, "Child1 forked, executing ./child1...\n");
        fsync(STDOUT_FILENO);
        execlp("./child1", "./child1", NULL);
        dprintf(STDOUT_FILENO, "Child1 exec failed: %s\n", strerror(errno));
        fsync(STDOUT_FILENO);
        exit(EXIT_FAILURE);  // Ensure exit after exec
    }

        // 2️⃣ FIFO1'e sayıları yaz
        int writefd = open(FIFO1, O_WRONLY);
        if (writefd == -1) {
            perror("Failed to open FIFO1 for writing");
            exit(EXIT_FAILURE);
        }
        dprintf(writefd, "%d %d", num1, num2);
        close(writefd);
    
    pid2 = fork();
    if (pid2 == 0) {
        sleep(10);
        dprintf(STDOUT_FILENO, "Child2 forked, executing ./child2...\n");
        fsync(STDOUT_FILENO);
        execlp("./child2", "./child2", NULL);
        dprintf(STDOUT_FILENO, "Child2 exec failed: %s\n", strerror(errno));
        fsync(STDOUT_FILENO);
        exit(EXIT_FAILURE);  // Ensure exit after exec
    }

    int waited = 0;
    while (child_count < 4 && waited < 30) {
        sleep(2);
        waited += 2;
        dprintf(STDOUT_FILENO, "proceeding... waited %d seconds\n", waited);
        fsync(STDOUT_FILENO);
    }
    
    // Timeout’a ulaşıldı ama çocuklar bitmedi mi?
    if (child_count < 4) {
        dprintf(STDOUT_FILENO, "Timeout reached! Terminating child processes.\n");
        fsync(STDOUT_FILENO);
        if (pid1 > 0) {
            kill(pid1, SIGKILL);
            dprintf(STDOUT_FILENO, "Sent SIGKILL to child1 (PID %d)\n", pid1);
        }
        if (pid2 > 0) {
            kill(pid2, SIGKILL);
            dprintf(STDOUT_FILENO, "Sent SIGKILL to child2 (PID %d)\n", pid2);
        }
        fsync(STDOUT_FILENO);
    }
    
    // Add logging before unlinking to ensure FIFO existence
    if (access(FIFO1, F_OK) != -1) {
        dprintf(STDOUT_FILENO, "Unlinking FIFO1...\n");
        unlink(FIFO1);
    } else {
        dprintf(STDOUT_FILENO, "FIFO1 not found for unlinking.\n");
    }

    if (access(FIFO2, F_OK) != -1) {
        dprintf(STDOUT_FILENO, "Unlinking FIFO2...\n");
        unlink(FIFO2);
    } else {
        dprintf(STDOUT_FILENO, "FIFO2 not found for unlinking.\n");
    }
    if (child_count >= 4) {
        FILE *temp = fopen("/tmp/temp_result.txt", "r");
        if (temp) {
            if (fscanf(temp, "%d", &result) == 1) {
                dprintf(tty_backup, "The larger number is: %d\n", result);  // terminale geri yaz
            }
            fclose(temp);
        }
    }
    
    dprintf(STDOUT_FILENO, "Both child processes finished. Exiting daemon.\n");
    fsync(STDOUT_FILENO);

    return 0;
}
