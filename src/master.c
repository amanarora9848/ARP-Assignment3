#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

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

    // Open semaphore:
    char sem_name[] = "/bmp_sem";
    sem_t *sem_id = sem_open(sem_name, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem_id == SEM_FAILED)
    {
        perror("Error opening semaphore");
        exit(1);
    }

    // Initialize semaphore:
    if (sem_init(sem_id, 1, 0) < 0)
    {
        perror("Error initializing semaphore");
        sem_close(sem_id);
        sem_unlink(sem_name);
        exit(1);
    }

    // Initialization process:

    char ip[15], port[5];
    int choice;

    printf("Choose execution type (write number):\n");
    printf("1-Normal\n");
    printf("2-Server\n");
    printf("3-Client\n");
    scanf("%d", &choice);
    if (choice != 1) {
      printf("Enter the IP address:\n");
      scanf("%s", ip);
      printf("Enter the port:\n");
      scanf("%s", port);
    }
    char mode[2];
    sprintf(mode, "%d", choice);

    // Spawn processes:

    char * arg_list_A[] = { "/usr/bin/konsole", "-e", "./bin/processA", mode, ip, port, NULL };
    char * arg_list_B[] = { "/usr/bin/konsole", "-e", "./bin/processB", NULL };

    pid_t pid_procB = spawn("/usr/bin/konsole", arg_list_B);
    pid_t pid_procA = spawn("/usr/bin/konsole", arg_list_A);
    
    // Wait for children termination:
    int status;
    waitpid(pid_procA, &status, 0);
    waitpid(pid_procB, &status, 0);
  
    // Close semaphore:
    sem_close(sem_id);
    sem_unlink(sem_name);

    printf ("Main program exiting with status %d\n", status);
    return 0;
}

