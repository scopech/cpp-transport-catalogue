#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

struct Point {
    Point() = default;
    Point(double x, double y) : x(x), y(y) {}
    double x = 0;
    double y = 0;
};

struct Rgb {
    Rgb() = default;
    Rgb(uint8_t red, uint8_t green, uint8_t blue) : red(red), green(green), blue(blue) {}
    uint8_t red = 0, green = 0, blue = 0;
};

struct Rgba {
    Rgba() = default;
    Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity)
        : red(red), green(green), blue(blue), opacity(opacity) {}
    uint8_t red = 0, green = 0, blue = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const Color NoneColor{"none"};

struct ColorPrinter {
    std::ostream& out;
    void operator()(std::monostate) const { out << "none"; }
    void operator()(const std::string& str) const { out << str; }
    void operator()(Rgb rgb) const {
        out << "rgb(" << static_cast<int>(rgb.red) << "," 
            << static_cast<int>(rgb.green) << "," << static_cast<int>(rgb.blue) << ")";
    }
    void operator()(Rgba rgba) const {
        out << "rgba(" << static_cast<int>(rgba.red) << "," << static_cast<int>(rgba.green) << "," 
            << static_cast<int>(rgba.blue) << "," << rgba.opacity << ")";
    }
};

inline std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit(ColorPrinter{out}, color);
    return out;
}

enum class StrokeLineCap { BUTT, ROUND, SQUARE };
enum class StrokeLineJoin { ARCS, BEVEL, MITER, MITER_CLIP, ROUND };

inline std::ostream& operator<<(std::ostream& out, StrokeLineCap cap) {
    switch (cap) {
        case StrokeLineCap::BUTT: out << "butt"; break;
        case StrokeLineCap::ROUND: out << "round"; break;
        case StrokeLineCap::SQUARE: out << "square"; break;
    }
    return out;
}

inline std::ostream& operator<<(std::ostream& out, StrokeLineJoin join) {
    switch (join) {
        case StrokeLineJoin::ARCS: out << "arcs"; break;
        case StrokeLineJoin::BEVEL: out << "bevel"; break;
        case StrokeLineJoin::MITER: out << "miter"; break;
        case StrokeLineJoin::MITER_CLIP: out << "miter-clip"; break;
        case StrokeLineJoin::ROUND: out << "round"; break;
    }
    return out;
}

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) { fill_color_ = std::move(color); return AsOwner(); }
    Owner& SetStrokeColor(Color color) { stroke_color_ = std::move(color); return AsOwner(); }
    Owner& SetStrokeWidth(double width) { stroke_width_ = width; return AsOwner(); }
    Owner& SetStrokeLineCap(StrokeLineCap line_cap) { stroke_line_cap_ = line_cap; return AsOwner(); }
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) { stroke_line_join_ = line_join; return AsOwner(); }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;
        if (fill_color_) { out << " fill=\""sv << *fill_color_ << "\""sv; }
        if (stroke_color_) { out << " stroke=\""sv << *stroke_color_ << "\""sv; }
        if (stroke_width_) { out << " stroke-width=\""sv << *stroke_width_ << "\""sv; }
        if (stroke_line_cap_) { out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv; }
        if (stroke_line_join_) { out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv; }
    }

private:
    Owner& AsOwner() { return static_cast<Owner&>(*this); }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;
};

struct RenderContext {
    RenderContext(std::ostream& out) : out(out) {}
    std::ostream& out;
};

class Object {
public:
    void Render(const RenderContext& context) const;
    virtual ~Object() = default;
private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);
private:
    void RenderObject(const RenderContext& context) const override;
    Point center_;
    double radius_ = 1.0;
};

class Polyline final : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(Point point);
private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> points_;
};

class Text final : public Object, public PathProps<Text> {
public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);
    Text& SetData(std::string data);
private:
    void RenderObject(const RenderContext& context) const override;
    Point pos_;
    Point offset_;
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class ObjectContainer {
public:
    template <typename Obj>
    void Add(Obj obj) { AddPtr(std::make_unique<Obj>(std::move(obj))); }
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
protected:
    ~ObjectContainer() = default;
};

class Document : public ObjectContainer {
public:
    void AddPtr(std::unique_ptr<Object>&& obj) override;
    void Render(std::ostream& out) const;
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg
