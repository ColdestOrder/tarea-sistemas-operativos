#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <time.h>

//definimos los pipes FIFOs
//CMD es donde el cliente escribe las instrucciones y resp donde el servidor responde al cliente

#define FIFO_CMD "/tmp/toadd_cmd.pipe"
#define FIFO_RESP "/tmp/toadd_resp.pipe"

// definimos los nombres para los numeros de comandos requeridos (0 = start, 1 = stop, ps, status, kill y zombie)

typedef enum {
    CMD_START,
    CMD_STOP,
    CMD_PS,
    CMD_STATUS,
    CMD_KILL,
    CMD_ZOMBIE
} CommandType;

//mensajes que viajarán por el pipe, lo que toad-cli escribe toadd lo lee
// iid es para stop, status y kill, path para start
typedef struct {
    CommandType type;
    int iid;
    char path[256];
} Message;

// estados del proceso, definimos en que estado puede estar un proceso administrado por el daemon

typedef enum {
    STATE_RUNNING,
    STATE_STOPPED,
    STATE_ZOMBIE,
    STATE_FAILED,  //ESTE ESTADO ES PARA EL BONUS
} ProcessState;

// esta es la estructura de control, todo lo necesario para que el daemon recuerde cada proceso que "vigila"

typedef struct {
    int iid;
    pid_t pid;  //ste es el id asignado por el SO, a diferencia de iid que es asignado a partir del 2 (1 asignado a toadd)
    char binary[256];
    ProcessState state;
    time_t start_time;
    int restarts;  //esto es para el bonus, identifica cuantas veces se reinicio solo
    int stopped_by_user; // esto es para saber si el proceso fue detenido o pausado por el usuario.
} ManagedProcess;

#define MAX_PROCESSES 64
#define MAX_RESTARTS 5  // esto es para el bonus

#endif
