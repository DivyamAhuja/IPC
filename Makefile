all:
	gcc -g -lpthread -o main src/main.c src/gpu_manager.c src/camera_manager.c