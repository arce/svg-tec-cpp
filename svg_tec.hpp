#ifndef SVG_TEC_HPP
#define SVG_TEC_HPP

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <fstream>

namespace SVG {

using Points = std::vector<std::pair<double, double>>;

constexpr double PI = 3.14159265358979323846;

struct Color {
  int r, g, b;
  Color(int r = 0, int g = 0, int b = 0) : r(r), g(g), b(b) {}
  Color(const std::string& hex) {
    std::string h = hex;
    if (h.size() > 1 && h[0] == '#') h = h.substr(1);
    else if (h.size() > 2 && h[0] == '0' && (h[1] == 'x' || h[1] == 'X')) h = h.substr(2);
    bool valid = h.size() == 6 && std::all_of(h.begin(), h.end(), [](unsigned char c) { return std::isxdigit(c); });
    if (!valid) throw std::invalid_argument("Color: \"" + hex + "\" is not a valid hex color, expected #RRGGBB");
    r = std::stoi(h.substr(0, 2), nullptr, 16);
    g = std::stoi(h.substr(2, 2), nullptr, 16);
    b = std::stoi(h.substr(4, 2), nullptr, 16);
  }
  Color(const char* hex) : Color(std::string(hex)) {}
  static Color hex(unsigned int rgb) { return Color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF); }
  std::string toHex() const {
    std::stringstream ss;
    ss << "#" << std::hex << std::setfill('0')
       << std::setw(2) << r
       << std::setw(2) << g
       << std::setw(2) << b;
    return ss.str();
  }
};

struct Stroke {
  double width{1.0};
  std::string dashArray{};
  std::string cap{};
  std::string toString() const {
    std::stringstream ss;
    ss << "stroke-width=\"" << width << "\" ";
    if (!dashArray.empty()) {
      ss << "stroke-dasharray=\"" << dashArray << "\" ";
    }
    if (!cap.empty()) {
      ss << "stroke-linecap=\"" << cap << "\" ";
    }
    return ss.str();
  }
};

struct Transform {
  std::string value;
  Transform() = default;
  Transform(const std::string& v) : value(v) {}
  static Transform translate(double x, double y) {
    std::stringstream ss;
    ss << "translate(" << x << "," << y << ")";
    return Transform(ss.str());
  }
  static Transform scale(double sx, double sy) {
    std::stringstream ss;
    ss << "scale(" << sx << "," << sy << ")";
    return Transform(ss.str());
  }
  static Transform rotate(double angle, double cx = 0, double cy = 0) {
    std::stringstream ss;
    ss << "rotate(" << angle;
    if (cx != 0 || cy != 0) ss << "," << cx << "," << cy;
    ss << ")";
    return Transform(ss.str());
  }
  static Transform skewX(double angle) {
    std::stringstream ss;
    ss << "skewX(" << angle << ")";
    return Transform(ss.str());
  }
  static Transform skewY(double angle) {
    std::stringstream ss;
    ss << "skewY(" << angle << ")";
    return Transform(ss.str());
  }
  Transform combine(const Transform& t) const { return Transform(value + " " + t.value); }
  std::string toString() const { return value; }
};

class Shape {
protected:
  Color fillColor;
  Color strokeColor;
  bool hasFill{true};
  bool hasStroke{false};
  Stroke strokeStyle;
  std::string xform;

  std::string tag(const std::string& name, const std::string& attrs, bool useFill = true, bool useStroke = true, const std::string& body = "") const {
    std::stringstream ss;
    ss << "<" << name << attrs;
    if (useFill) ss << (hasFill ? " fill=\"" + fillColor.toHex() + "\"" : " fill=\"none\"");
    if (useStroke && hasStroke) ss << " stroke=\"" << strokeColor.toHex() << "\" " << strokeStyle.toString();
    if (!xform.empty()) ss << " transform=\"" << xform << "\"";
    if (body.empty()) ss << "/>"; else ss << ">" << body << "</" << name << ">";
    return ss.str();
  }
public:
  virtual ~Shape() = default;

  Shape& fill(const Color& c) { fillColor = c; hasFill = true; return *this; }
  Shape& noFill() { hasFill = false; return *this; }
  Shape& stroke(const Color& c) { strokeColor = c; hasStroke = true; return *this; }
  Shape& stroke(double width, const std::string& dashArray = "") { strokeStyle = Stroke{width, dashArray}; hasStroke = true; return *this; }
  Shape& noStroke() { hasStroke = false; return *this; }
  Shape& cap(const std::string& c) { strokeStyle.cap = c; hasStroke = true; return *this; }
  Shape& transform(const std::string& t) { xform = t; return *this; }
  Shape& transform(const Transform& t) { return transform(t.toString()); }
  Shape& addTransform(const Transform& t) { return transform(xform.empty() ? t.toString() : xform + " " + t.toString()); }
  Shape& translate(double x, double y) { return addTransform(Transform::translate(x, y)); }
  Shape& rotate(double angle, double cx = 0, double cy = 0) { return addTransform(Transform::rotate(angle, cx, cy)); }
  Shape& scale(double sx, double sy) { return addTransform(Transform::scale(sx, sy)); }
  Shape& skewX(double angle) { return addTransform(Transform::skewX(angle)); }
  Shape& skewY(double angle) { return addTransform(Transform::skewY(angle)); }

  virtual std::string toSVG() const = 0;
};

class Rectangle : public Shape {
private:
  double x, y, width, height, rx;
public:
  Rectangle(double x, double y, double width, double height, double rx = 0)
    : x(x), y(y), width(width), height(height), rx(rx) {}

  std::string toSVG() const override {
    std::stringstream a;
    a << " x=\"" << x << "\" y=\"" << y << "\" width=\"" << width << "\" height=\"" << height << "\"";
    if (rx > 0) a << " rx=\"" << rx << "\" ry=\"" << rx << "\"";
    return tag("rect", a.str());
  }
};

class Circle : public Shape {
private:
  double cx, cy, r;
public:
  Circle(double cx, double cy, double r) : cx(cx), cy(cy), r(r) {}

  std::string toSVG() const override {
    std::stringstream a;
    a << " cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"" << r << "\"";
    return tag("circle", a.str());
  }
};

class Ellipse : public Shape {
private:
  double cx, cy, rx, ry;
public:
  Ellipse(double cx, double cy, double rx, double ry) : cx(cx), cy(cy), rx(rx), ry(ry) {}

  std::string toSVG() const override {
    std::stringstream a;
    a << " cx=\"" << cx << "\" cy=\"" << cy << "\" rx=\"" << rx << "\" ry=\"" << ry << "\"";
    return tag("ellipse", a.str());
  }
};

class Line : public Shape {
private:
  double x1, y1, x2, y2;
public:
  Line(double x1, double y1, double x2, double y2) : x1(x1), y1(y1), x2(x2), y2(y2) {
    stroke(Color(0, 0, 0));
  }

  std::string toSVG() const override {
    std::stringstream a;
    a << " x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\"";
    return tag("line", a.str(), false);
  }
};

class Polyline : public Shape {
protected:
  Points points;

  std::string pointsAttr() const {
    std::stringstream ss;
    ss << " points=\"";
    for (size_t i = 0; i < points.size(); ++i) {
      if (i > 0) ss << " ";
      ss << points[i].first << "," << points[i].second;
    }
    ss << "\"";
    return ss.str();
  }
public:
  Polyline() { noFill(); }
  Polyline(const Points& pts) : points(pts) { noFill(); }
  void addPoint(double x, double y) { points.emplace_back(x, y); }

  std::string toSVG() const override { return tag("polyline", pointsAttr()); }
};

class Polygon : public Polyline {
public:
  Polygon() { fill(Color(0, 0, 0)); }
  Polygon(const Points& pts) : Polyline(pts) { fill(Color(0, 0, 0)); }

  std::string toSVG() const override { return tag("polygon", pointsAttr()); }
};

class Wedge : public Polygon {
public:
  Wedge(double x, double y, double r1, double r2, double a1, double a2, int segments = 48) {
    fill(Color(0, 0, 0));
    double rad1 = a1 * PI / 180.0;
    double rad2 = a2 * PI / 180.0;
    for (int i = 0; i <= segments; ++i) {
      double a = rad1 + (rad2 - rad1) * (static_cast<double>(i) / segments);
      addPoint(x + r2 * std::cos(a), y + r2 * std::sin(a));
    }
    if (r1 == 0) {
      addPoint(x, y);
    } else {
      for (int i = segments; i >= 0; --i) {
        double a = rad1 + (rad2 - rad1) * (static_cast<double>(i) / segments);
        addPoint(x + r1 * std::cos(a), y + r1 * std::sin(a));
      }
    }
  }
};

class QuadCurve : public Polyline {
public:
  QuadCurve(double x1, double y1, double cx, double cy, double x2, double y2, int segments = 32) {
    for (int i = 0; i <= segments; ++i) {
      double t = static_cast<double>(i) / segments;
      double mt = 1 - t;
      addPoint(mt * mt * x1 + 2 * mt * t * cx + t * t * x2, mt * mt * y1 + 2 * mt * t * cy + t * t * y2);
    }
  }
};

class CubicCurve : public Polyline {
public:
  CubicCurve(double x1, double y1, double cx1, double cy1, double cx2, double cy2, double x2, double y2, int segments = 32) {
    for (int i = 0; i <= segments; ++i) {
      double t = static_cast<double>(i) / segments;
      double mt = 1 - t;
      double x = mt * mt * mt * x1 + 3 * mt * mt * t * cx1 + 3 * mt * t * t * cx2 + t * t * t * x2;
      double y = mt * mt * mt * y1 + 3 * mt * mt * t * cy1 + 3 * mt * t * t * cy2 + t * t * t * y2;
      addPoint(x, y);
    }
  }
};

class Text : public Shape {
private:
  double x, y;
  std::string text;
  std::string fontFamily;
  double fontSize;
public:
  Text(double x, double y, const std::string& text) : x(x), y(y), text(text), fontSize(12) {}

  Text& font(const std::string& family, double size) { fontFamily = family; fontSize = size; return *this; }

  std::string toSVG() const override {
    std::stringstream a;
    a << " x=\"" << x << "\" y=\"" << y << "\"";
    if (!fontFamily.empty()) a << " font-family=\"" << fontFamily << "\"";
    if (fontSize > 0) a << " font-size=\"" << fontSize << "\"";
    return tag("text", a.str(), true, false, text);
  }
};

class Group;

class ShapeContainer {
protected:
  std::vector<std::shared_ptr<Shape>> shapes;
public:
  template <typename ShapeT, typename... Args>
  ShapeT& create(Args&&... args) {
    auto s = std::make_shared<ShapeT>(std::forward<Args>(args)...);
    shapes.push_back(s);
    return *s;
  }

  ShapeContainer& add(std::shared_ptr<Shape> shape) { shapes.push_back(shape); return *this; }

  Rectangle& rect(double x, double y, double w, double h, double rx = 0) { return create<Rectangle>(x, y, w, h, rx); }
  Circle& circle(double cx, double cy, double r) { return create<Circle>(cx, cy, r); }
  Ellipse& ellipse(double cx, double cy, double rx, double ry) { return create<Ellipse>(cx, cy, rx, ry); }
  Line& line(double x1, double y1, double x2, double y2) { return create<Line>(x1, y1, x2, y2); }
  Polyline& polyline(const Points& pts) { return create<Polyline>(pts); }
  Polygon& polygon(const Points& pts) { return create<Polygon>(pts); }
  Wedge& wedge(double x, double y, double r1, double r2, double a1, double a2) { return create<Wedge>(x, y, r1, r2, a1, a2); }
  QuadCurve& quadCurve(double x1, double y1, double cx, double cy, double x2, double y2) { return create<QuadCurve>(x1, y1, cx, cy, x2, y2); }
  CubicCurve& cubicCurve(double x1, double y1, double cx1, double cy1, double cx2, double cy2, double x2, double y2) { return create<CubicCurve>(x1, y1, cx1, cy1, cx2, cy2, x2, y2); }
  Text& text(double x, double y, const std::string& t) { return create<Text>(x, y, t); }
  Group& group();
};

class Group : public Shape, public ShapeContainer {
public:
  Group() = default;

  std::string toSVG() const override {
    std::stringstream b;
    for (const auto& shape : shapes) b << "\n  " << shape->toSVG();
    b << "\n";
    return tag("g", "", false, false, b.str());
  }
};

inline Group& ShapeContainer::group() { return create<Group>(); }

class Svg : public ShapeContainer {
private:
  double width, height;
public:
  static constexpr const char* BUTT = "butt";
  static constexpr const char* ROUND = "round";
  static constexpr const char* SQUARE = "square";

  Svg(double width, double height) : width(width), height(height) {}

  std::string toString() const {
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width << "\" height=\"" << height << "\">\n";
    for (const auto& shape : shapes) {
      ss << "  " << shape->toSVG() << "\n";
    }
    ss << "</svg>";
    return ss.str();
  }

  void save(const std::string& filename) const {
    std::ofstream(filename) << toString();
  }
};

} // namespace SVG

#endif // SVG_TEC_HPP
