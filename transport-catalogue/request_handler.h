#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "svg.h"
#include <optional>
#include <string_view>

class RequestHandler {
public:
    RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer);

    std::optional<transport::domain::BusStat> GetBusStat(const std::string_view& bus_name) const;
    std::optional<transport::domain::StopInfo> GetBusesByStop(const std::string_view& stop_name) const;
    svg::Document RenderMap() const;

private:
    const transport::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
};
