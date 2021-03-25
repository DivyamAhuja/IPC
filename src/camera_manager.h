#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#define MEMORY_SIZE 102400000 // 1024 x 768 x 50 = 78643200 bytes; enough for 100 frames
#define FRAME_SIZE 1024000
typedef struct
{
    int id;
    int pid;
    char name[20];
    void* mem;
    unsigned long queueStart;
    unsigned long queueEnd;
    int camera_fd;
    int ready_to_be_encoded;
    unsigned long frameRead;
} camera_manager;


camera_manager create_camera_manager();
int read_from_camera(camera_manager*);

#endif