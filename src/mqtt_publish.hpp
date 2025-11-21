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
        void clearAllCaches(); // Clear cache to force republish of all values on next update
        void republishAllCached(); // Republish all currently cached values without reading from boiler
    }
}

#endif // MQTT_PUBLISH_HPP
