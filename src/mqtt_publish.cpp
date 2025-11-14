#include "mqtt_publish.hpp"
#include "mqtt_common.hpp"
#include <unordered_map>
#include <string>
#include <cstdio>

namespace OpenTherm
{
    namespace Publish
    {
        static std::unordered_map<std::string, std::string> g_last_published;

        static void formatFloat(char *buf, size_t len, float value, int precision)
        {
            char fmt[8];
            snprintf(fmt, sizeof(fmt), "%%.%df", precision);
            snprintf(buf, len, fmt, value);
        }

        bool publishFloatIfChanged(const std::string &topic, float value, int precision, bool retain)
        {
            char payload[32];
            formatFloat(payload, sizeof(payload), value, precision);
            auto it = g_last_published.find(topic);
            if (it != g_last_published.end() && it->second == payload)
                return true; // nothing to do
            if (OpenTherm::Common::mqtt_publish_wrapper(topic.c_str(), payload, retain))
            {
                g_last_published[topic] = payload;
                return true;
            }
            return false;
        }

        bool publishIntIfChanged(const std::string &topic, int value, bool retain)
        {
            char payload[32];
            snprintf(payload, sizeof(payload), "%d", value);
            auto it = g_last_published.find(topic);
            if (it != g_last_published.end() && it->second == payload)
                return true;
            if (OpenTherm::Common::mqtt_publish_wrapper(topic.c_str(), payload, retain))
            {
                g_last_published[topic] = payload;
                return true;
            }
            return false;
        }

        bool publishStringIfChanged(const std::string &topic, const std::string &value, bool retain)
        {
            auto it = g_last_published.find(topic);
            if (it != g_last_published.end() && it->second == value)
                return true;
            if (OpenTherm::Common::mqtt_publish_wrapper(topic.c_str(), value.c_str(), retain))
            {
                g_last_published[topic] = value;
                return true;
            }
            return false;
        }

        bool publishBinaryIfChanged(const std::string &topic, bool value, bool retain)
        {
            const char *payload = value ? "ON" : "OFF";
            auto it = g_last_published.find(topic);
            if (it != g_last_published.end() && it->second == payload)
                return true;
            if (OpenTherm::Common::mqtt_publish_wrapper(topic.c_str(), payload, retain))
            {
                g_last_published[topic] = payload;
                return true;
            }
            return false;
        }
    }
}
