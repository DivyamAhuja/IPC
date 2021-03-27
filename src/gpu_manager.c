#include "gpu_manager.h"
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

gpu_manager* create_gpu_manager() {
    gpu_manager* gpu = (gpu_manager*)mmap(NULL,sizeof(gpu_manager),PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_SHARED,-1,0);
    gpu->numCameras = 0;
    return gpu;
}

camera_manager* add_camera_manager(gpu_manager* gpu,camera_manager cam) {
    if (gpu->numCameras < 512) {
        gpu->cameras[gpu->numCameras] = cam;
        gpu->numCameras++;
        return &gpu->cameras[gpu->numCameras - 1];
    }
    return NULL;
}

int remove_camera_manager(gpu_manager* gpu, camera_manager* cam) {
    if (gpu->numCameras > 0) {
        camera_manager* temp;
        
        for(int i=0; i < gpu->numCameras; i++) {
            temp = &gpu->cameras[i];
            if (temp != cam)
                continue;
            else {
                kill(cam->pid, SIGKILL);
                shmctl(cam->id, IPC_RMID, NULL);
                *temp = gpu->cameras[gpu->numCameras - 1];
                gpu->numCameras--;
                return 0;
            }
        }
        
    }
    return -1;
}

int remove_gpu_manager(gpu_manager* gpu) {
    for (int i = gpu->numCameras - 1; i >= 0; i++) {
        remove_camera_manager(gpu, &gpu->cameras[i]);
    }
    free(gpu);
}
