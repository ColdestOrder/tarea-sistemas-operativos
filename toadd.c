#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include "common.h"

//definiciones glovales 

ManagedProcess procs[MAX_PROCESSES];
int proc_count = 0;
int next_iid = 2;

//MP es un arreglo que almacena la info de hasta 64 procesos, proc_count es el contador de procesos registrados y next iid es para que el primer hijo tenga ID 2, el 1 se reserva para el daemon

//se convierte al programa en daemon

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);  //aqui se termina el proceso padre

    setsid(); //daemonizamos creando un nuevo grupo independiente del terminal

//creamos un segundo fork para evitar la re adquirir terminal

   pid = fork();
   if (pid < 0) exit(1);
   if (pid > 0) exit(0); //el primer hijo muere dejando al nieto

//hacemos el "black hole" del sistema, cualquier intento de lectura devuelve EOF y al escribir en ellos lo escrito desaparece, puff magia

    int devnull = open("/dev/null", O_RDWR);
    dup2(devnull, STDIN_FILENO);
    dup2(devnull, STDOUT_FILENO);
    dup2(devnull, STDERR_FILENO);
    close(devnull);
}


// creamos una funcion para formatear tiempo

void format_uptime(time_t start, char *buf, size_t len) {
    time_t elapsed = time(NULL) - start;
    int h = elapsed / 3600;
    int m = (elapsed % 3600) / 60;
    int s = elapsed % 60;
    snprintf(buf, len, "%02d:%02d:%02d", h, m, s); //escribimos el resultado en el buffer
}

//funcion para el nombre de estado

const char* state_name(ProcessState s) {
     switch(s) {
           case STATE_RUNNING: return "RUNNING";
           case STATE_STOPPED: return "STOPPED";
           case STATE_ZOMBIE:  return "ZOMBIE";
           case STATE_FAILED:  return "FAILED";
           default:            return "UNKNOWN";
          }
}

//creamos una funcion para chequear por hijos muertis para limpiar la tabla de procesos
//wnohang  no bloquea si no tenemos hijos muertos

void check_children(){
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0){
        for (int i = 0; i < proc_count; i++){
            if(procs[i].pid == pid){
                if (procs[i].stopped_by_user) {
                    procs[i].state = STATE_STOPPED;
                } 
                else {  
                    procs[i].restarts++;
                    if(procs[i].restarts > MAX_RESTARTS){
                        procs[i].state = STATE_FAILED;
                    }
                    else {
                        pid_t new_pid = fork();
                        if (new_pid == 0){
                            // --- ESTO ES LO QUE CAMBIA ---
                            setsid(); // Independencia de grupo
                            int dn = open("/dev/null", O_RDWR);
                            dup2(dn, STDIN_FILENO);
                            dup2(dn, STDOUT_FILENO);
                            dup2(dn, STDERR_FILENO);
                            close(dn);
                            // ----------------------------

                            char *args[] = {procs[i].binary, NULL};
                            execv(procs[i].binary, args);
                            exit(1);
                        }
                        procs[i].pid = new_pid;
                        procs[i].state = STATE_RUNNING;
                        procs[i].start_time = time(NULL);
                    }
                }
                break;
            }
        }
    }
}
//funcion para matar proceso y todos sus hijos

void kill_tree(pid_t pid){
//matamos al grupo completo de procesos
    killpg(getpgid(pid), SIGKILL);
    kill(pid, SIGKILL);
}


// enviamos la respuesta a toad cli
void send_response(int fd, const char *msg) {
    write(fd, msg, strlen(msg));
}

// se maneja el comando que se recibe
void handle_command(Message *msg, int resp_fd) {
   char buf[4096] = {0};

    switch (msg->type) {

    case CMD_START: {
      // SE CREA el nuevo proceso hijo
       pid_t pid = fork();
       if (pid == 0) {
         // el hijo ejecuta el binario
         setsid(); // nuevo grupo para matar a los hijos
         char *args[] = { msg->path, NULL };
         execv(msg->path, args);
         exit(1); // si execv falla
       }
       //el padre registra el proceso
       int idx = proc_count++;
       procs[idx].iid = next_iid++;
       procs[idx].pid = pid;
       strncpy(procs[idx].binary, msg->path, 255);
       procs[idx].state = STATE_RUNNING;
       procs[idx].start_time = time(NULL);
       procs[idx].restarts = 0;
       procs[idx].stopped_by_user = 0;
       snprintf(buf, sizeof(buf), "IID: %d\n", procs[idx].iid);
       send_response(resp_fd, buf);
       break;
    }

    case CMD_STOP: {
       int found = 0;
       for (int i = 0; i < proc_count; i++) {
         if (procs[i].iid == msg->iid) {
             procs[i].stopped_by_user = 1;
             kill(procs[i].pid, SIGTERM);
             send_response(resp_fd, "OK\n");
             found = 1;
             break; 
          }
       }
       if (!found) send_response(resp_fd, "ERROR: IID no encontrado\n");
       break;
    }

    case CMD_PS: {
       snprintf(buf, sizeof(buf), "%-6s %-8s %-10s %-12s %s\n","IID", "PID", "STATE", "UPTIME", "BINARY");
       send_response(resp_fd, buf);
       for (int i = 0; i < proc_count; i++) {
         char uptime[16];
         format_uptime(procs[i].start_time, uptime, sizeof(uptime));
         snprintf(buf, sizeof(buf), "%-6d %-8d %-10s %-12s %s\n", procs[i].iid, procs[i].pid, state_name(procs[i].state), uptime, procs[i].binary);
         send_response(resp_fd, buf);
       }
       send_response(resp_fd, "END\n");
       break;
    }

    case CMD_STATUS: {
      int found = 0;
      for (int i = 0; i < proc_count; i++) {
        if (procs[i].iid == msg->iid) {
          char uptime[16];
          format_uptime(procs[i].start_time, uptime, sizeof(uptime));
          snprintf(buf, sizeof(buf),
            "IID: %d\nPID: %d\nBINARY: %s\n"
            "STATE: %s\nUPTIME: %s\nRESTARTS: %d\n",
          procs[i].iid, procs[i].pid,
          procs[i].binary,
          state_name(procs[i].state),
          uptime, procs[i].restarts);
          send_response(resp_fd, buf);
          found = 1;
          break;
        }
      }
      if (!found) send_response(resp_fd, "ERROR: IID no encontrado\n");
      break;
    }

    case CMD_KILL: {
      int found = 0;
      for (int i = 0; i < proc_count; i++) {
        if (procs[i].iid == msg->iid) {
           procs[i].stopped_by_user = 1;
           kill_tree(procs[i].pid);
           procs[i].state = STATE_STOPPED;
           send_response(resp_fd, "OK\n");
           found = 1;
           break;
        }
      }
      if (!found) send_response(resp_fd, "ERROR: IID no encontrado\n");
      break;
    }

    case CMD_ZOMBIE: {
       int found_z = 0;
       for (int i = 0; i < proc_count; i++) {
         if (procs[i].state == STATE_ZOMBIE) {
           snprintf(buf, sizeof(buf),
           "IID: %d  PID: %d  BINARY: %s\n",
            procs[i].iid, procs[i].pid, procs[i].binary);
            send_response(resp_fd, buf);
            found_z = 1;
         }
       }
       if (!found_z) send_response(resp_fd, "No hay procesos zombie\n");
       send_response(resp_fd, "END\n");
       break;
    }
  }
}

// creamos el int main, con el loop principal
int main() {
    daemonize();
// se crean los FIFO en caso de que no existan
    mkfifo(FIFO_CMD,  0666);
    mkfifo(FIFO_RESP, 0666);

int cmd_fd = open(FIFO_CMD, O_RDONLY | O_NONBLOCK);
int resp_fd = open(FIFO_RESP, O_RDWR);  //CON EStas lineas evitamos bloquear las pipelines
// loop para leer comandos y responderlos
while (1) {
    Message msg;
    ssize_t n = read(cmd_fd, &msg, sizeof(Message));

    if (n == sizeof(Message)) {
        handle_command(&msg, resp_fd);
    }

    // revisamos por si algun hijo murio
    // esta funcion siempre se ejecuta limpiando zombies y huerfanos
    check_children();

    // respiro al procesador
   usleep(100000); 
}
  return 0;
}
