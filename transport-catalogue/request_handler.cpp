#include "request_handler.h"

RequestHandler::RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer, const transport::TransportRouter& router) 
    : db_(db), renderer_(renderer), router_(router) {}

std::optional<transport::domain::BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    return db_.GetBusInfo(bus_name);
}

std::optional<transport::domain::StopInfo> RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    return db_.GetStopInfo(stop_name);
}

std::optional<transport::RouteInfo> RequestHandler::GetRoute(const std::string_view& from, const std::string_view& to) const {
    return router_.BuildRoute(from, to);
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.Render(db_.GetSortedAllBuses());
}
