#pragma once

#include "geo.h"
#include <string>
#include <string_view>
#include <vector>
#include <set>

namespace transport {
namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip;
};

struct BusStat {
    size_t stops_count;
    size_t unique_stops_count;
    double route_length;
    double curvature;
};

struct StopInfo {
    const std::set<std::string_view>* buses;
};

} // namespace domain
} // namespace transport
