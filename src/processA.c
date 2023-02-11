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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define DT 25 // Time in ms (40Hz)
#define PORT 4000

int finish = 0;

int spawn(const char * program, char * arg_list[]) {

  pid_t child_pid = fork();

  if(child_pid < 0) {
    perror("Error while forking...");
    return -1;
  }

  else if(child_pid != 0) {
    return child_pid;
  }

  else {
    if(execvp (program, arg_list) < 0) perror("Exec failed");
    return -1;
  }
}

int write_log(int fd_log, char *msg, int lmsg) {
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

int main(int argc, char *argv[]) {

    int mode = 1;
    int new_mode;
    int change_mode = 0;
    // Declare a socket server connection
    int server_fd, new_socket, port;
    char buffer[10] = {0};
    char ip[15];

    // Check the argv value for mode (1, 2, 3):
    if (argc != 4) {
        printf("Usage: ./processA [1|2|3] [port](0) [ip](0), use 0 in the indicated args to use default values.\n");
    }
    else {
        // Parse args
        mode = atoi(argv[1]);
        if (argv[2][0] == '0') port = PORT;
        else sscanf(argv[2], "%d", &port);
        sprintf(ip, "%s", argv[3]);
    }

    cycle:

    if (change_mode) {
        char aux[2];
        sprintf(aux, "%d", mode);
        char * args[] = { "/usr/bin/konsole", "-e", "./bin/processB", aux, NULL };
        spawn("/usr/bin/konsole", args);
        change_mode = 0;
    }

    // Wait for process B to initialize
    sleep(1);

    // Get the PID of process B:
    FILE *search = popen("pgrep processB", "r");
    char result[10];
    fgets(result, sizeof(result), search);
    pid_t pid_b;
    sscanf(result, "%d", &pid_b);

    // Log file:
    char logfile[30];
    sprintf(logfile, "%s%d%s", "./logs/processA", mode, ".txt");
    int fd_log = creat(logfile, 0666);
    if (fd_log < 0)
        perror("Error opening log file (A)");
    char log_msg[64];
    int length;

    // Write processB PID to log:
    length = snprintf(log_msg, 64, "PID process B: %d.\n", pid_b);
    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        perror("Error writing to log (A)");

    if (mode != 1) {

        // Declare the server and client address
        struct sockaddr_in server_address, client_address;
        // int opt = 1;
        int addrlen = sizeof(client_address);

        // Create the socket file descriptiors
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            perror("Socket failed");
            exit(EXIT_FAILURE);
        }
        bzero((char *)&server_address, sizeof(server_address));

        // Assign the IP, port
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);

        // Server
        if (mode == 2) {
            if (ip[0] != '0') server_address.sin_addr.s_addr = htonl(INADDR_ANY);
            else server_address.sin_addr.s_addr = inet_addr(ip);
            // Attach the port to the defined port
            if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
            {
                length = snprintf(log_msg, 64, "Bind failed: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
                exit(EXIT_FAILURE);
            }

            // Listen for connections
            if (listen(server_fd, 2) < 0)
            {
                length = snprintf(log_msg, 64, "Listen failed: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
                exit(EXIT_FAILURE);
            }

            // Accept the connection
            if ((new_socket = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&addrlen)) < 0)
            {
                length = snprintf(log_msg, 64, "Accept failed: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
                exit(EXIT_FAILURE);
            }

        // Client
        } else {
            server_address.sin_addr.s_addr = inet_addr(ip);

            // Connect to server
            if (connect(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
            {
                length = snprintf(log_msg, 64, "Connection failed: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
                exit(EXIT_FAILURE);
            }
        }
    }

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
    char shm_name[20];
    sprintf(shm_name, "%s%d", "/bmp_memory", mode);
    mvprintw(LINES - 3, 1, shm_name);
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
        close(fd_log);
        exit(1);
    }

    // Open semaphore:
    char sem_name[20];
    sprintf(sem_name, "%s%d", "/bmp_sem", mode);
    mvprintw(LINES - 2, 1, sem_name);
    sem_t *sem_id = sem_open(sem_name, O_CREAT | O_RDWR, 0666, 0);
    if (sem_id == SEM_FAILED)
    {
        length = snprintf(log_msg, 64, "Error opening semaphore: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        close(shm_fd);
        shm_unlink(shm_name);
        bmp_destroy(bmp);
        close(fd_log);
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
        close(fd_log);
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
        close(fd_log);
        exit(1);
    }

    // Infinite loop
    while (!finish)
    {
        // Get input in non-blocking mode
        int cmd, cmd_server = 0;
        if (mode == 2) {
            // Use a select to read from client in non-blocking mode

            // Variables for select()
            fd_set rfds;
            int retval;
            struct timeval tv = {0, 0};

            // Create the set of read fds:
            FD_ZERO(&rfds);
            FD_SET(new_socket, &rfds);

            retval = select(new_socket+1, &rfds, NULL, NULL, &tv);
            if (retval < 0 && errno != EINTR) {
                length = snprintf(log_msg, 64, "Error in select: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
            }
            else if (retval) {
                if (read(new_socket, buffer, sizeof(buffer)) < 0) {
                    length = snprintf(log_msg, 64, "Error reading from client: %d.\n", errno);
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing to log (A)");
                } else cmd = atoi(buffer);
            }
            cmd_server = getch();   
        } else {
            cmd = getch();
            mvprintw(LINES - 4, 1, "cc: %d", cmd);
        } 

        // If client, send command to server
        if (mode == 3) {
            sprintf(buffer, "%d", cmd);
            if (write(server_fd, buffer, sizeof(buffer)) < 0) {
                length = snprintf(log_msg, 64, "Error writing to server: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
            }
        }
        

        // If user resizes screen, re-draw UI...
        if (cmd == KEY_RESIZE || cmd_server == KEY_RESIZE)
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
        else if (cmd == KEY_MOUSE || cmd_server == KEY_MOUSE)
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
                else if (check_button_pressed(exit_btn, &event)) finish = 1;

                // Pressed normal button:
                else if (check_button_pressed(normal_btn, &event)) {
                    change_mode = 1;
                    new_mode = 1;
                    finish = 1;
                }

                // Pressed client button:
                else if (check_button_pressed(client_btn, &event)) {
                    change_mode = 1;
                    new_mode = 3;
                    finish = 1;
                }

                // Pressed server button:
                else if (check_button_pressed(server_btn, &event)) {
                    change_mode = 1;
                    new_mode = 2;
                    finish = 1;
                }
            }
        }

        // If input is an arrow key, move circle accordingly...
        else if (cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN)
        {
            // Move and draw circle:
            mvprintw(LINES - 5, 1, "pmc: %d", cmd);
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

    // Terminate process B
    kill(pid_b, SIGTERM);
    sem_post(sem_id); // Unlock sem_wait because we have not designed signal to work in multithread like semaphore
    sleep(1);

    // Termination:
    length = snprintf(log_msg, 64, "Exited successfully.");
    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        perror("Error writing to log (A)");
    bmp_destroy(bmp);
    close(shm_fd);
    shm_unlink(shm_name);
    sem_close(sem_id);
    sem_unlink(sem_name);
    close(fd_log);
    if (mode != 1){
        close(server_fd);
        if (mode == 2) close(new_socket);
    }
    reset_console_ui();
    endwin();

    //sleep(1);

    // Change mode routine
    if (change_mode) {
        finish = 0;
        if (new_mode != 1) {
            printf("Enter the port (write 0 to use default port):\n");
            if (scanf("%d", &port) < 0) perror("Error getting port");
            //fflush(stdin);
            printf("Enter the IP address of the server (if server you can write 0 to take any local ip):\n");
            scanf("%s", ip);
        }
        mode = new_mode;
        goto cycle;
    }

    return 0;
}
