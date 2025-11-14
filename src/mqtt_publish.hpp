#ifndef MQTT_PUBLISH_HPP
#define MQTT_PUBLISH_HPP

#include <string>

namespace OpenTherm
{
    namespace Publish
    {
        bool publishFloatIfChanged(const std::string &topic, float value, int precision = 2, bool retain = false);
        bool publishIntIfChanged(const std::string &topic, int value, bool retain = false);
        bool publishStringIfChanged(const std::string &topic, const std::string &value, bool retain = false);
        bool publishBinaryIfChanged(const std::string &topic, bool value, bool retain = false);
    }
}

#endif // MQTT_PUBLISH_HPP
