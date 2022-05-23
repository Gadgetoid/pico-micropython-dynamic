#include "module.hpp"

extern "C" {
    #include "hardware/gpio.h"
    #include "py/dynruntime.h"

    mp_obj_t test(mp_obj_t x_obj) {
        int result = gpio_get(1);
        return mp_obj_new_int(result);
    }
}