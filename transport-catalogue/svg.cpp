#include "svg.h"
#include <algorithm>

using namespace std::literals;

namespace svg {

void Object::Render(const RenderContext& context) const {
    RenderObject(context);
}

Circle& Circle::SetCenter(Point center) { center_ = center; return *this; }
Circle& Circle::SetRadius(double radius) { radius_ = radius; return *this; }
void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Polyline& Polyline::AddPoint(Point point) { points_.push_back(point); return *this; }
void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool first = true;
    for (const auto& p : points_) {
        if (!first) { out << ' '; }
        first = false;
        out << p.x << ',' << p.y;
    }
    out << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Text& Text::SetPosition(Point pos) { pos_ = pos; return *this; }
Text& Text::SetOffset(Point offset) { offset_ = offset; return *this; }
Text& Text::SetFontSize(uint32_t size) { font_size_ = size; return *this; }
Text& Text::SetFontFamily(std::string font_family) { font_family_ = std::move(font_family); return *this; }
Text& Text::SetFontWeight(std::string font_weight) { font_weight_ = std::move(font_weight); return *this; }
Text& Text::SetData(std::string data) { data_ = std::move(data); return *this; }

namespace {
std::string EscapeText(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '&': result += "&amp;"s; break;
            case '"': result += "&quot;"s; break;
            case '\'': result += "&apos;"s; break;
            case '<': result += "&lt;"s; break;
            case '>': result += "&gt;"s; break;
            default: result += c; break;
        }
    }
    return result;
}
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(out);
    out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << '"';
    out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << '"';
    out << " font-size=\""sv << font_size_ << '"';
    if (!font_family_.empty()) out << " font-family=\""sv << font_family_ << '"';
    if (!font_weight_.empty()) out << " font-weight=\""sv << font_weight_ << '"';
    out << '>' << EscapeText(data_) << "</text>"sv;
}

void Document::AddPtr(std::unique_ptr<Object>&& obj) { objects_.push_back(std::move(obj)); }
void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;
    RenderContext ctx(out);
    for (const auto& obj : objects_) {
        out << "  ";
        obj->Render(ctx);
        out << "\n";
    }
    out << "</svg>"sv; 
}

}  // namespace svg
