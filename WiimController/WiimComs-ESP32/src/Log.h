#pragma once

#ifdef ESP_PLATFORM
  #include "esp_log.h"
#else
  #include <cstdio>

  #define WIIM_LOG_IMPL(stream, level, tag, ...) \
      do { \
          std::fprintf((stream), "%s (%s) ", (level), (tag)); \
          std::fprintf((stream), __VA_ARGS__); \
          std::fprintf((stream), "\n"); \
      } while (0)

  #define ESP_LOGI(tag, ...) WIIM_LOG_IMPL(stdout, "I", tag, __VA_ARGS__)
  #define ESP_LOGW(tag, ...) WIIM_LOG_IMPL(stdout, "W", tag, __VA_ARGS__)
  #define ESP_LOGE(tag, ...) WIIM_LOG_IMPL(stderr, "E", tag, __VA_ARGS__)
  #define ESP_LOGD(tag, ...) WIIM_LOG_IMPL(stdout, "D", tag, __VA_ARGS__)
  #define ESP_LOGV(tag, ...) WIIM_LOG_IMPL(stdout, "V", tag, __VA_ARGS__)
#endif
