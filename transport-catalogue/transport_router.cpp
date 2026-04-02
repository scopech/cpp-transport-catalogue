#include "transport_router.h"

namespace transport {

TransportRouter::TransportRouter(const TransportCatalogue& catalogue, const RoutingSettings& settings)
    : catalogue_(catalogue), settings_(settings) {
    BuildGraph();
}

void TransportRouter::BuildGraph() {
    const auto& all_stops = catalogue_.GetSortedAllStops();
    size_t vertex_count = all_stops.size() * 2;
    graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(vertex_count);

    graph::VertexId v = 0;
    for (const auto& [stop_name, stop_ptr] : all_stops) {
        stop_to_wait_vertex_[stop_name] = v;
        stop_to_board_vertex_[stop_name] = v + 1;
        
        graph_->AddEdge({
            v, 
            v + 1, 
            static_cast<double>(settings_.bus_wait_time)
        });

        edge_info_.push_back({"Wait", std::string(stop_name), 0, static_cast<double>(settings_.bus_wait_time)});
        
        v += 2;
    }

    const auto& all_buses = catalogue_.GetSortedAllBuses();
    for (const auto& [bus_name, bus_ptr] : all_buses) {
        const auto& stops = bus_ptr->stops;
        size_t n = stops.size();
        for (size_t i = 0; i < n; ++i) {
            double distance = 0.0;
            for (size_t j = i + 1; j < n; ++j) {
                if (!bus_ptr->is_roundtrip) {
                    size_t terminus = stops.size() / 2;
                    if (i < terminus && j > terminus) {
                        break;
                    }
                }
                
                distance += catalogue_.GetDistance(stops[j-1], stops[j]);
                double time = distance / (settings_.bus_velocity * 1000.0 / 60.0);
                
                graph_->AddEdge({
                    stop_to_board_vertex_.at(stops[i]->name),
                    stop_to_wait_vertex_.at(stops[j]->name),
                    time
                });

                edge_info_.push_back({"Bus", std::string(bus_name), static_cast<int>(j - i), time});
            }
        }
    }

    router_ = std::make_unique<graph::Router<double>>(*graph_);
}

std::optional<RouteInfo> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const {
    if (stop_to_wait_vertex_.count(from) == 0 || stop_to_wait_vertex_.count(to) == 0) {
        return std::nullopt;
    }

    auto route = router_->BuildRoute(stop_to_wait_vertex_.at(from), stop_to_wait_vertex_.at(to));
    if (!route) {
        return std::nullopt;
    }

    RouteInfo info;
    info.total_time = route->weight;
    for (graph::EdgeId edge_id : route->edges) {
        const auto& edge = edge_info_[edge_id];
        info.items.push_back({edge.type, edge.name, edge.time, edge.span_count});
    }

    return info;
}

} // namespace transport
