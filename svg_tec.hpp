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
#include <charconv>
#include <initializer_list>

namespace SVG {

using Points = std::vector<std::pair<double, double>>;

constexpr double PI = 3.14159265358979323846;

inline std::string num(double v) {
  if (!std::isfinite(v)) throw std::invalid_argument("SVG::num: NaN or infinite value");
  char buf[400];
  auto r = std::to_chars(buf, buf + sizeof buf, v, std::chars_format::fixed, 2);
  std::string s(buf, r.ptr);
  if (s.find('.') != std::string::npos) {
    while (s.back() == '0') s.pop_back();
    if (s.back() == '.') s.pop_back();
  }
  return s == "-0" ? "0" : s;
}

inline std::string esc(const std::string& s) {
  std::string o;
  o.reserve(s.size());
  for (char c : s) {
    switch (c) {
      case '&': o += "&amp;"; break;
      case '<': o += "&lt;"; break;
      case '>': o += "&gt;"; break;
      case '"': o += "&quot;"; break;
      case '\'': o += "&apos;"; break;
      default: o += c;
    }
  }
  return o;
}

struct Color {
  int r, g, b;
  static int clamp(int v) { return std::min(255, std::max(0, v)); }
  Color(int r_ = 0, int g_ = 0, int b_ = 0) : r(clamp(r_)), g(clamp(g_)), b(clamp(b_)) {}
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
    ss << "#" << std::hex << std::setfill('0') << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b;
    return ss.str();
  }
};

struct Stroke {
  double width{1.0};
  std::string dashArray{};
  std::string cap{};
  std::string join{};
  std::string toString() const {
    std::string s = " stroke-width=\"" + num(width) + "\"";
    if (!dashArray.empty()) s += " stroke-dasharray=\"" + esc(dashArray) + "\"";
    if (!cap.empty()) s += " stroke-linecap=\"" + esc(cap) + "\"";
    if (!join.empty()) s += " stroke-linejoin=\"" + esc(join) + "\"";
    return s;
  }
};

struct Transform {
  std::string value;
  Transform() = default;
  Transform(const std::string& v) : value(v) {}
  static Transform translate(double x, double y) { return Transform("translate(" + num(x) + "," + num(y) + ")"); }
  static Transform scale(double sx, double sy) { return Transform("scale(" + num(sx) + "," + num(sy) + ")"); }
  static Transform rotate(double angle, double cx = 0, double cy = 0) {
    std::string s = "rotate(" + num(angle);
    if (cx != 0 || cy != 0) s += "," + num(cx) + "," + num(cy);
    return Transform(s + ")");
  }
  static Transform skewX(double angle) { return Transform("skewX(" + num(angle) + ")"); }
  static Transform skewY(double angle) { return Transform("skewY(" + num(angle) + ")"); }
  Transform combine(const Transform& t) const { return Transform(value + " " + t.value); }
  std::string toString() const { return value; }
};

class Shape {
protected:
  Color fillColor;
  Color strokeColor;
  bool hasFill{true};
  bool hasStroke{false};
  bool fillSet{false};
  bool strokeSet{false};
  Stroke strokeStyle;
  double alpha{-1};
  std::string xform, idv, clsv, extra;

  std::string tag(const std::string& name, const std::string& attrs, bool useFill = true, bool useStroke = true, const std::string& body = "") const {
    std::stringstream ss;
    ss << "<" << name << attrs;
    if (!idv.empty()) ss << " id=\"" << esc(idv) << "\"";
    if (!clsv.empty()) ss << " class=\"" << esc(clsv) << "\"";
    if (useFill && fillSet) ss << (hasFill ? " fill=\"" + fillColor.toHex() + "\"" : " fill=\"none\"");
    if (useStroke && strokeSet) {
      if (hasStroke) ss << " stroke=\"" << strokeColor.toHex() << "\"" << strokeStyle.toString();
      else ss << " stroke=\"none\"";
    }
    if (alpha >= 0) ss << " opacity=\"" << num(alpha) << "\"";
    ss << extra;
    if (!xform.empty()) ss << " transform=\"" << esc(xform) << "\"";
    if (body.empty()) ss << "/>"; else ss << ">" << body << "</" << name << ">";
    return ss.str();
  }
public:
  virtual ~Shape() = default;
  virtual std::string toSVG() const = 0;
};

template <class D>
class ShapeT : public Shape {
  D& self() { return static_cast<D&>(*this); }
public:
  D& fill(const Color& c) { fillColor = c; hasFill = true; fillSet = true; return self(); }
  D& noFill() { hasFill = false; fillSet = true; return self(); }
  D& stroke(const Color& c) { strokeColor = c; hasStroke = true; strokeSet = true; return self(); }
  D& stroke(double width, const std::string& dashArray = "") { strokeStyle.width = width; strokeStyle.dashArray = dashArray; hasStroke = true; strokeSet = true; return self(); }
  D& noStroke() { hasStroke = false; strokeSet = true; return self(); }
  D& cap(const std::string& c) { strokeStyle.cap = c; hasStroke = true; strokeSet = true; return self(); }
  D& join(const std::string& j) { strokeStyle.join = j; hasStroke = true; strokeSet = true; return self(); }
  D& opacity(double a) { alpha = a; return self(); }
  D& id(const std::string& v) { idv = v; return self(); }
  D& cls(const std::string& v) { clsv = v; return self(); }
  D& attr(const std::string& name, const std::string& value) { extra += " " + name + "=\"" + esc(value) + "\""; return self(); }
  D& transform(const std::string& t) { xform = t; return self(); }
  D& transform(const Transform& t) { return transform(t.toString()); }
  D& addTransform(const Transform& t) { return transform(xform.empty() ? t.toString() : xform + " " + t.toString()); }
  D& translate(double x, double y) { return addTransform(Transform::translate(x, y)); }
  D& rotate(double angle, double cx = 0, double cy = 0) { return addTransform(Transform::rotate(angle, cx, cy)); }
  D& scale(double sx, double sy) { return addTransform(Transform::scale(sx, sy)); }
  D& skewX(double angle) { return addTransform(Transform::skewX(angle)); }
  D& skewY(double angle) { return addTransform(Transform::skewY(angle)); }
};

class Rectangle : public ShapeT<Rectangle> {
  double x, y, width, height, rx;
public:
  Rectangle(double x, double y, double width, double height, double rx = 0) : x(x), y(y), width(width), height(height), rx(rx) {}
  std::string toSVG() const override {
    std::string a = " x=\"" + num(x) + "\" y=\"" + num(y) + "\" width=\"" + num(width) + "\" height=\"" + num(height) + "\"";
    if (rx > 0) a += " rx=\"" + num(rx) + "\" ry=\"" + num(rx) + "\"";
    return tag("rect", a);
  }
};

class Circle : public ShapeT<Circle> {
  double cx, cy, r;
public:
  Circle(double cx, double cy, double r) : cx(cx), cy(cy), r(r) {}
  std::string toSVG() const override {
    return tag("circle", " cx=\"" + num(cx) + "\" cy=\"" + num(cy) + "\" r=\"" + num(r) + "\"");
  }
};

class Ellipse : public ShapeT<Ellipse> {
  double cx, cy, rx, ry;
public:
  Ellipse(double cx, double cy, double rx, double ry) : cx(cx), cy(cy), rx(rx), ry(ry) {}
  std::string toSVG() const override {
    return tag("ellipse", " cx=\"" + num(cx) + "\" cy=\"" + num(cy) + "\" rx=\"" + num(rx) + "\" ry=\"" + num(ry) + "\"");
  }
};

class Line : public ShapeT<Line> {
  double x1, y1, x2, y2;
public:
  Line(double x1, double y1, double x2, double y2) : x1(x1), y1(y1), x2(x2), y2(y2) { stroke(Color(0, 0, 0)); }
  std::string toSVG() const override {
    return tag("line", " x1=\"" + num(x1) + "\" y1=\"" + num(y1) + "\" x2=\"" + num(x2) + "\" y2=\"" + num(y2) + "\"", false);
  }
};

class Polyline : public ShapeT<Polyline> {
protected:
  Points points;
  std::string pointsAttr() const {
    std::string s = " points=\"";
    for (size_t i = 0; i < points.size(); ++i) s += (i ? " " : "") + num(points[i].first) + "," + num(points[i].second);
    return s + "\"";
  }
public:
  Polyline() { noFill(); }
  Polyline(const Points& pts) : points(pts) { noFill(); }
  Polyline& addPoint(double x, double y) { points.emplace_back(x, y); return *this; }
  std::string toSVG() const override { return tag("polyline", pointsAttr()); }
};

class Polygon : public Polyline {
public:
  Polygon() { fillSet = false; }
  Polygon(const Points& pts) : Polyline(pts) { fillSet = false; }
  std::string toSVG() const override { return tag("polygon", pointsAttr()); }
};

class Wedge : public Polygon {
public:
  Wedge(double x, double y, double r1, double r2, double a1, double a2, int segments = 48) {
    double rad1 = a1 * PI / 180.0, rad2 = a2 * PI / 180.0;
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
      double t = static_cast<double>(i) / segments, mt = 1 - t;
      addPoint(mt * mt * x1 + 2 * mt * t * cx + t * t * x2, mt * mt * y1 + 2 * mt * t * cy + t * t * y2);
    }
  }
};

class CubicCurve : public Polyline {
public:
  CubicCurve(double x1, double y1, double cx1, double cy1, double cx2, double cy2, double x2, double y2, int segments = 32) {
    for (int i = 0; i <= segments; ++i) {
      double t = static_cast<double>(i) / segments, mt = 1 - t;
      addPoint(mt * mt * mt * x1 + 3 * mt * mt * t * cx1 + 3 * mt * t * t * cx2 + t * t * t * x2,
               mt * mt * mt * y1 + 3 * mt * mt * t * cy1 + 3 * mt * t * t * cy2 + t * t * t * y2);
    }
  }
};

class Path : public ShapeT<Path> {
  std::string d;
  Path& cmd(char c, std::initializer_list<double> v) {
    if (!d.empty()) d += ' ';
    d += c;
    for (double x : v) d += " " + num(x);
    return *this;
  }
public:
  Path() { noFill(); }
  explicit Path(const std::string& data) : d(data) { noFill(); }
  Path& moveTo(double x, double y) { return cmd('M', {x, y}); }
  Path& lineTo(double x, double y) { return cmd('L', {x, y}); }
  Path& quadTo(double cx, double cy, double x, double y) { return cmd('Q', {cx, cy, x, y}); }
  Path& curveTo(double cx1, double cy1, double cx2, double cy2, double x, double y) { return cmd('C', {cx1, cy1, cx2, cy2, x, y}); }
  Path& arcTo(double rx, double ry, double rotation, bool large, bool sweep, double x, double y) { return cmd('A', {rx, ry, rotation, large ? 1.0 : 0.0, sweep ? 1.0 : 0.0, x, y}); }
  Path& close() { if (!d.empty()) d += " Z"; return *this; }
  std::string toSVG() const override { return tag("path", " d=\"" + esc(d) + "\""); }
};

class Text : public ShapeT<Text> {
  double x, y, fontSize{12};
  std::string content, fontFamily, anchorv, weight, fontStyle;
public:
  Text(double x, double y, const std::string& text) : x(x), y(y), content(text) {}
  Text& font(const std::string& family, double size) { fontFamily = family; fontSize = size; return *this; }
  Text& anchor(const std::string& a) { anchorv = a; return *this; }       // start, middle, end
  Text& bold(bool b = true) { weight = b ? "bold" : ""; return *this; }
  Text& italic(bool i = true) { fontStyle = i ? "italic" : ""; return *this; }
  std::string toSVG() const override {
    std::string a = " x=\"" + num(x) + "\" y=\"" + num(y) + "\"";
    if (!fontFamily.empty()) a += " font-family=\"" + esc(fontFamily) + "\"";
    if (fontSize > 0) a += " font-size=\"" + num(fontSize) + "\"";
    if (!anchorv.empty()) a += " text-anchor=\"" + esc(anchorv) + "\"";
    if (!weight.empty()) a += " font-weight=\"" + weight + "\"";
    if (!fontStyle.empty()) a += " font-style=\"" + fontStyle + "\"";
    return tag("text", a, true, true, esc(content));
  }
};

class Raw : public Shape {
  std::string markup;
public:
  explicit Raw(const std::string& s) : markup(s) {}
  std::string toSVG() const override { return markup; }
};

class Group;

class ShapeContainer {
protected:
  std::vector<std::shared_ptr<Shape>> shapes;
public:
  template <typename ShapeT_, typename... Args>
  ShapeT_& create(Args&&... args) {
    auto s = std::make_shared<ShapeT_>(std::forward<Args>(args)...);
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
  Path& path() { return create<Path>(); }
  Path& path(const std::string& d) { return create<Path>(d); }
  Text& text(double x, double y, const std::string& t) { return create<Text>(x, y, t); }
  Raw& raw(const std::string& markup) { return create<Raw>(markup); }
  Group& group();
};

class Group : public ShapeT<Group>, public ShapeContainer {
public:
  std::string toSVG() const override {
    std::string b;
    for (const auto& shape : shapes) b += "\n  " + shape->toSVG();
    return tag("g", "", true, true, b + "\n");
  }
};

inline Group& ShapeContainer::group() { return create<Group>(); }

class Svg : public ShapeContainer {
  double width, height;
  std::string viewBoxAttr, titleText, css, defsMarkup;
public:
  static constexpr const char* BUTT = "butt";
  static constexpr const char* ROUND = "round";
  static constexpr const char* SQUARE = "square";

  Svg(double width, double height) : width(width), height(height) {}

  Svg& viewBox(double x, double y, double w, double h) { viewBoxAttr = num(x) + " " + num(y) + " " + num(w) + " " + num(h); return *this; }
  Svg& title(const std::string& t) { titleText = t; return *this; }
  Svg& style(const std::string& cssText) { css = cssText; return *this; }        // CSS as it is, goes in <style>
  Svg& defs(const std::string& markup) { defsMarkup = markup; return *this; }    // markup as it is, goes in <defs>

  std::string toString() const {
    std::string s = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" + num(width) + "\" height=\"" + num(height) + "\"";
    if (!viewBoxAttr.empty()) s += " viewBox=\"" + viewBoxAttr + "\"";
    s += ">\n";
    if (!titleText.empty()) s += "  <title>" + esc(titleText) + "</title>\n";
    if (!css.empty()) s += "  <style><![CDATA[\n" + css + "\n  ]]></style>\n";
    if (!defsMarkup.empty()) s += "  <defs>" + defsMarkup + "</defs>\n";
    for (const auto& shape : shapes) s += "  " + shape->toSVG() + "\n";
    return s + "</svg>";
  }

  void save(const std::string& filename) const {
    std::ofstream f(filename);
    if (!f) throw std::runtime_error("Svg::save: cannot open \"" + filename + "\"");
    f << toString();
    f.flush();
    if (!f) throw std::runtime_error("Svg::save: error writing \"" + filename + "\"");
  }
};

} // namespace SVG

#endif // SVG_TEC_HPP
