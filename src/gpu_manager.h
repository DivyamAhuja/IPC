#ifndef GPU_MANAGER_H
#define GPU_MANAGER_H

#include "camera_manager.h"
#include <sys/types.h>
#include <semaphore.h>

typedef struct
{
    int numCameras;
    camera_manager cameras[512];
    sem_t process_lock;
} gpu_manager;



gpu_manager* create_gpu_manager();
camera_manager* add_camera_manager(gpu_manager* , camera_manager );
int remove_camera_manager(gpu_manager* , camera_manager* );
int remove_gpu_manager(gpu_manager* );

#endif