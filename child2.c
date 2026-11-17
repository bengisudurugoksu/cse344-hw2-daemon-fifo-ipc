#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO2 "fifo2"

int main() {
    int larger_value;
    FILE *log_clear = fopen("child2.log", "w");
    if (log_clear) {
        fclose(log_clear); // Clear child2.log
    }

    // Log the start of child2 execution
    FILE *log = fopen("child2.log", "a");
    if (log) {
        fprintf(log, "child2: Starting execution.\n");
        fclose(log);
    }

    // Declare the buffer for reading data from FIFO2
    char buffer[100];  // Buffer to store data read from FIFO2

    // Open FIFO2 for reading
    int fd2 = open(FIFO2, O_RDONLY);  // Open FIFO2 for reading
    if (fd2 != -1) {
        printf("child2: Waiting to read from FIFO2...\n");  // Debug log
        ssize_t bytesRead = read(fd2, buffer, sizeof(buffer));  // Read the number from FIFO2
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            printf("child2: Reads from FIFO2: %s", buffer);  // Debug log

            // Log what we read from FIFO2
            FILE *log_value = fopen("child2.log", "a");
            if (log_value) {
                fprintf(log_value, "child2: Read from FIFO2: %s", buffer);
                fclose(log_value);
            }
        } else {
            printf("child2: Failed to read from FIFO2\n");  // Debug log
        }
        close(fd2);
    } else {
        printf("child2: Failed to open FIFO2 for reading\n");
    }

    // Convert the string to an integer
    larger_value = atoi(buffer);  // Convert the string to an integer

    // Print the larger number to the screen
    printf("child2: The larger number is %d\n", larger_value);

    // Log the larger number
    FILE *log_larger_value = fopen("child2.log", "a");
    if (log_larger_value) {
        fprintf(log_larger_value, "child2: The larger number is %d\n", larger_value);
        fclose(log_larger_value);
    }

    // Log the completion of child2
    FILE *log_finish = fopen("child2.log", "a");
    if (log_finish) {
        fprintf(log_finish, "child2: Execution finished.\n");
        fclose(log_finish);
    }

    // Exit successfully
    exit(EXIT_SUCCESS);
}
