#include "transport_catalogue.h"
#include <numeric>

namespace transport {

void TransportCatalogue::AddStop(std::string_view name, geo::Coordinates coordinates) {
    stops_.push_back({std::string(name), coordinates});
    const domain::Stop* stop_ptr = &stops_.back();
    
    stopname_to_stop_[stop_ptr->name] = stop_ptr;
    stop_to_buses_[stop_ptr]; 
}

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string_view>& stop_names, bool is_roundtrip) {
    domain::Bus bus;
    bus.name = std::string(name);
    bus.is_roundtrip = is_roundtrip;
    bus.stops.reserve(stop_names.size());
    
    for (const auto& stop_name : stop_names) {
        if (const auto stop_ptr = FindStop(stop_name)) {
            bus.stops.push_back(stop_ptr);
        }
    }

    if (!is_roundtrip && bus.stops.size() > 1) {
        for (int i = static_cast<int>(bus.stops.size()) - 2; i >= 0; --i) {
            bus.stops.push_back(bus.stops[i]);
        }
    }

    buses_.push_back(std::move(bus));
    const domain::Bus* bus_ptr = &buses_.back();
    busname_to_bus_[bus_ptr->name] = bus_ptr;

    for (const auto* stop_ptr : bus_ptr->stops) {
        stop_to_buses_[stop_ptr].insert(bus_ptr->name);
    }
}

void TransportCatalogue::SetDistance(const domain::Stop* from, const domain::Stop* to, double distance) {
    distances_[{from, to}] = distance;
}

double TransportCatalogue::GetDistance(const domain::Stop* from, const domain::Stop* to) const {
    if (auto it = distances_.find({from, to}); it != distances_.end()) {
        return it->second;
    }
    if (auto it = distances_.find({to, from}); it != distances_.end()) {
        return it->second;
    }
    return geo::ComputeDistance(from->coordinates, to->coordinates);
}

const domain::Bus* TransportCatalogue::FindBus(std::string_view name) const {
    if (auto it = busname_to_bus_.find(name); it != busname_to_bus_.end()) {
        return it->second;
    }
    return nullptr;
}

const domain::Stop* TransportCatalogue::FindStop(std::string_view name) const {
    if (auto it = stopname_to_stop_.find(name); it != stopname_to_stop_.end()) {
        return it->second;
    }
    return nullptr;
}

std::optional<domain::BusStat> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    const domain::Bus* bus = FindBus(bus_name);
    if (!bus || bus->stops.empty()) {
        return std::nullopt;
    }

    domain::BusStat stat{};
    stat.stops_count = bus->stops.size();
    
    std::unordered_set<const domain::Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
    stat.unique_stops_count = unique_stops.size();

    double road_length = 0.0;
    double geo_length = 0.0;
    for (size_t i = 1; i < bus->stops.size(); ++i) {
        road_length += GetDistance(bus->stops[i - 1], bus->stops[i]);
        geo_length += geo::ComputeDistance(bus->stops[i - 1]->coordinates, bus->stops[i]->coordinates);
    }
    stat.route_length = road_length;
    stat.curvature = (geo_length > 0) ? road_length / geo_length : 0.0;

    return stat;
}

std::optional<domain::StopInfo> TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    const domain::Stop* stop_ptr = FindStop(stop_name);
    if (!stop_ptr) {
        return std::nullopt;
    }
    auto it = stop_to_buses_.find(stop_ptr);
    if (it == stop_to_buses_.end()) {
        static const std::set<std::string_view> empty_set;
        return domain::StopInfo{ &empty_set };
    }
    return domain::StopInfo{ &it->second };
}

std::map<std::string_view, const domain::Bus*> TransportCatalogue::GetSortedAllBuses() const {
    std::map<std::string_view, const domain::Bus*> result;
    for (const auto& [name, bus_ptr] : busname_to_bus_) {
        result[name] = bus_ptr;
    }
    return result;
}

} // namespace transport
