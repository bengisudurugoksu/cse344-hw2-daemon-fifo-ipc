#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define FIFO1 "fifo1"
#define FIFO2 "fifo2"

int main() {
    // Log dosyasını temizle
    FILE *log_clear = fopen("child1.log", "w");
    if (log_clear) {
        fclose(log_clear); // child1.log dosyasını temizle
    }

    // Log the start of execution
    FILE *log = fopen("child1.log", "a");
    if (log) {
        fprintf(log, "child1: Starting execution.\n");
        fflush(log);  // Ensure the buffer is written to the file
        fclose(log);
    }

    int num1, num2;

    // FIFO1'den sayıları oku
    int fd1 = open(FIFO1, O_RDONLY | O_NONBLOCK);
    if (fd1 == -1) {
        FILE *f = fopen("child1.log", "a");
        if (f) {
            fprintf(f, "child1: failed to open FIFO1\n");
            fflush(f);  // Ensure the buffer is written to the file
            fclose(f);
        }
        perror("child1: failed to open FIFO1");
        return 1;
    }

    FILE *log_open_fifo1 = fopen("child1.log", "a");
    if (log_open_fifo1) {
        fprintf(log_open_fifo1, "child1: FIFO1 opened successfully.\n");
        fflush(log_open_fifo1);  // Ensure the buffer is written to the file
        fclose(log_open_fifo1);
    }

    char buffer[100];
    ssize_t bytesRead;
    int retries = 10; // max 10 kez dene (10 saniye)
    while (retries-- > 0) {
        bytesRead = read(fd1, buffer, sizeof(buffer));
        if (bytesRead > 0){
            buffer[bytesRead] = '\0';
         break;
        }

        if (bytesRead == -1 && errno == EAGAIN) {
            // Veri yok, biraz bekleyip tekrar dene
            FILE *log_retry = fopen("child1.log", "a");
            if (log_retry) {
                fprintf(log_retry, "child1: FIFO1 is empty, retrying...\n");
                fclose(log_retry);
            }
            sleep(1);
        } else {
            // Gerçek bir hata oluştuysa logla ve çık
            perror("child1: failed to read from FIFO1");
            close(fd1);
            return 1;
        }
    }

    if (bytesRead == -1) {
        FILE *f = fopen("child1.log", "a");
        if (f) {
            fprintf(f, "child1: failed to read from FIFO1\n");
            fflush(f);  // Ensure the buffer is written to the file
            fclose(f);
        }
        perror("child1: failed to read from FIFO1");
        close(fd1);
        return 1;
    }

    FILE *log_read_fifo1 = fopen("child1.log", "a");
    if (log_read_fifo1) {
        fprintf(log_read_fifo1, "child1: Read from FIFO1: %s\n", buffer);
        fflush(log_read_fifo1);  // Ensure the buffer is written to the file
        fclose(log_read_fifo1);
    }

    sscanf(buffer, "%d %d", &num1, &num2);  // Sayıları ayıkla
    close(fd1);

    // Hangisi büyükse onu seç
    int bigger = (num1 > num2) ? num1 : num2;

    FILE *log_bigger_value = fopen("child1.log", "a");
    if (log_bigger_value) {
        fprintf(log_bigger_value, "child1: Larger value determined: %d\n", bigger);
        fflush(log_bigger_value);  // Ensure the buffer is written to the file
        fclose(log_bigger_value);
    }

    // FIFO2'ye büyük sayıyı yaz
// FIFO2'ye büyük sayıyı yaz
    int fd2 = open(FIFO2, O_WRONLY);  // Open FIFO2 for writing only
    if (fd2 != -1) {
        dprintf(fd2, "%d\n", bigger);  // <-- This writes the larger number to FIFO2
        close(fd2);

        // Log the action
        FILE *log_written_to_fifo2 = fopen("child1.log", "a");
        if (log_written_to_fifo2) {
            fprintf(log_written_to_fifo2, "child1: Written to FIFO2: %d\n", bigger);
            fflush(log_written_to_fifo2);  // Ensure the buffer is written to the file
            fclose(log_written_to_fifo2);
        }
    }

    FILE *temp = fopen("/tmp/temp_result.txt", "w");
    if (temp) {
        fprintf(temp, "%d\n", bigger);
        fclose(temp);
    } else {

        }
    
    FILE *log_open_fifo2 = fopen("child1.log", "a");
    if (log_open_fifo2) {
        fprintf(log_open_fifo2, "child1: FIFO2 opened successfully.\n");
        fflush(log_open_fifo2);  // Ensure the buffer is written to the file
        fclose(log_open_fifo2);
    }
    
    // Log yazalım (gerekirse)
    FILE *log_finish = fopen("child1.log", "a");
    if (log_finish) {
        fprintf(log_finish, "Child1: Read values %d and %d. Larger value: %d\n", num1, num2, bigger);
        fflush(log_finish);  // Ensure the buffer is written to the file
        fclose(log_finish);
    }

    // child1 tamamlandığında doğru bir şekilde çıkalım
    FILE *log_exit = fopen("child1.log", "a");
    if (log_exit) {
        fprintf(log_exit, "child1: Execution finished.\n");
        fflush(log_exit);  // Ensure the buffer is written to the file
        fclose(log_exit);
    }

    // Exit komutunu ekleyelim
    exit(EXIT_SUCCESS);
}
