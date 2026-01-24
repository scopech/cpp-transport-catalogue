#include "transport_catalogue.h"
#include <numeric>

namespace transport {

void TransportCatalogue::AddStop(std::string_view name, geo::Coordinates coordinates) {
    stops_.push_back({std::string(name), coordinates});
    const domain::Stop* stop_ptr = &stops_.back();
    
    stopname_to_stop_[stop_ptr->name] = stop_ptr;
    stop_to_buses_[stop_ptr]; 
}

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string_view>& stop_names) {
    domain::Bus bus;
    bus.name = std::string(name);
    
    for (const auto& stop_name : stop_names) {
        const auto stop_ptr = FindStop(stop_name);
        if (stop_ptr) {
            bus.stops.push_back(stop_ptr);
        }
    }

    buses_.push_back(std::move(bus));
    const domain::Bus* bus_ptr = &buses_.back();
    busname_to_bus_[bus_ptr->name] = bus_ptr;

    for (const auto* stop_ptr : bus_ptr->stops) {
        stop_to_buses_[stop_ptr].insert(bus_ptr->name);
    }
}

const domain::Bus* TransportCatalogue::FindBus(std::string_view name) const {
    if (const auto it = busname_to_bus_.find(name); it != busname_to_bus_.end()) {
        return it->second;
    }
    return nullptr;
}

const domain::Stop* TransportCatalogue::FindStop(std::string_view name) const {
    if (const auto it = stopname_to_stop_.find(name); it != stopname_to_stop_.end()) {
        return it->second;
    }
    return nullptr;
}

std::optional<domain::BusStat> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    const domain::Bus* bus = FindBus(bus_name);
    if (!bus) {
        return std::nullopt;
    }

    domain::BusStat stat;
    stat.stops_count = bus->stops.size();
    
    std::unordered_set<const domain::Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
    stat.unique_stops_count = unique_stops.size();

    double length = 0.0;
    for (size_t i = 1; i < bus->stops.size(); ++i) {
        length += geo::ComputeDistance(bus->stops[i - 1]->coordinates, bus->stops[i]->coordinates);
    }
    stat.route_length = length;

    return stat;
}

std::optional<domain::StopInfo> TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    const domain::Stop* stop_ptr = FindStop(stop_name);
    if (!stop_ptr) {
        return std::nullopt;
    }
    return domain::StopInfo{ &stop_to_buses_.at(stop_ptr) };
}

} // namespace transport
