#include "request_handler.h"

RequestHandler::RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer) 
    : db_(db), renderer_(renderer) {}

std::optional<transport::domain::BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    return db_.GetBusInfo(bus_name);
}

std::optional<transport::domain::StopInfo> RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    return db_.GetStopInfo(stop_name);
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.Render(db_.GetSortedAllBuses());
}
