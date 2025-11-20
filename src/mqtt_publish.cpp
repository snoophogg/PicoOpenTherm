#include "mqtt_publish.hpp"
#include "mqtt_common.hpp"
#include <unordered_map>
#include <string>
#include <cstdio>
#include "pico/time.h"

namespace OpenTherm
{
    namespace Publish
    {
        static std::unordered_map<std::string, std::string> g_last_published;
        static uint32_t last_cache_clear = 0;
        constexpr uint32_t CACHE_CLEAR_INTERVAL_MS = 86400000; // 24 hours

        // Check if cache needs clearing (called periodically from publish functions)
        static void checkCacheClear()
        {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            if (now - last_cache_clear >= CACHE_CLEAR_INTERVAL_MS)
            {
                printf("Clearing last published cache (%zu entries)\n", g_last_published.size());
                g_last_published.clear();
                last_cache_clear = now;
            }
        }

        static void formatFloat(char *buf, size_t len, float value, int precision)
        {
            char fmt[8];
            snprintf(fmt, sizeof(fmt), "%%.%df", precision);
            snprintf(buf, len, fmt, value);
        }

        bool publishFloatIfChanged(const std::string &topic, float value, int precision, bool retain)
        {
            checkCacheClear();
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
            checkCacheClear();
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
            checkCacheClear();
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
            checkCacheClear();
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
