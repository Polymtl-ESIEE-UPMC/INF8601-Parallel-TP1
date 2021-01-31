#include <tbb/pipeline.h>

extern "C" {

#include "filter.h"
#include "log.h"
#include "pipeline.h"

image_dir_t* image_dir;

class Thread1 {
   public:
    image_t* operator()(tbb::flow_control& fc) const {
        image_t* image = image_dir_load_next(image_dir);
        if (image == NULL) {
            fc.stop();
            return NULL;
        }
        return image;
    }
};

class Thread2 {
   public:
    image_t* operator()(image_t* image) const {
        image_t* scaled_image = filter_scale_up(image, 2);
        image_destroy(image);
        return scaled_image;
    }
};

class Thread3 {
   public:
    image_t* operator()(image_t* scaled_image) const {
        image_t* hsv_image = filter_to_hsv(scaled_image);
        image_destroy(scaled_image);
        return hsv_image;
    }
};

class Thread4 {
   private:
    mutable pixel_t pixel = {.bytes = {0, 0, 0, 0}};

   public:
    image_t* operator()(image_t* hsv_image) const {
        pixel.bytes[0] += 4;
        image_t* add_image = filter_add_pixel(hsv_image, &pixel);
        image_destroy(hsv_image);
        return add_image;
    }
};

class Thread5 {
   public:
    image_t* operator()(image_t* add_image) const {
        image_t* rgb_image = filter_to_rgb(add_image);
        image_destroy(add_image);
        return rgb_image;
    }
};

class Thread6 {
   public:
    void operator()(image_t* rgb_image) const {
        image_dir_save(image_dir, rgb_image);
        image_destroy(rgb_image);
    }
};

int pipeline_tbb(image_dir_t* img_dir) {
    image_dir = img_dir;
    tbb::parallel_pipeline(6, tbb::make_filter<void, image_t*>(tbb::filter::serial_in_order, Thread1()) &
                                  tbb::make_filter<image_t*, image_t*>(tbb::filter::parallel, Thread2()) &
                                  tbb::make_filter<image_t*, image_t*>(tbb::filter::parallel, Thread3()) &
                                  tbb::make_filter<image_t*, image_t*>(tbb::filter::serial_in_order, Thread4()) &
                                  tbb::make_filter<image_t*, image_t*>(tbb::filter::parallel, Thread5()) &
                                  tbb::make_filter<image_t*, void>(tbb::filter::parallel, Thread6()));
    return 0;
}

} /* extern "C" */
