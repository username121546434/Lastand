#pragma once
#ifndef __CONSTANTS_H__

#define __CONSTANTS_H__

constexpr double tick_rate_ms {1.0 / 120 * 1000};

constexpr short channel_events {0};
constexpr short channel_updates {1};
constexpr short channel_user_updates {2};
constexpr short num_channels = 3;
constexpr long time_for_game_to_start_ms {10'000};

constexpr int window_size {600};

#endif // __CONSTANTS_H__
