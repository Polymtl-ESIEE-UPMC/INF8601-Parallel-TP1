#include <pthread.h>

#include "filter.h"
#include "log.h"
#include "pipeline.h"
#include "queue.h"

image_dir_t* img_dir;
queue_t* q12;
queue_t* q23;
queue_t* q34;
queue_t* q45;
queue_t* q56;

void* thread1(void* inutilise) {
    while (1) {
        image_t* image = image_dir_load_next(img_dir);
        queue_push(q12, image);
        if (image == NULL) {
            break;
        }
    }
    pthread_exit(NULL);
}

void* thread2(void* inutilise) {
    while (1) {
        void* image = queue_pop(q12);
        if (image == NULL) {
            queue_push(q23, NULL);
            break;
        }
        image_t* scaled_image = filter_scale_up(image, 2);
        image_destroy(image);
        queue_push(q23, scaled_image);
    }
    pthread_exit(NULL);
}

void* thread3(void* inutilise) {
    while (1) {
        void* scaled_image = queue_pop(q23);
        if (scaled_image == NULL) {
            queue_push(q34, NULL);
            break;
        }
        image_t* hsv_image = filter_to_hsv(scaled_image);
        image_destroy(scaled_image);
        queue_push(q34, hsv_image);
    }
    pthread_exit(NULL);
}

void* thread4(void* inutilise) {
    pixel_t pixel = {.bytes = {0, 0, 0, 0}};
    while (1) {
        void* hsv_image = queue_pop(q34);
        if (hsv_image == NULL) {
            queue_push(q45, NULL);
            break;
        }
        pixel.bytes[0] += 4;
        image_t* add_image = filter_add_pixel(hsv_image, &pixel);
        image_destroy(hsv_image);
        queue_push(q45, add_image);
    }
    pthread_exit(NULL);
}

void* thread5(void* inutilise) {
    while (1) {
        void* add_image = queue_pop(q45);
        if (add_image == NULL) {
            queue_push(q56, NULL);
            break;
        }
        image_t* rgb_image = filter_to_rgb(add_image);
        image_destroy(add_image);
        queue_push(q56, rgb_image);
    }
    pthread_exit(NULL);
}

void* thread6(void* inutilise) {
    while (1) {
        void* rgb_image = queue_pop(q56);
        if (rgb_image == NULL) {
            break;
        }
        image_dir_save(img_dir, rgb_image);
        image_destroy(rgb_image);
    }
    pthread_exit(NULL);
}

int pipeline_pthread(image_dir_t* im_dir) {
    img_dir = im_dir;

    int queue_size = 999;

    q12 = queue_create(queue_size);
    q23 = queue_create(queue_size);
    q34 = queue_create(queue_size);
    q45 = queue_create(queue_size);
    q56 = queue_create(queue_size);

    pthread_t th1, th2, th3, th4, th5, th6;
    pthread_create(&th1, NULL, thread1, NULL);
    pthread_create(&th2, NULL, thread2, NULL);
    pthread_create(&th3, NULL, thread3, NULL);
    pthread_create(&th4, NULL, thread4, NULL);
    pthread_create(&th5, NULL, thread5, NULL);
    pthread_create(&th6, NULL, thread6, NULL);
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    pthread_join(th3, NULL);
    pthread_join(th4, NULL);
    pthread_join(th5, NULL);
    pthread_join(th6, NULL);

    return 0;
}
