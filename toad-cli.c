#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common.h"

void send_and_print(Message *msg){
int resp_fd = open (FIFO_RESP, O_RDWR);

int cmd_fd = open(FIFO_CMD, O_WRONLY);
//se abre el tubo de ida fifo_cmd pero solo en modo de escritura o_wronly
if (cmd_fd < 0) {
    fprintf(stderr, "Error: toadd no se esta ejecutando\n");
    close(resp_fd);
    exit(1);
//se hace un if para el caso de que se devuelva un numero menor a 0, esto indicaria un error de que el daemon toadd no se ha conseguido ejecutr y se cierra
}
write(cmd_fd, msg, sizeof(Message));
close(cmd_fd);
//se toma el mensaje y se introduce en el tubo, luego se usa close para que cerrar el tubo al terminar de usarlo
char buf[256];
//el buffer actuara como una memoria temporal que depositara informacion en un kugar fisico de la memoria ram
ssize_t n;
//es el contador de cuantos datos se sacaron del buffer
while ((n = read(resp_fd, buf, sizeof(buf)-1))>0){
buf[n] = '\0';
//la funcion read hacer 3 cosas ve el tubo de respuesta saca la cantidad de letras y las mete en el buf 
//dejando un espacio libre al final, la variable n toma el valor de cuantas letras se saquen
//despues se agrega el caracter nulo para que el sistema sepa donde terminar la plaabra
if (strstr (buf, "END\n")){
char *end = strstr(buf, "END\n");
*end = '\0';
printf ("%s", buf);
break;
}
//usaremos la funcion strstr que busca una palabra dentro de una palabra mas grande
//la instruccion del if dice que busque si dentro del buf hay una palabra end\n que nos dice donde termina el mensaje
//depues se usa strstr de nuevo para crear un puntero a end\n despues con ese puntero se reescribira con el caracter nulo
//al final solo imprimimos en la pantalla el contenido del buf y usamosa break para cerrar por la fuerza el while
printf ("%s", buf);
//se imprime lo que se haya encontrado en el buf en el caso de que no se encontrara la palabra end'n
if (buf[n-1]=='\n') break;
//esta linea seran para cuando en el buf hay respuestas cortas
if (msg->type != CMD_PS && msg->type != CMD_ZOMBIE) {
    break;
  }

}
close (resp_fd);
//se cierra el tubo de lectura
}


int main (int argc, char *argv[]){
//argc es una funcion que cuenta cuantas palabras escribio el usuaerio en la terminal
//argv es un arreglo que guarda las palabras que el usuario escribio
if (argc < 2){
//detendremos al usuario en caso de que solo haya escrito menos de dos palabras, esto signfica que algo olvido
fprintf(stderr, "¡VAYA VAYA! Te faltó decirme qué comando quieres ejecutar.\n");
fprintf(stderr, "Ejemplo de uso: ./toad-cli start /home/mi_programa_bello\n");
fprintf(stderr, "Los comandos que entiendo son: start, stop, ps, status, kill o zombie.\n");
//fprintf(stderr) imprime un mensaje en la pantalla pero por el canal de errores y cerraremos el programa indicando al sistyema que hubo un error
return 1;
}
Message msg = {0};
//creamos una variables llamada msg del tipo message el {0} es una forma de decirle al sistema que llene los compartimiento con ceros para no tener informacion remanente de ejecucuciones anteriores
if (strcmp (argv[1], "start")==0){
//strcmp compara texto, en este caso compara lo que el usuario escribiop en argv[1] con la palabra start si son oiguales devuelve 0, en esta funcion 0 significa que son iguales
if (argc < 3) { fprintf(stderr, "¡AJA! Te faltó la ruta del programa. Ejemplo: ./toad-cli start /home/mi_programa\n"); return 1; }
//en este if se le avisa al usuario que debe entregar la ruta del progrsma que quiere ejecutar
msg.type = CMD_START;
//aqui guardaremos en type del msg el codigo cmd_start que se entiende como la orden de iniciar un proceso
strncpy(msg.path, argv[2], 255);
//strncpy es la funcion para copiar textos, esta linea le dice al programa que tome lo escrito por el usuario en argv[2] lo copie dentro del compartimeinto msg.path pero con un maximo de 255 caracteres para que podamos evitar un ruta muy grande
}
else if (strcmp(argv[1], "stop") == 0) {
if (argc < 3) { fprintf(stderr, "¡ALto! Necesito saber qué número de proceso (IID) quieres detener. Ejemplo: ./toad-cli stop 2\n"); return 1; }
msg.type = CMD_STOP;
//aqui se guardara el codigo cmd_stop que se entinede como que el usuario quiere detener tal proceso
msg.iid = atoi(argv[2]);
//atoi es uan funcion que toma una palabra que escribio el usuario y la transforma en un numero entero
//cuando atoi hace la conversion ese numero se guarda en msg.iid (cuando el usuario escribe 3 en la terminal no se toma como un numero entero sino como un simbolo de texto por eso debemos hacer la conversion)
}
else if (strcmp(argv[1], "ps") == 0) {
msg.type = CMD_PS;
//el codigo cmd_ps  es la orden de solicitar la tabal con todos los procesos
}
else if (strcmp(argv[1], "status") == 0) {
if (argc < 3) { fprintf(stderr, "¡STOP! Para darte el estado detallado, necesito saber de qué proceso hablamos. Ejemplo: ./toad-cli status 5\n"); return 1; }
msg.type = CMD_STATUS;
//el codigo cmd_status es la orden de mostrar la informacion de un porceso en especifico
msg.iid = atoi(argv[2]);
//toma el texto que dio el usuario y lo convertimos en un numero entero
}
else if (strcmp(argv[1], "kill") == 0) {
if (argc < 3) { fprintf(stderr, "¡Tranquilo pequeñin! Tienes que decirme a qué proceso (IID) quieres aniquilar. Ejemplo: ./toad-cli kill 3\n"); return 1; }
msg.type = CMD_KILL;
//el codigo cmd_kill es la orden de matar un proceso
msg.iid = atoi(argv[2]);
}
else if (strcmp(argv[1], "zombie") == 0) {
msg.type = CMD_ZOMBIE;
//el cmd_zombie es la orden para solicitar la lista de procesos zombie
}
else {
fprintf(stderr, "¿Quien eres tu? No te conozco este comando desconocido: %s\n", argv[1]);
//el %s es para mostrar en pantalla lo que se halla escrito en argv[1]
return 1;
}
send_and_print(&msg);
//se llama a la funcion send_and_print pero el msg se adjunta con un & que es para no enviar una copia de msg sino su coordenada y ser mas eficiente con los recursos
return 0;
}


