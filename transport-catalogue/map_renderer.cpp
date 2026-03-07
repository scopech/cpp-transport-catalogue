#include "map_renderer.h"

namespace renderer {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

MapRenderer::MapRenderer(RenderSettings settings) : settings_(std::move(settings)) {}

svg::Document MapRenderer::Render(const std::map<std::string_view, const transport::domain::Bus*>& buses) const {
    svg::Document doc;
    std::vector<geo::Coordinates> all_coords;
    std::map<std::string_view, const transport::domain::Stop*> used_stops;

    for (const auto& [name, bus] : buses) {
        if (bus->stops.empty()) continue;
        for (const auto* stop : bus->stops) {
            all_coords.push_back(stop->coordinates);
            used_stops[stop->name] = stop;
        }
    }

    SphereProjector proj(all_coords.begin(), all_coords.end(),
                         settings_.width, settings_.height, settings_.padding);

    RenderRouteLines(doc, buses, proj);
    RenderBusLabels(doc, buses, proj);
    RenderStopSymbols(doc, used_stops, proj);
    RenderStopLabels(doc, used_stops, proj);

    return doc;
}

void MapRenderer::RenderRouteLines(svg::Document& doc, const std::map<std::string_view, const transport::domain::Bus*>& buses, const SphereProjector& proj) const {
    size_t color_idx = 0;
    for (const auto& [name, bus] : buses) {
        if (bus->stops.empty()) continue;

        svg::Polyline route_line;
        for (const auto* stop : bus->stops) {
            route_line.AddPoint(proj(stop->coordinates));
        }

        route_line.SetFillColor(svg::NoneColor);
        route_line.SetStrokeColor(settings_.color_palette[color_idx % settings_.color_palette.size()]);
        route_line.SetStrokeWidth(settings_.line_width);
        route_line.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        route_line.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        doc.Add(route_line);
        ++color_idx;
    }
}

void MapRenderer::RenderBusLabels(svg::Document& doc, const std::map<std::string_view, const transport::domain::Bus*>& buses, const SphereProjector& proj) const {
    size_t color_idx = 0;
    for (const auto& [name, bus] : buses) {
        if (bus->stops.empty()) continue;

        auto color = settings_.color_palette[color_idx % settings_.color_palette.size()];
        ++color_idx;

        auto add_bus_label = [&](const transport::domain::Stop* stop) {
            svg::Point pos = proj(stop->coordinates);

            svg::Text underlayer;
            underlayer.SetPosition(pos).SetOffset(settings_.bus_label_offset)
                      .SetFontSize(settings_.bus_label_font_size)
                      .SetFontFamily("Verdana").SetFontWeight("bold").SetData(bus->name)
                      .SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color)
                      .SetStrokeWidth(settings_.underlayer_width)
                      .SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            doc.Add(underlayer);

            svg::Text foreground;
            foreground.SetPosition(pos).SetOffset(settings_.bus_label_offset)
                      .SetFontSize(settings_.bus_label_font_size)
                      .SetFontFamily("Verdana").SetFontWeight("bold").SetData(bus->name)
                      .SetFillColor(color);
            doc.Add(foreground);
        };

        add_bus_label(bus->stops.front());
        if (!bus->is_roundtrip) {
            const auto* end_stop = bus->stops[bus->stops.size() / 2];
            if (end_stop != bus->stops.front()) add_bus_label(end_stop);
        }
    }
}

void MapRenderer::RenderStopSymbols(svg::Document& doc, const std::map<std::string_view, const transport::domain::Stop*>& stops, const SphereProjector& proj) const {
    for (const auto& [name, stop] : stops) {
        svg::Circle circle;
        circle.SetCenter(proj(stop->coordinates))
              .SetRadius(settings_.stop_radius).SetFillColor("white");
        doc.Add(circle);
    }
}

void MapRenderer::RenderStopLabels(svg::Document& doc, const std::map<std::string_view, const transport::domain::Stop*>& stops, const SphereProjector& proj) const {
    for (const auto& [name, stop] : stops) {
        svg::Point pos = proj(stop->coordinates);

        svg::Text underlayer;
        underlayer.SetPosition(pos).SetOffset(settings_.stop_label_offset)
                  .SetFontSize(settings_.stop_label_font_size).SetFontFamily("Verdana").SetData(stop->name)
                  .SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color)
                  .SetStrokeWidth(settings_.underlayer_width)
                  .SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        doc.Add(underlayer);

        svg::Text foreground;
        foreground.SetPosition(pos).SetOffset(settings_.stop_label_offset)
                  .SetFontSize(settings_.stop_label_font_size).SetFontFamily("Verdana").SetData(stop->name)
                  .SetFillColor("black");
        doc.Add(foreground);
    }
}

} // namespace renderer
