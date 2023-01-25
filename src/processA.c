#include "./../include/processA_utilities.h"
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

#define DT 25 // Time in ms (40Hz)

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

int main(int argc, char *argv[])
{
    // Some time for process B to initialize:
    sleep(1);
    
    // Get the PID of process B:
    FILE *cmd = popen("pgrep processB", "r");
    char result[10];
    fgets(result, sizeof(result), cmd);
    pid_t pid_b;
    sscanf(result, "%d", &pid_b);

    // Log file:
    int fd_log = creat("./logs/processA.txt", 0666);
    if (fd_log < 0)
        perror("Error opening log file (A)");
    char log_msg[64];
    int length;

    // Write processB PID to log:
    // length = snprintf(log_msg, 64, "PID process B: %d.\n", pid_b);
    // if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
    //     perror("Error writing to log (A)");

    // Time structure for the sleep:
    const struct timespec delay_nano = {
        .tv_sec = 0,
        .tv_nsec = DT * 1e6}; // 25ms

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    // Declare circle center variables:
    int circle_x;
    int circle_y;

    // Create bitmap structure:
    bmpfile_t *bmp;
    const size_t shm_size = WIDTH * HEIGHT * sizeof(rgb_pixel_t);
    bmp = bmp_create(WIDTH, HEIGHT, 4);
    if (bmp == NULL)
    {
        length = snprintf(log_msg, 64, "Error creating bitmap: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        exit(1);
    }
    
    // Draw initial state:
    circle_x = get_circle_x() * SCALE;
    circle_y = get_circle_y() * SCALE;
    draw_bmp(bmp, circle_x, circle_y);
    length = snprintf(log_msg, 64, "Center of circle drawn at (%d, %d).\n", circle_x, circle_y);
    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        perror("Error writing to log (A)");

    // Create shared memory:
    char shm_name[] = "/bmp_memory";
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0)
    {
        length = snprintf(log_msg, 64, "Error opening shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        bmp_destroy(bmp);
        exit(1);
    }

    // Set the size of the shm:
    if (ftruncate(shm_fd, shm_size) < 0)
    {
        length = snprintf(log_msg, 64, "Error truncating shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        close(shm_fd);
        shm_unlink(shm_name);
        bmp_destroy(bmp);
        exit(1);
    }

    // Open semaphore:
    char sem_name[] = "/bmp_sem";
    sem_t *sem_id = sem_open(sem_name, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem_id == SEM_FAILED)
    {
        length = snprintf(log_msg, 64, "Error opening semaphore: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        close(shm_fd);
        shm_unlink(shm_name);
        bmp_destroy(bmp);
        exit(1);
    }

    // Map the shm:
    rgb_pixel_t *ptr = (rgb_pixel_t *)mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        length = snprintf(log_msg, 64, "Error mapping shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        bmp_destroy(bmp);
        close(shm_fd);
        shm_unlink(shm_name);
        sem_close(sem_id);
        exit(1);
    }

    // Copy bitmap to shm:
    save_bmp(bmp, ptr);

    // Tell the process B that writing is done:
    if (sem_post(sem_id) < 0)
    {
        length = snprintf(log_msg, 64, "Error posting to semaphore: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        bmp_destroy(bmp);
        close(shm_fd);
        shm_unlink(shm_name);
        sem_close(sem_id);
        exit(1);
    }

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

        // Else, if user presses button...
        else if (cmd == KEY_MOUSE)
        {
            if (getmouse(&event) == OK)
            {
                // Pressed print button:
                if (check_button_pressed(print_btn, &event))
                {
                    // Save snapshot:
                    bmp_save(bmp, "./out/snapshot.bmp");

                    mvprintw(LINES - 1, 1, "Print button pressed");
                    refresh();
                    sleep(1);
                    for (int j = 0; j < COLS - BTN_SIZE_X - 2; j++)
                    {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }
                
                // Pressed exit button:
                else if (check_button_pressed(exit_btn, &event))
                {
                    finish = 1;
                    kill(pid_b, SIGTERM);
                }
            }
        }

        // If input is an arrow key, move circle accordingly...
        else if (cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN)
        {
            // Move and draw circle:
            move_circle(cmd);
            draw_circle();
            
            // Destroy bitmap:
            bmp_destroy(bmp);
            
            // Create new bitmap:
            bmp = bmp_create(WIDTH, HEIGHT, 4);
            if (bmp == NULL)
            {
                length = snprintf(log_msg, 64, "Error creating bitmap: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
                finish = 1;
            }
            
            // Draw new bitmap:
            circle_x = get_circle_x() * SCALE;
            circle_y = get_circle_y() * SCALE;
            draw_bmp(bmp, circle_x, circle_y);

            // Log the center
            length = snprintf(log_msg, 64, "Center of circle drawn at (%d, %d).\n", circle_x, circle_y);
            if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                perror("Error writing to log (A)");

            // Copy bitmap to shm:
            save_bmp(bmp, ptr);

            // Tell the process B that writing is done:
            if (sem_post(sem_id) < 0)
            {
                length = snprintf(log_msg, 64, "Error posting to semaphore: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
                finish = 1;
            }
        }
        
        nanosleep(&delay_nano, NULL);
    }

    // Termination:
    length = snprintf(log_msg, 64, "Exited successfully.");
    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        perror("Error writing to log (A)");
    bmp_destroy(bmp);
    close(shm_fd);
    sem_close(sem_id);
    close(fd_log);
    endwin();
    return 0;
}
