#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"
#include <iostream>

class JsonReader {
public:
    explicit JsonReader(std::istream& input);

    void ProcessBaseRequests(transport::TransportCatalogue& catalogue) const;
    renderer::RenderSettings ParseRenderSettings() const;
    bool HasRenderSettings() const;
    transport::RoutingSettings ParseRoutingSettings() const;
    void ProcessStatRequests(const RequestHandler& handler, std::ostream& out) const;

private:
    json::Document document_;
    svg::Color ParseColor(const json::Node& node) const;
    void ProcessStops(const json::Array& base_requests, transport::TransportCatalogue& catalogue) const;
    void ProcessDistances(const json::Array& base_requests, transport::TransportCatalogue& catalogue) const;
    void ProcessBuses(const json::Array& base_requests, transport::TransportCatalogue& catalogue) const;
};
