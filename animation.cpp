#include "svg_tec.hpp"

int main() {
  SVG::Svg svg(220, 220);

  svg.keyframes("latido", {
    "0%   { transform: scale(1);    }",
    "50%  { transform: scale(1.15); }",
    "100% { transform: scale(1);    }"
  });

  svg.keyframes("flotar", {
    "0%   { transform: translateY(0);    }",
    "50%  { transform: translateY(-6px); }",
    "100% { transform: translateY(0);    }"
  });

  svg.transition(".boton", "fill, transform", 0.3);
  svg.onHover (".boton", "fill: #2ecc71; transform: translateY(-3px); cursor: pointer;");
  svg.onActive(".boton", "fill: #27ae60;");

  svg.circle(110, 90, 40)
    .fill(SVG::Color("#e74c3c"))
    .className("latiendo")
    .animate("latido", 1.2, "ease-in-out", "infinite")
    .cssTransformOrigin(110, 90);

  svg.rect(70, 160, 80, 25, 6)
    .fill(SVG::Color("#3498db"))
    .className("boton");

  svg.text(110, 45, "SVG + CSS")
    .font("sans-serif", 14)
    .fill(SVG::Color("#ffffff"))
    .className("titulo")
    .animate("flotar", 2.0, "ease-in-out", "infinite", 0.2);

  svg.save("interactive.svg");
}