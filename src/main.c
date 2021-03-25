#include <stdio.h>
#include "camera_manager.h"
#include "gpu_manager.h"
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>

void* processFrames(void* x);

int fd = 0;

void* processFrames(void* c) {
    camera_manager* cam = (camera_manager*)c;
    int shmid = cam->id;
    char* mem = (char*) shmat(shmid, (void*)0, 0);
    unsigned long long i, j, checksum;
    for(i = 0; i < 60; i++) {
        checksum = 0;
        for(j = 0; j < FRAME_SIZE; j++){
            checksum += (unsigned long long)*(mem + cam->queueStart + j);
        }
        cam->queueStart = (cam->queueStart + FRAME_SIZE)%MEMORY_SIZE;
        char textToWrite[100];
        sprintf(textToWrite, "camera number: %s  frame number: %d  checksum: %llu\n", (cam->name + 6), cam->frameRead, checksum);
        printf(textToWrite);
        write(fd, textToWrite, strlen(textToWrite));
        cam->frameRead++;
    }
    shmdt(mem);
    
    return NULL;
}


int main(int argc, char** argv) {
    int outputFile = open("output.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
    fd = outputFile;
    printf("Program Started...\n");
    printf("It'll take time for Camera Managers to read batch of Frames... :D\n");
    fflush(stdout);
    gpu_manager* gpu = create_gpu_manager();
    pid_t gpu_process_pid = getpid();
    pid_t pid = gpu_process_pid;
    int i;
    for (i = 0; i < 10; i++) {
        if (pid != 0) {
            //Parent Process
            //GPU Manager
            pid = fork();
            /*if (gpu->numCameras > 0){
                gpu->cameras[gpu->numCameras]->pid = pid;
            }*/
        }
        else {
            //Child Process
            //Camera Manager
            //Or Maybe Error
            break;
        }
    }
    if (pid == 0) {
        //Child Process
        //Camera Manager
        char name[20];
        sprintf(name, "camera%d", i);

        camera_manager cam = create_camera_manager(name);
        camera_manager* c = add_camera_manager(gpu, cam);
        while (1)
        {
            read_from_camera(c);
        }   
    } else if (pid > 0)
    {
        pthread_t thread1, thread2;
        int i = 0, j = 0;
        while (1) {
            if(gpu->numCameras == 0)
                continue;
            
            camera_manager* cam = &gpu->cameras[i];
            camera_manager* cam2;

            int flag1 = 0, flag2 = 0;
            if(cam->ready_to_be_encoded){
                //pthread_create(&thread1,NULL, processFrames, (void*)cam);
                int p = fork();
                if (p == 0){
                    processFrames(cam);
                    return 0;
                }
                flag1 = 1;
            }
            
            if (gpu->numCameras > 1) {
                j = (i+1)%gpu->numCameras;
                cam2 = &gpu->cameras[j];
                while (!cam2->ready_to_be_encoded) {
                    j += (j + 1)%gpu->numCameras;
                    if (j == i) {
                        flag2 = 0;
                        break;
                    }
                    cam2 = &gpu->cameras[j];
                }
                if (j != i) {
                    //pthread_create(&thread2,NULL, processFrames, (void*)cam2);
                    int p = fork();
                    if (p == 0){
                        processFrames(cam2);
                        return 0;
                    }
                    flag2 = 1;
                }
            }
            
            if(flag1 == 1)
                wait(NULL);
                //pthread_join(thread1, NULL);
            
            if (flag2 == 1){
                wait(NULL);
                //pthread_join(thread2, NULL);
            }
            i = (i + 1)%gpu->numCameras;
        }
    }
    munmap(gpu, sizeof(gpu_manager));
    close(fd);
    return 0;
}