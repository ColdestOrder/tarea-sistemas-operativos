# Gestor de Procesos (toadd & toad-cli)

**Tarea 1 - Sistemas Operativos**

Este proyecto consiste en un sistema cliente-servidor escrito en C para Linux. Su objetivo es ejecutar, monitorear y administrar el ciclo de vida de distintos procesos en segundo plano utilizando comunicación interprocesos mediante **Pipes (FIFOs)**

## 🛠️ Compilación

Para compilar el proyecto cumpliendo con los estándares exigidos, debes compilar ambos programas por separado. Abre tu terminal en la carpeta del proyecto y ejecuta estos dos comandos:

```bash
gcc -Wall -Wextra -std=c17 -o toadd toadd.c
gcc -Wall -Wextra -std=c17 -o toad-cli toad-cli.c
```
## Uso del Programa
**Iniciar el demonio**

Primero, debes levantar el proceso administrador. En tu terminal escribe:
```bash
./toadd
```
## Comandos del cliente

Una vez que el demonio está escuchando, abre una nueva ventana en tu terminal y usa la herramienta toad-cli para interactuar con él.

**Iniciar un nuevo proceso**

Solicita al demonio que ejecute un binario y lo deje bajo su administración.
```bash
./toad-cli start <ruta_al_binario>
```
**Detener un proceso**

Envía una señal (SIGTERM) para que el proceso termine amigablemente.
```bash
./toad-cli stop <iid>
```

**Listar procesos generales**

Muestra una tabla con todos los procesos administrados y su información básica.
```bash
./toad-cli ps
```
**Estado detallado**

Muestra información específica de un proceso en particular (PID, ruta, estado, uptime, etc.).
```bash
./toad-cli status <iid>
```
**Aniquilar un proceso**

Termina de forma inmediata e irrecuperable el proceso indicado y sus descendientes.
```bash
./toad-cli kill <iid>
```

**Radar de Zombies**

Lista únicamente los procesos que se encuentren en estado ZOMBIE (terminaron pero su estado no ha sido recogido por el demonio).
```bash
./toad-cli zombie
```
