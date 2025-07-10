#pragma once

#include <Geode/loader/Event.hpp>

class GroupViewUpdateEvent final : public geode::Event {
public:
    GroupViewUpdateEvent() : geode::Event() {}
};