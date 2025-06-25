#define DT_DRV_COMPAT zmk_input_processor_move_determin

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

LOG_MODULE_DECLARE(zmk,CONFIG_ZMK_LOG_LEVEL);

struct move_determin_config {
    uint16_t move_threshold;
    uint16_t window_ms;
};

struct move_determin_data {
    int64_t start_time;
    int32_t accumulated_movement;
    bool passed_threshold;
};

static int move_determin_handle_event(
    const struct device *dev, struct input_event *event, uint32_t param1,
    uint32_t param2, struct zmk_input_processor_state *state) {
    struct move_determin_data *data = dev->data;
    const struct move_determin_config *config = dev->config;
    
    if (event->type != INPUT_EV_REL || (event->code != INPUT_REL_X && event->code != INPUT_REL_Y)) {
        return ZMK_INPUT_PROC_CONTINUE; // Not a movement event
    }
    uint64_t current_time = k_uptime_get();
    
    if (data->start_time == 0 || (current_time - data->start_time) > config->window_ms) {
        data->start_time = current_time; // Initialize start time
        data->accumulated_movement = 0; // Reset accumulated movement
        data->passed_threshold = false; // Reset threshold flag
    }
    
    if (event->value < 0) {
        data->accumulated_movement -= event->value;
    } else {
        data->accumulated_movement += event->value;
    } 
    if (!data->passed_threshold && data->accumulated_movement >= config->move_threshold) {
        data->passed_threshold = true; // Movement exceeds threshold
    }
    if (!data->passed_threshold) {
        return ZMK_INPUT_PROC_STOP; // Still below threshold
    }
    
    return ZMK_INPUT_PROC_CONTINUE; // Movement exceeds threshold, continue processing
}

static int move_determin_init(const struct device *dev) {
    struct move_determin_data *data = dev->data;
    const struct move_determin_config *config = dev->config;

    data->start_time = 0;
    data->accumulated_movement = 0;
    data->passed_threshold = false;

    if (config->move_threshold == 0 || config->window_ms == 0) {
        LOG_ERR("Invalid configuration: move_threshold and window_ms must be greater than zero.");
        return -EINVAL;
    }

    return 0;
}

static struct zmk_input_processor_driver_api move_determin_driver_api = {
    .handle_event = move_determin_handle_event,
};

#define MOVE_DETERMIN_INST(inst) \
    static struct move_determin_data move_determin_data_##inst; \
    static const struct move_determin_config move_determin_config_##inst = { \
        .move_threshold = DT_INST_PROP(inst, move_threshold), \
        .window_ms = DT_INST_PROP(inst, window_ms), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, &move_determin_init, NULL, \
                                          &move_determin_data_##inst, \
                                          &move_determin_config_##inst, \
                                          POST_KERNEL, CONFIG_INPUT_PROCESSOR_INIT_PRIORITY, \
                                          &move_determin_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MOVE_DETERMIN_INST)
