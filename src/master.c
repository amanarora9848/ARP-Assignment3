#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

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

int main() {

    // Initialization process:

    char ip[15] = {}, port[10] = {};
    int choice;
    int flag_mode = 1;

    printf("Choose execution type (write number):\n");
    printf("1-Normal\n");
    printf("2-Server\n");
    printf("3-Client\n");
    while (flag_mode) {
      if (scanf("%d", &choice) > 0 && (choice == 1 || choice == 2 || choice == 3)) {
        flag_mode = 0;
      } else printf("Wrong input. Please try again.");
    }
    if (choice != 1) {
      printf("Enter the port (write 0 to use default port):\n");
      scanf("%s", port);
      printf("Enter the IP address of the server (if server you can write 0 to take any local ip):\n");
      scanf("%s", ip);
    }
    char mode[2];
    sprintf(mode, "%d", choice);

    // Open semaphore:
    char sem_name[20];
    sprintf(sem_name, "%s%s", "/bmp_sem", mode);
    sem_t *sem_id = sem_open(sem_name, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem_id == SEM_FAILED)
    {
        perror("Error opening semaphore");
        exit(1);
    }

    // Spawn processes:

    char * arg_list_A[] = { "/usr/bin/konsole", "-e", "./bin/processA", mode, port, ip, NULL };
    char * arg_list_B[] = { "/usr/bin/konsole", "-e", "./bin/processB", mode, NULL };

    pid_t pid_procB = spawn("/usr/bin/konsole", arg_list_B);
    pid_t pid_procA = spawn("/usr/bin/konsole", arg_list_A);
    
    // Wait for children termination:
    int status;
    waitpid(pid_procA, &status, 0);

    printf ("Main program exiting with status %d\n", status);
    return 0;
}

