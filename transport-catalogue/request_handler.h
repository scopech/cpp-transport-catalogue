#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "svg.h"
#include <optional>
#include <string_view>

class RequestHandler {
public:
    RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer, const transport::TransportRouter& router);

    std::optional<transport::domain::BusStat> GetBusStat(const std::string_view& bus_name) const;
    std::optional<transport::domain::StopInfo> GetBusesByStop(const std::string_view& stop_name) const;
    std::optional<transport::RouteInfo> GetRoute(const std::string_view& from, const std::string_view& to) const;
    svg::Document RenderMap() const;

private:
    const transport::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    const transport::TransportRouter& router_;
};
