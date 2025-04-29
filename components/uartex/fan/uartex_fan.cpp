#include "uartex_fan.h"

namespace esphome {
namespace uartex {

static const char *TAG = "uartex.fan";

void UARTExFan::dump_config()
{
#ifdef ESPHOME_LOG_HAS_DEBUG
    log_config(TAG, "Name", get_name().c_str());
    log_config(TAG, "State Speed", get_state_num("state_speed"));
    uartex_dump_config(TAG);
#endif
}

fan::FanTraits UARTExFan::get_traits()
{
    fan::FanTraits traits{};
    if (this->speed_count_ > 0)
    {
        traits.set_speed(true);
        traits.set_supported_speed_count(this->speed_count_);
    }
    if (!this->preset_modes_.empty()) traits.set_supported_preset_modes(this->preset_modes_);
    return traits;
}

void UARTExFan::publish(const std::vector<uint8_t>& data)
{
    bool changed = false;
    optional<float> val = get_state_speed(data);
    if (val.has_value() && this->speed != (int)val.value())
    {
        this->speed = (int)val.value();
        changed = true;
    }
    optional<std::string> preset = get_state_preset(data);
    if(preset.has_value() && this->preset_mode != preset.value())
    {
        this->preset_mode = preset.value();
        changed = true;
    }
    if (changed) publish_state();
}

void UARTExFan::publish(const bool state)
{
    if (state == this->state) return;
    this->state = state; 
    this->publish_state();
}

void UARTExFan::control(const fan::FanCall& call)
{
    if (call.get_state().has_value() && this->state != *call.get_state())
    {
        bool state = *call.get_state();
        if (state)
        {
            if (enqueue_tx_cmd(get_command_on()))
            {
                this->state = state;
            }
        }
    }
    if (call.get_speed().has_value() && this->speed != *call.get_speed())
    {
        int speed = *call.get_speed();
        if (enqueue_tx_cmd(get_command_speed(speed)))
        {
            this->speed = speed;
        }
    }
    if (call.get_state().has_value() && this->state != *call.get_state())
    {
        bool state = *call.get_state();
        if (!state)
        {
            if (enqueue_tx_cmd(get_command_off()))
            {
                this->state = state;
            }
        }
    }
    if (call.get_oscillating().has_value() && this->oscillating != *call.get_oscillating())
    {
        // bool oscillating = *call.get_oscillating();
        // {
        //     this->oscillating = oscillating;
        // }
    }
    if (call.get_direction().has_value() && this->direction != *call.get_direction())
    {
        // fan::FanDirection direction = *call.get_direction();
        // {
        //     this->direction = direction;
        // }
    }
    if (call.get_preset_mode().size() > 0 && this->preset_mode != call.get_preset_mode())
    {
        std::string preset_mode = call.get_preset_mode();
        if (enqueue_tx_cmd(get_command_preset(preset_mode)))
        {
            this->preset_mode = preset_mode;
        }
    }
    publish_state();
}

}  // namespace uartex
}  // namespace esphome
