#pragma once

#include "transport_catalogue.h"
#include "router.h"
#include <memory>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>

namespace transport {

struct RoutingSettings {
    int bus_wait_time = 0;
    double bus_velocity = 0.0;
};

struct RouteItem {
    std::string type;
    std::string name; // имя остановки или автобуса
    double time = 0.0;
    int span_count = 0; // только для Bus
};

struct RouteInfo {
    double total_time = 0.0;
    std::vector<RouteItem> items;
};

class TransportRouter {
public:
    TransportRouter(const TransportCatalogue& catalogue, const RoutingSettings& settings);

    std::optional<RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

private:
    const TransportCatalogue& catalogue_;
    RoutingSettings settings_;
    
    std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
    std::unique_ptr<graph::Router<double>> router_;

    std::unordered_map<std::string_view, graph::VertexId> stop_to_wait_vertex_;
    std::unordered_map<std::string_view, graph::VertexId> stop_to_board_vertex_;

    struct EdgeInfo {
        std::string type;
        std::string name;
        int span_count = 0;
        double time = 0.0;
    };
    std::vector<EdgeInfo> edge_info_;

    void BuildGraph();
};

} // namespace transport
