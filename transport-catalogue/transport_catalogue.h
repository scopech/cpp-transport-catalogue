#pragma once

#include <deque>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string_view>
#include <optional>
#include <utility>

#include "geo.h"

namespace transport {

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
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

struct PairHasher {
    size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& pair) const {
        auto h1 = std::hash<const void*>{}(pair.first);
        auto h2 = std::hash<const void*>{}(pair.second);
        return h1 ^ (h2 << 1);
    }
};

class TransportCatalogue {
public:
    void AddStop(std::string_view name, geo::Coordinates coordinates);
    void AddBus(std::string_view name, const std::vector<std::string_view>& stop_names);
    
    void SetDistance(const domain::Stop* from, const domain::Stop* to, double distance);
    double GetDistance(const domain::Stop* from, const domain::Stop* to) const;

    const domain::Bus* FindBus(std::string_view name) const;
    const domain::Stop* FindStop(std::string_view name) const;

    std::optional<domain::BusStat> GetBusInfo(std::string_view bus_name) const;
    
    std::optional<domain::StopInfo> GetStopInfo(std::string_view stop_name) const;

private:
    std::deque<domain::Stop> stops_;
    std::deque<domain::Bus> buses_;

    std::unordered_map<std::string_view, const domain::Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, const domain::Bus*> busname_to_bus_;

    std::unordered_map<const domain::Stop*, std::set<std::string_view>> stop_to_buses_;
    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, double, PairHasher> distances_;
};

} // namespace transport
