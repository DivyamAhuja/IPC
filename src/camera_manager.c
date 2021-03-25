#include "camera_manager.h"
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

camera_manager create_camera_manager(const char* name){
    key_t key = ftok(name, 69);
    // MEMORY_SIZE FOR FRAMES, + 1 BYTE FOR EVENT
    int shmid = shmget(key, MEMORY_SIZE + 1, 0666 | IPC_CREAT);
    char* mem = (char*) shmat(shmid, (void*)0, 0);
    camera_manager cam;
    cam.id = shmid;
    cam.pid = 0;
    strcpy(cam.name, name);
    cam.mem = (void*)mem;
    cam.queueStart = 0;
    cam.queueEnd = 1;
    cam.camera_fd = open("/dev/urandom", O_RDONLY);
    cam.ready_to_be_encoded = 0;
    cam.frameRead = 0;
    return cam;
}

int read_from_camera(camera_manager* cam) {
    if (
          (cam->queueEnd > cam->queueStart && cam->queueEnd - cam->queueStart <= MEMORY_SIZE - FRAME_SIZE)
        ||(cam->queueStart > cam->queueEnd && cam->queueStart - cam->queueEnd >= FRAME_SIZE)
       )
    {
        int size_read = read(cam->camera_fd, cam->mem + (cam->queueEnd - 1)%MEMORY_SIZE, FRAME_SIZE);
        if (size_read == FRAME_SIZE) {
            cam->queueEnd += FRAME_SIZE;
            cam->ready_to_be_encoded = (
                cam->queueEnd > cam->queueStart && cam->queueEnd - cam->queueStart >= 60*FRAME_SIZE
             || cam->queueStart > cam->queueEnd && cam->queueStart - cam->queueEnd <= 40*FRAME_SIZE
            );
            return 0;
        }
    }
    return -1;
}