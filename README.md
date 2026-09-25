# svg-tec (C++)

A small, header-only C++17 library for generating SVG graphics.

This is an educational project, built for learning purposes. Each shape owns its own style (fill, stroke, transform) instead of relying on a mutable drawing context; the fluent, chainable method calls (`.fill().stroke()...`) are inspired by [SVG.js](https://svgjs.dev/).

![example output](example.svg)

## Features

- Shapes: `Rectangle`, `Circle`, `Ellipse`, `Line`, `Polyline`, `Polygon`, `Wedge`, `QuadCurve`, `CubicCurve`, `Text`, `Group`
- Curves (`QuadCurve`, `CubicCurve`) and `Wedge` are sampled into `<polyline>`/`<polygon>` points, no `<path>` element involved
- Every shape owns its own style (fill, stroke, transform) instead of relying on a mutable drawing context
- Fluent, chainable API: `doc.rect(10,20,30,40).fill("#ff0000").stroke("#000000").stroke(2.0);`
- `Group` and `Svg` share the same shape-creation methods (both inherit from `ShapeContainer`), so nesting groups works the same way as the top-level document
- Colors from RGB ints, hex strings (`"#ff0000"`, `"0xff0000"`, `"ff0000"`), or `Color::hex(0xff0000)`
- `translate`, `rotate`, `scale`, `skewX`, `skewY` compose directly onto a shape's transform
- `doc.save("file.svg")` writes the document to disk, no file handling required
- **CSS animations and interaction** baked into the same fluent API: `@keyframes`, transitions, `:hover`/`:active`/`:focus` pseudoclasses, and per-shape CSS declarations. No JavaScript involved; the output is a single self-contained SVG file

## Requirements

- A C++17 compiler (no other dependencies, header-only)

## Usage

Copy `svg_tec.hpp` into your project and include it:

```cpp
#include "svg_tec.hpp"

using namespace SVG;

int main() {
  Svg doc(800, 600);

  doc.rect(50, 50, 150, 100).fill("#ff0000").stroke("#000000").stroke(2.0);
  doc.circle(300, 100, 50).fill("#0000ff");
  doc.line(50, 200, 750, 200).stroke("0xffa500").stroke(3.0).cap(Svg::ROUND);
  doc.text(50, 250, "Hello SVG").font("Arial", 24).fill("#323232");

  auto& g = doc.group();
  g.translate(100, 400).rotate(45);
  g.rect(0, 0, 100, 80, 10).fill("#800080");

  doc.save("output.svg");
}
```

See [`example.cpp`](example.cpp) for a complete demonstration of every shape and feature. Build and run it with:

```
g++ -std=c++17 -o example example.cpp
./example
```

### Animated and interactive example

```cpp
#include "svg_tec.hpp"

using namespace SVG;

int main() {
  Svg doc(220, 220);

  // @keyframes are registered once and reused by any element.
  doc.keyframes("latido", {
    "0%   { transform: scale(1);    }",
    "50%  { transform: scale(1.15); }",
    "100% { transform: scale(1);    }"
  });
  doc.keyframes("flotar", {
    "0%   { transform: translateY(0);    }",
    "50%  { transform: translateY(-6px); }",
    "100% { transform: translateY(0);    }"
  });

  // Selector-based interaction (no JavaScript).
  doc.transition(".boton", "fill, transform", 0.3);
  doc.onHover (".boton", "fill: #2ecc71; transform: translateY(-3px); cursor: pointer;");
  doc.onActive(".boton", "fill: #27ae60;");

  // A shape that pulses, anchored at its own center.
  doc.circle(110, 90, 40)
     .fill("#e74c3c")
     .className("latiendo")
     .animate("latido", 1.2, "ease-in-out", "infinite")
     .cssTransformOrigin(110, 90);

  // A rectangle that reacts to hover/active.
  doc.rect(70, 160, 80, 25, 6)
     .fill("#3498db")
     .className("boton");

  // A title that floats gently, with a small delay.
  doc.text(110, 45, "SVG + CSS")
     .font("sans-serif", 14)
     .fill("#ffffff")
     .className("titulo")
     .animate("flotar", 2.0, "ease-in-out", "infinite", 0.2);

  doc.save("interactivo.svg");
}
```

The generated SVG contains a single `<style>` block with the `@keyframes`, transitions and pseudoclass rules, followed by the shapes. Everything is CSS: no `<script>`, no SMIL, no external assets.

## API overview

### Document and groups

`Svg` and `Group` both inherit from `ShapeContainer`, so they share the same shape-creation methods below. Each one creates the shape, attaches it to the container, and returns a reference to it so you can keep chaining style calls.

| Method | Description |
|---|---|
| `Svg(width, height)` | Creates a document with the given canvas size |
| `.rect(x, y, w, h, rx=0)` | Adds a `Rectangle`, optionally rounded |
| `.circle(cx, cy, r)` | Adds a `Circle` |
| `.ellipse(cx, cy, rx, ry)` | Adds an `Ellipse` |
| `.line(x1, y1, x2, y2)` | Adds a `Line` |
| `.polyline(points)` | Adds a `Polyline` from a vector of `{x,y}` points |
| `.polygon(points)` | Adds a closed `Polygon` from a vector of `{x,y}` points |
| `.wedge(x, y, r1, r2, a1, a2)` | Adds a `Wedge`: the annular sector centered at `(x,y)` between radii `r1` and `r2`, swept from angle `a1` to `a2` (degrees, 0° = +x axis, increasing clockwise). Pass `r1 = 0` for a plain pie slice |
| `.quadCurve(x1, y1, cx, cy, x2, y2)` | Adds a quadratic Bezier curve, sampled into a `Polyline` |
| `.cubicCurve(x1, y1, cx1, cy1, cx2, cy2, x2, y2)` | Adds a cubic Bezier curve, sampled into a `Polyline` |
| `.text(x, y, content)` | Adds a `Text` element |
| `.group()` | Adds a nested `Group`, itself a `ShapeContainer` |
| `.add(shape)` | Attaches an already-created `shared_ptr<Shape>` |
| `.save(filename)` | Writes the SVG document to a file |
| `.toString()` | Returns the SVG document as a string |

### Shape styling (available on every shape)

| Method | Description |
|---|---|
| `.fill(color)` / `.noFill()` | Sets or disables the fill |
| `.stroke(color)` / `.stroke(width, dash="")` / `.noStroke()` | Sets stroke color, stroke width/dash array, or disables the stroke |
| `.cap(Svg::BUTT \| Svg::ROUND \| Svg::SQUARE)` | Sets `stroke-linecap` |
| `.transform(t)` / `.translate(x,y)` / `.rotate(a,cx,cy)` / `.scale(sx,sy)` / `.skewX(a)` / `.skewY(a)` | Sets or composes the shape's transform |

### CSS: identity and per-shape declarations

Every shape can carry a CSS class and/or id, and can declare CSS properties that are collected into the document's `<style>` block.

| Method | Description |
|---|---|
| `.className("foo")` | Sets the `class` attribute (and enables selector-based rules elsewhere) |
| `.idName("bar")` | Sets the `id` attribute |
| `.css(declarations)` | Appends raw CSS declarations to the shape's class. Auto-generates a class name if none is set |
| `.cssOpacity(o)` | Shorthand for `opacity: o;` |
| `.transition(properties, seconds, timing="ease", delay=0)` | Shorthand for `transition: <properties> <s>s <timing>[ <delay>s];` |
| `.cssTransform(expr)` | Shorthand for `transform: <expr>;` (CSS transform, not the SVG attribute) |
| `.cssTransformOrigin(x, y)` | Sets `transform-origin: <x>px <y>px;` — useful for CSS animations on SVG shapes |
| `.animate(keyframeName, duration, timing="ease", iteration="infinite", delay=0, direction="", fillMode="")` | Shorthand for the `animation:` property, referencing a keyframe registered with `keyframes()` |
| `.animateTransform(keyframeName, duration, timing="ease", iteration="infinite")` | Alias of `.animate()` kept for symmetry |

### CSS: document-level helpers

Available on `Svg` and `Group` through `ShapeContainer`.

| Method | Description |
|---|---|
| `.keyframes(name, {"0% {...}", "50% {...}", "100% {...}"})` | Registers an `@keyframes` block. Duplicated names are ignored |
| `.css(rule)` | Appends a raw CSS rule (escape hatch) |
| `.onHover(selector, decls)` | Emits `<selector>:hover { <decls> }` |
| `.onActive(selector, decls)` | Emits `<selector>:active { <decls> }` |
| `.onFocus(selector, decls)` | Emits `<selector>:focus { <decls> }` |
| `.onVisited(selector, decls)` | Emits `<selector>:visited { <decls> }` |
| `.transition(selector, properties, seconds, timing="ease", delay=0)` | Emits a transition rule on a selector |
| `.animate(selector, keyframeName, duration, timing="ease", iteration="infinite", delay=0)` | Emits an `animation:` rule on a selector |

Static selector helpers on `Shape`:

```cpp
Shape::sel("boton")    // ".boton"
Shape::id("titulo")    // "#titulo"
Shape::hover(".boton") // ".boton:hover"
Shape::active(".boton")
Shape::focus(".boton")
```

### Colors

```cpp
Color(255, 0, 0)
Color("#ff0000")
Color("0xff0000")
Color::hex(0xff0000)
```

An invalid hex string throws `std::invalid_argument`.

## How the CSS integration works

- Each shape accumulates its own CSS declarations in a `pendingRules` list. `Shape::collectStyles()` exposes them, and `ShapeContainer::collectStyles()` walks the tree and merges everything (global rules + per-shape rules) into a single list.
- `Svg::toString()` emits that list inside a single `<style>` element placed before the shapes, so the browser applies the rules from the first paint.
- `@keyframes` blocks are stored separately so a name is only emitted once, even if multiple shapes call `.animate()` with it.
- Per-shape rules live in the shape's `class` attribute. If you call `.animate()` (or `.css()`, `.transition()`, …) without an explicit `.className()`, an internal class name is generated automatically.
- Because SVG presentation attributes have lower priority than CSS, your `.fill("#e74c3c")` still applies unless a CSS rule overrides it — which is exactly what the `:hover`/`:active` rules do.

## Limitations

- CSS-only. No JavaScript, no SMIL, no external stylesheets, no event handlers.
- CSS `transform` and the SVG `transform` attribute are independent. If a shape uses both, CSS wins while the animation runs; use `.cssTransformOrigin()` if you need a specific anchor point (percentage-based `transform-origin` is unreliable across browsers for SVG).
- `transform-origin` is expressed in user units (`px`), not percentages, matching SVG's coordinate system.

## License

MIT — see [LICENSE](LICENSE).