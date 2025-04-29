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
    if (val.has_value())
    {
        int raw_speed = (int)val.value();
        int mapped_speed;

        // 🔥 수신값 매핑: 패킷 값 ➔ 내부 fan speed (1~4)
        if (raw_speed == 0x21)
            mapped_speed = 1;  // 청정 미
        else if (raw_speed == 0x22)
            mapped_speed = 2;  // 청정 약
        else if (raw_speed == 0x23)
            mapped_speed = 3;  // 청정 강
        else if (raw_speed == 0x03)
            mapped_speed = 4;  // 환기 강
        else
            mapped_speed = 1;  // fallback

        if (this->speed != mapped_speed)
        {
            this->speed = mapped_speed;
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
        uint8_t packet_speed;

        // 🔥 보낼 때 매핑: 내부 fan speed ➔ 패킷에 넣을 값
        if (speed == 1)
            packet_speed = 0x21;  // 청정 미
        else if (speed == 2)
            packet_speed = 0x22;  // 청정 약
        else if (speed == 3)
            packet_speed = 0x23;  // 청정 강
        else if (speed == 4)
            packet_speed = 0x03;  // 환기 강
        else
            packet_speed = 0x21;  // fallback

        if (enqueue_tx_cmd(get_command_speed(packet_speed)))
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
        // 오실레이팅 제어 생략
    }

    if (call.get_direction().has_value() && this->direction != *call.get_direction())
    {
        // 방향 제어 생략
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
