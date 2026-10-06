# SimPHY 2D Constructive Geometry

Evidence: `org/shikhar/simphy/geom/**` (~150 classes), `ShapesManager` (1194L),
`GeometrySettings`, `Shape2D` base, XML `Class=` values across examples.

SimPHY is **not just a physics sandbox**: it embeds a GeoGebra-style **dependent
2D geometry** system. Shapes reference **parents** (other shapes, points, lines,
or the sentinels `X-AXIS`/`Y-AXIS`) and update live when parents move.

## 1. Framework
- Base `Shape2D`: `Id`, `Name`, `Class` (subclass simple name), `Parents`
  (comma list of parent ids or `X-AXIS`/`Y-AXIS`), `Params`, `DrawPattern`,
  `DrawColor`/`FillColor`, `Touchable`, `Visible`, `OffsetPixels`,
  `DisplayInfoFormat` (e.g. `{x}, {y}`), `ShowEqn`, `StrokeWidth`, optional
  `ExtraShapeData` CDATA (per-type).
- `ShapesManager` builds/repaints the dependency graph, handles selection,
  dragging of free objects, and "find intersection"/"critical point" utilities.
- `GeometrySettings` holds display defaults (grid, precision, show-eqn).

## 2. Object catalogue by family (class simple names)

### Points
`FreePoint2D`, `PointExpr2D` (expression-defined), `PointOnCurve2D`,
`PointOnBody2D` (attach to physics body), `PointIntersection2Curves2D`,
`PointProjection2D`, `PointRatio2D` (divide segment), `PointReflectionLine2D`,
`PointRelativePoint2D` (polar/offset from another point), `LatexPoint2D`,
`SvgPoint2D`, `LazyPoint`.

### Lines / Rays / Segments
`Line2D`, `Line2Points2D`, `LineEqn2D` (ax+by+c=0), `LineLineAnglePoint2D`,
`LineNormal2D`, `LinePointAngle2D`, `LinePointParallelLine2D`,
`LinePointPerpendicularLine2D`, `LineTangent2D`, `LineAngleBisector2D`,
`LineAngleBisectorPair2D`, `Ray2D`, `Ray2Points2D`, `RayReflect2PointsCurve2D`,
`Segment2D`, `Segment2Points2D`, `PolyLine2D`, `Triangle2D`, `Polygon2D`.

### Circles & arcs
`Circle2D`, `Circle2Points2D`, `Circle3Points2D`, `CircleArc2D`,
`CircleArc3Points2D`, `CircleArcCenter2Points2D`, `CircleCenterRadius2D`,
`CircleDiameter2D`.

### Conics (full quadric support)
`Conic2D`, `Conic5Points2D`, `ConicEqnShape2D` (general Ax²+Bxy+Cy²+Dx+Ey+F=0),
`ConicLinePointE2D`, `ConicTwoLines2D`, `Conics2D`, `ConicWrapper2D`,
`Ellipse2D`, `Ellipse2FocusPoint2D`, `EllipseCenter2Points2D`,
`Hyperbola2D`, `Hyperbola2FocusPoint2D`, `HyperbolaCenter2Points2D`,
`Parabola2D`, `Parabola2Points2D`, `ParabolaArc2D`, `ParabolaLinePoint2D`.
Conic features: `FociConic2D`, `PointConicCenter`, `PoleConicLine2D`,
`PolarConicPoint2D`, `MidPointChordConicPoint2D`, `Intersection2Conics2D`,
`CommonTangents2Conics2D`, `TangentsConicLine2D`, `TangentsConicPoint2D`,
`NormalsConicLine2D`, `NormalsConicPoint2D`.

### Curves & functions
`Curve2D`, `DynamicCurve2D`, `Function2D`, `FunctionExplicit2D` (y=f(x)),
`ParametricCurve2D`, `PolarCurve2D`, `ExpressionPlotter2D`, `Graph2Points2D`,
`Spline`, `Spline2D`, `Bezier3Points2D`, `Bezier4Points2D`, `Path2D`,
`PathNpoints2D`, `CriticalPointFunction2D`, `OsculatingCircle2D`.

### Vectors
`Vector2D`, `Vector2Points2D`, `VectorDifference2D`, `VectorEqn2D`,
`VectorPointParallelLine2D`, `VectorPointPerpendicularLine2D`, `VectorSum2D`,
`VectorUnit2D`.

### Measurements & tools
`AngleMeasure2D`, `LengthMeasure2D`, `ExpressionMeasure2D`, `PointMeasure2D`,
`Measure2D`, `Ruler2Points2D`, `Protractor3Points2D`, `Slider2D`, `Image2Points2D`,
`Image3Points2D`, `Image4Points2D`, `Label2D`, `Text2Points2D`, `ShapeInfo2D`,
`Bounds2D`, `Transform2D`.

Utilities: `AngleUtils`, `CurveUtils`, `CommonTangents2Conics2D`,
`CriticalPointFunction2D`, `Intersection`, `PaintStroke`, `LazyBrush`,
`SequenceElementArray2D`.

## 3. Behavior
- **Dependency tracking:** moving a parent recomputes children; circles/conics
  re-derive foci/centers/polars.
- **Expression-defined** objects (`PointExpr2D`, `ExpressionMeasure2D`,
  `FunctionExplicit2D`, `Parabola2D`) take math expressions evaluated by exp4j
  (variables x, y, t, and referenced-object properties).
- **Equation display** (`ShowEqn`) prints the analytic equation of lines/circles/
  conics; LaTeX-capable via `LatexPoint2D`/JLaTeXMath.
- **Intersection** utilities find curve-conic, conic-conic, line-curve points.
- **Critical points** find extrema/inflection of functions; `OsculatingCircle2D`
  shows curvature circles.
- Geometry objects can be **touched** (dragged) and can attach to physics bodies
  (`PointOnBody2D`), bridging static geometry and dynamics.

## 4. Persistence
Each shape = `<Shape2D … Class="…">` inside `<World><Shapes>`, plus optional
`<ExtraShapeData>`. See FILE_FORMAT §8.

## Cross-refs
OPTICS.md (optical shapes reuse this), FILE_FORMAT §8, SCRIPTING (Geometry API),
FEATURES.md (OBJ-G###).
