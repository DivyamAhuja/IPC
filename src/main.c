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
#include <sys/prctl.h>
#include <signal.h>
#include <semaphore.h>


void sigchld_handler(int);
void* processFrames(void*);
int fd = 0;
gpu_manager* gpu;

void sigchld_handler(int sig) {
    
    pid_t pid;
    int status;
    pid = wait(NULL);
    printf("pid %d\n", pid);
    for(int i = 0; i < gpu->numCameras; i++) {
        if(gpu->cameras[i].pid == pid){
            printf("Camera %s died.\n", gpu->cameras[i].name);
            remove_camera_manager(gpu, &gpu->cameras[i]);
            break;
        }
    }
}

void sigterm_handler(int sig) {
    pid_t pid = getpid();
    char buf[30];
    sprintf(buf, "\n%d died.     %s\n", pid,
        sig == SIGINT ? "SIGINT" :
        sig == SIGTERM ? "SIGTERM" :
        "-"
    );
    printf(buf);
    fflush(stdout);
    //write(fd, buf, strlen(buf));
    for(int i = 0; i < gpu->numCameras; i++) {
        if(gpu->cameras[i].pid == pid){
            char buf[30];
            sprintf(buf, "Camera %s died.\n", gpu->cameras[i].name);
            printf(buf);
            fflush(stdout);
            //write(fd, buf, strlen(buf));
        }
    }
    _exit(0);
}
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
        //printf(textToWrite);
        write(fd, textToWrite, strlen(textToWrite));
        cam->frameRead++;
    }
    shmdt(mem);
    
    return NULL;
}


int main(int argc, char** argv) {
    signal(SIGCHLD, sigchld_handler);
    int outputFile = open("output.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
    fd = outputFile;
    printf("Program Started...\n");
    printf("It'll take time for Camera Managers to read batch of Frames... :D\n");
    fflush(stdout);
    gpu = create_gpu_manager();
    sem_init(&gpu->process_lock, 1, 1);
    //setsid();
    pid_t gpu_process_pid = getpid();
    printf("gpu proceess pid    %d\n", gpu_process_pid);
    pid_t pid = gpu_process_pid;
    int i;
    for (i = 0; i < 10; i++) {
        if (pid > 0) {
            //Parent Process
            //GPU Manager
            pid = fork();
        }
        if (pid == 0){
            //Child Process
            //Camera Manager
            //Or Maybe Error
            //printf("%d  ---  %d\n", getpid(), getppid());
            prctl(PR_SET_PDEATHSIG, SIGTERM);
            if (getppid() != gpu_process_pid){
                exit(1);
            }
            signal(SIGTERM, sigterm_handler);
            signal(SIGINT, sigterm_handler);
            break;
        }
    }
    if (pid == 0) {
        //Child Process
        //Camera Manager
        sem_wait(&gpu->process_lock);
        char name[20];
        sprintf(name, "camera%d", i);
        camera_manager cam = create_camera_manager(name);
        camera_manager* c = add_camera_manager(gpu, cam);
        if(c != NULL) {
            c->pid = getpid();
        }
        sem_post(&gpu->process_lock);
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
                pthread_create(&thread1,NULL, processFrames, (void*)cam);
                flag1 = 1;
            }
            
            if (gpu->numCameras > 1) {
                j = (i+1)%gpu->numCameras;
                cam2 = &gpu->cameras[j];
               
                while (!cam2->ready_to_be_encoded) {
                    j = (j + 1)%gpu->numCameras;
                    if (j == i) {
                        flag2 = 0;
                        break;
                    }
                    cam2 = &gpu->cameras[j];
                }
                if (j != i) {
                    pthread_create(&thread2,NULL, processFrames, (void*)cam2);
                    flag2 = 1;
                }
            }
            
            if(flag1 == 1) {
                pthread_join(thread1, NULL);
            }
            if (flag2 == 1){
                pthread_join(thread2, NULL);
            }
            i = (i + 1)%gpu->numCameras;
        }
        remove_gpu_manager(gpu);
        munmap(gpu, sizeof(gpu_manager));
        close(fd);
    }
    return 0;
}