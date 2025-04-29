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

    // 여기 수정함 🔥
    optional<float> val = get_state_speed(data);
    if (val.has_value()) {
        int new_speed = (int)val.value();
        if (this->preset_mode == "청정") {
            new_speed -= 0x20;  // 청정 preset일 때 0x20 빼기!
        }
        if (this->speed != new_speed) {
            this->speed = new_speed;
            changed = true;
        }
    }

    optional<std::string> preset = get_state_preset(data);
    if (preset.has_value() && this->preset_mode != preset.value())
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
    bool changed_state = false;
    bool changed_speed = false;
    bool changed_oscillating = false;
    bool changed_direction = false;
    bool changed_preset = false;

    if (call.get_state().has_value() && this->state != *call.get_state())
    {
        this->state = *call.get_state();
        changed_state = true;
    }
    if (call.get_oscillating().has_value() && this->oscillating != *call.get_oscillating())
    {
        this->oscillating = *call.get_oscillating();
        changed_oscillating = true;
    }
    if (call.get_speed().has_value() && this->speed != *call.get_speed())
    {
        this->speed = *call.get_speed();
        changed_speed = true;
    }
    if (call.get_direction().has_value() && this->direction != *call.get_direction())
    {
        this->direction = *call.get_direction();
        changed_direction = true;
    }
    if (call.get_preset_mode().size() > 0 && this->preset_mode != call.get_preset_mode())
    {
        this->preset_mode = call.get_preset_mode();
        changed_preset = true;
    }

    if (changed_preset) {
        enqueue_tx_cmd(get_command_preset(this->preset_mode));
        publish_state();  // ✅ preset 변경 후 상태 업데이트 추가
    }

    if (this->state && changed_state) enqueue_tx_cmd(get_command_on());
    if (changed_speed) enqueue_tx_cmd(get_command_speed(this->speed));
    if (!this->state && changed_state) enqueue_tx_cmd(get_command_off());

    publish_state();
}

}  // namespace uartex
}  // namespace esphome
