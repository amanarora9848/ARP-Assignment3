#define _XOPEN_SOURCE 700
#include "./../include/processB_utilities.h"
#include <bmpfile.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include "./../include/bmp_functions.h"
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

int finish = 0;

int write_log(int fd_log, char *msg, int lmsg)
{
    char log_msg[64];
    time_t now = time(NULL);
    struct tm *timenow = localtime(&now);
    int length = strftime(log_msg, 64, "[%H:%M:%S]: ", timenow);
    if (write(fd_log, log_msg, length) < 0 && errno != EINTR)
        return -1;
    if (write(fd_log, msg, lmsg) < 0 && errno != EINTR)
        return -1;
    return 0;
}

void handler_exit(int sig)
{
    finish = 1;
}

int main(int argc, char const *argv[])
{
    // Log file:
    char logfile[30];
    sprintf(logfile, "%s%s%s", "./logs/processB", argv[1], ".txt");
    int fd_log = creat(logfile, 0666);
    if (fd_log < 0) {
        endwin();
        perror("Error opening log file (B)");
        sleep(3);
    }
    char log_msg[64];
    int length;

    // Signal handling to exit process:
    struct sigaction sa_exit;
    sigemptyset(&sa_exit.sa_mask);
    sa_exit.sa_handler = &handler_exit;
    // sa_exit.sa_flags = SA_RESTART;
    sa_exit.sa_flags = 0;
    if (sigaction(SIGTERM, &sa_exit, NULL) < 0)
    {
        length = snprintf(log_msg, 64, "Cannot catch SIGTERM.\n");
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            endwin();
            perror("Error writing to log (cmd)");
            sleep(3);
    }

    sleep(1);

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();
    printw(getcwd(NULL, 0));
    
    const size_t shm_size = WIDTH * HEIGHT * sizeof(rgb_pixel_t);

    // Open shared memory:
    char shm_name[20];
    sprintf(shm_name, "%s%s", "/bmp_memory", argv[1]);
    mvprintw(LINES - 3, 1, shm_name);
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0 && errno != EINTR)
    {
        length = snprintf(log_msg, 64, "Error opening shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            endwin();
            perror("Error writing to log (B)");
            sleep(3);
        close(fd_log);
        exit(1);
    }

    // Open semaphore:
    char sem_name[20];
    sprintf(sem_name, "%s%s", "/bmp_sem", argv[1]);
    mvprintw(LINES - 2, 1, sem_name);
    sem_t *sem_id = sem_open(sem_name, O_CREAT | O_RDWR, 0666, 0);
    if (sem_id == SEM_FAILED)
    {
        length = snprintf(log_msg, 64, "Error opening semaphore: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (B)");
        close(shm_fd);
        close(fd_log);
        exit(1);
    }

    // Map shared memory:
    rgb_pixel_t *ptr = (rgb_pixel_t *)mmap(0, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        length = snprintf(log_msg, 64, "Error mapping shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (B)");
        close(shm_fd);
        sem_close(sem_id);
        close(fd_log);
        exit(1);
    }

    int center_pos[2];
    
    // Wait for semaphore:
    if (sem_wait(sem_id) < 0 && errno != EINTR)
    {
        length = snprintf(log_msg, 64, "Error waiting semaphore: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (B)");
        finish = 1;
    }
    
    // Get center of the circle:
    find_circle_center(ptr, center_pos);
    length = snprintf(log_msg, 64, "Center of circle found at (%d, %d).\n", center_pos[0], center_pos[1]);
    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        perror("Error writing to log (B)");

    // Draw circle center:
    mvaddch(center_pos[1] / SCALE, center_pos[0] / SCALE, '0');

    // Infinite loop
    while (!finish)
    {

        // Get input in non-blocking mode
        int cmd = getch();

        // If user resizes screen, re-draw UI...
        if (cmd == KEY_RESIZE)
        {
            if (first_resize)
            {
                first_resize = FALSE;
            }
            else
            {
                reset_console_ui();
            }
        }

        else
        {
            // Wait for semaphore:
            if (sem_wait(sem_id) < 0 && errno != EINTR)
            {
                length = snprintf(log_msg, 64, "Error waiting semaphore: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (B)");
                finish = 1;
            }

            // Get center of the circle:
            find_circle_center(ptr, center_pos);
            length = snprintf(log_msg, 64, "Center of circle found at (%d, %d).\n", center_pos[0], center_pos[1]);
            if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                perror("Error writing to log (B)");

            // Draw circle center
            mvaddch(center_pos[1] / SCALE, center_pos[0] / SCALE, '0');

            refresh();
        }
    }

    // Termination:
    length = snprintf(log_msg, 64, "Exited successfully.");
    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        perror("Error writing to log (B)");
    close(shm_fd);
    sem_close(sem_id);
    close(fd_log);
    endwin();
    return 0;
}
