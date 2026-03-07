#include "json_reader.h"
#include <vector>
#include <string>
#include <sstream>

using namespace std;

JsonReader::JsonReader(istream& input) : document_(json::Load(input)) {}

void JsonReader::ProcessBaseRequests(transport::TransportCatalogue& catalogue) const {
    const auto& root_dict = document_.GetRoot().AsDict();
    if (root_dict.count("base_requests") == 0) return;
    
    const auto& base_requests = root_dict.at("base_requests").AsArray();

    ProcessStops(base_requests, catalogue);
    ProcessDistances(base_requests, catalogue);
    ProcessBuses(base_requests, catalogue);
}

void JsonReader::ProcessStops(const json::Array& base_requests, transport::TransportCatalogue& catalogue) const {
    for (const auto& req : base_requests) {
        const auto& dict = req.AsDict();
        if (dict.at("type").AsString() == "Stop") {
            catalogue.AddStop(dict.at("name").AsString(), {dict.at("latitude").AsDouble(), dict.at("longitude").AsDouble()});
        }
    }
}

void JsonReader::ProcessDistances(const json::Array& base_requests, transport::TransportCatalogue& catalogue) const {
    for (const auto& req : base_requests) {
        const auto& dict = req.AsDict();
        if (dict.at("type").AsString() == "Stop" && dict.count("road_distances")) {
            const string& from = dict.at("name").AsString();
            for (const auto& [to, dist_node] : dict.at("road_distances").AsDict()) {
                if (!catalogue.FindStop(to)) { catalogue.AddStop(to, {0, 0}); }
                catalogue.SetDistance(catalogue.FindStop(from), catalogue.FindStop(to), dist_node.AsInt());
            }
        }
    }
}

void JsonReader::ProcessBuses(const json::Array& base_requests, transport::TransportCatalogue& catalogue) const {
    for (const auto& req : base_requests) {
        const auto& dict = req.AsDict();
        if (dict.at("type").AsString() == "Bus") {
            vector<string_view> stops;
            for (const auto& stop_node : dict.at("stops").AsArray()) {
                stops.push_back(stop_node.AsString());
            }
            catalogue.AddBus(dict.at("name").AsString(), stops, dict.at("is_roundtrip").AsBool());
        }
    }
}

bool JsonReader::HasRenderSettings() const {
    return document_.GetRoot().AsDict().count("render_settings") > 0;
}

svg::Color JsonReader::ParseColor(const json::Node& node) const {
    if (node.IsString()) return node.AsString();
    if (node.IsArray()) {
        const auto& arr = node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb{static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt())};
        } else if (arr.size() == 4) {
            return svg::Rgba{static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt()), arr[3].AsDouble()};
        }
    }
    return svg::NoneColor;
}

renderer::RenderSettings JsonReader::ParseRenderSettings() const {
    renderer::RenderSettings settings;
    const auto& root_dict = document_.GetRoot().AsDict();
    if (root_dict.count("render_settings") == 0) return settings;

    const auto& dict = root_dict.at("render_settings").AsDict();

    settings.width = dict.at("width").AsDouble();
    settings.height = dict.at("height").AsDouble();
    settings.padding = dict.at("padding").AsDouble();
    settings.line_width = dict.at("line_width").AsDouble();
    settings.stop_radius = dict.at("stop_radius").AsDouble();

    settings.bus_label_font_size = dict.at("bus_label_font_size").AsInt();
    const auto& blo = dict.at("bus_label_offset").AsArray();
    settings.bus_label_offset = {blo[0].AsDouble(), blo[1].AsDouble()};

    settings.stop_label_font_size = dict.at("stop_label_font_size").AsInt();
    const auto& slo = dict.at("stop_label_offset").AsArray();
    settings.stop_label_offset = {slo[0].AsDouble(), slo[1].AsDouble()};

    settings.underlayer_color = ParseColor(dict.at("underlayer_color"));
    settings.underlayer_width = dict.at("underlayer_width").AsDouble();

    for (const auto& color_node : dict.at("color_palette").AsArray()) {
        settings.color_palette.push_back(ParseColor(color_node));
    }

    return settings;
}

void JsonReader::ProcessStatRequests(const RequestHandler& handler, ostream& out) const {
    const auto& root_dict = document_.GetRoot().AsDict();
    if (root_dict.count("stat_requests") == 0) {
        json::Print(json::Document(json::Array{}), out);
        return;
    }

    json::Array responses;
    for (const auto& req : root_dict.at("stat_requests").AsArray()) {
        const auto& dict = req.AsDict();
        int request_id = dict.at("id").AsInt();
        const string& type = dict.at("type").AsString();
        
        json::Dict response_dict;
        response_dict["request_id"] = request_id;

        if (type == "Bus") {
            const string& name = dict.at("name").AsString();
            auto stat = handler.GetBusStat(name);
            if (stat) {
                response_dict["curvature"] = stat->curvature;
                response_dict["route_length"] = stat->route_length;
                response_dict["stop_count"] = static_cast<int>(stat->stops_count);
                response_dict["unique_stop_count"] = static_cast<int>(stat->unique_stops_count);
            } else {
                response_dict["error_message"] = "not found"s;
            }
        } else if (type == "Stop") {
            const string& name = dict.at("name").AsString();
            auto stat = handler.GetBusesByStop(name);
            if (stat) {
                json::Array buses;
                for (const auto& bus : *stat->buses) {
                    buses.push_back(string(bus));
                }
                response_dict["buses"] = move(buses);
            } else {
                response_dict["error_message"] = "not found"s;
            }
        } else if (type == "Map") {
            ostringstream strm;
            handler.RenderMap().Render(strm);
            response_dict["map"] = strm.str();
        }
        
        responses.push_back(json::Node(move(response_dict)));
    }
    
    json::Print(json::Document(json::Node(move(responses))), out);
}
