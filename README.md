# IPC (Inter-process Communication)
A simple C linux IPC program which very badly tries to implement Mock Queuing and Processing of incoming(never ending) camera data by GPU/software. :D


## Compiling and Running
Just run `make` in **linux** shell to compile.
or alternatively
```bash
gcc -g -lpthread -o main src/main.c src/gpu_manager.c src/camera_manager.c
```
### To Run
```bash
./main
```

## WARNINGS
This Program has very bad process management as process run infinite loop and Main gpu_manager process doesn't provide nice ui or interface for creating/adding and rdeleting/removing camera_manager processes

So watch out for your processes ':(
<br>
(Some functions are still untested too ':"(  )

## Working

The main(gpu_manager_process) forks into multiple child(camera_manager_process) and stores information about them in shared memory created from mmap.

The camera_manager_process infinitely reads from /dev/urandom and stores frames(chunks of 1280x800 bytes) into it's shared memory.

The gpu_manager_process checks if some camera_manager has enough frames, ready to be encoded and then creates 2 childprocess for handling camera_manager_process (simulating 2 encode sessions of GPU)

![Working of Project](./working.png "Working of Project")
