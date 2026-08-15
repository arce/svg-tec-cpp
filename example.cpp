#include "svg_tec.hpp"

using namespace SVG;

int main() {
  Svg doc(800, 600);

  doc.rect(50, 50, 150, 100).fill("#ff0000").stroke("#000000").stroke(2.0);
  doc.circle(300, 100, 50).fill("#0000ff");
  doc.ellipse(500, 100, 70, 40).fill("#00ff00");
  doc.line(50, 200, 750, 200).stroke("0xffa500").stroke(3.0).cap(Svg::ROUND);
  doc.text(50, 250, "Hello SVG Library").font("Arial", 24).fill("#323232");
  doc.polyline({{50, 320}, {90, 280}, {130, 320}, {170, 280}, {210, 320}}).stroke("#000080").stroke(2.0);
  doc.polygon({{300, 280}, {340, 280}, {360, 320}, {320, 350}, {280, 320}}).fill("#ffd700").stroke("#8b4513").stroke(2.0);
  doc.wedge(720, 100, 25, 55, -60, 60).fill("#ff69b4").stroke("#000000").stroke(1.5);
  doc.quadCurve(420, 350, 470, 260, 520, 350).stroke("#dc143c").stroke(2.5);
  doc.cubicCurve(560, 350, 590, 260, 650, 260, 680, 350).stroke("#1e90ff").stroke(2.5);

  auto& g = doc.group();
  g.translate(100, 400).rotate(45);
  g.rect(0, 0, 100, 80, 10).fill("#800080");
  g.circle(50, 40, 30).noFill().stroke("#00ffff").stroke(2.0, "5,5");

  doc.save("example.svg");
  return 0;
}