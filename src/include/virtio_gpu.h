#ifndef VIRTIO_GPU_H
#define VIRTIO_GPU_H

#include "types.h"

bool virtio_gpu_init(uint16_t width, uint16_t height, uint32_t *backbuffer);
void virtio_gpu_present(void);
void virtio_gpu_shutdown(void);

#endif