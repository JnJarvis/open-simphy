# SimPHY Optics (Geometrical / Ray Optics)

Evidence: `org/shikhar/simphy/geom/optics/**` (25 classes), `OpticsUtils`,
`ShotData`, `OpticalSettings`; examples `Prism Dispersion.ssim`, `Optics Demo.ssim`.

Optics is a **2D ray-tracing** module built on the same constructive-geometry
(`Shape2D`) framework. Rays are propagated by reflection/refraction at surfaces;
dispersion (wavelength-dependent index) is supported.

## 1. Optical elements (class → editable parameters)
| Element | Class | Parameters |
|---|---|---|
| Point source / Ray | `OpticalSource2D`, `OpticalRay2D`, `OpticalRay2Points2D` | Number of Rays, WaveLength (nm), Draw Rays (nm) |
| Beam (parallel) | `OpticalBeam2D` | Number of Rays, WaveLength (nm), Draw Rays (nm) |
| White-light source | `OpticalWhiteLight2D` | Ray Count (spectrum spread) |
| Real lens | `OpticalRealLens2D` | R1, R2 (surface radii), Thickness, Refractive Index, Show Foci/Axis |
| Ideal (thin) lens | `OpticalIdealLens2D` | Focal Length, Show Foci/Axis/Images |
| Plane mirror | `OpticalPlaneMirror2D` | Show Images |
| Ideal (curved) mirror | `OpticalIdealMirror2D` | Focal Length, Show Foci/Axis/Images |
| Circular arc (mirror/lens surface) | `OpticalArc2D` | Show Center/Axis/Images |
| Parabolic arc (mirror) | `OpticalParabolicArc2D` | Show Focus/Axis/Images |
| Custom reflector (curve) | `OpticalCustomReflector2D` | Expression (curve eqn) |
| Custom refractor (slab/lens) | `OpticalCustomRefractor2D` | Refractive Index, Reflectivity, Dispersive power, Show Images, Upper Curve, Lower Curve |
| Blocker (absorber) | `OpticalBlocker2D` | — |
| Observer / screen | `OpticalObserver2D` | — |
| Path shape | `OpticalPathShape2D` | — |

### OpticalSettings defaults (OBSERVED)
`MAX_RAY_INTERACTIONS=10`, `MIN_INTERSECTIONS_FOR_IMAGE=2`,
`defaultRefractiveIndex=1.5`, `defaultFocalLength_Lens=3.0`,
`defaultFocalLength_Mirror=3.0`, `defaultNumRays_Beam=5`, ray color yellow,
real-image green, virtual-image blue, virtual-ray orange.

Supporting: `OpticalDevice`/`OpticalDevice2D` (base), `OpticalRefractor`,
`OpticalImage`/`OpticalImageWrapper2D` (image construction), `OpticalSettings`
(global: max bounces, ray count, dispersion range), `ShotData` (per-ray state),
`OpticsUtils` (Snell's law, sign conventions).

## 2. Physics implemented
- **Reflection** at planar / spherical / parabolic / arbitrary-curve surfaces.
- **Refraction** via Snell's law with per-surface **refractive index**;
  **reflectivity** (partial reflection) and **dispersive power** (index varies with
  wavelength → prism/rainbow dispersion).
- **Lensmaker** for real lenses (two spherical surfaces + thickness + index);
  thin-lens idealization.
- **Image construction** (real/virtual images, foci, principal axis) toggled per
  device.
- **White light** decomposed across a wavelength band (nm) → spectral spread.
- Rays carry a **wavelength (nm)**; color mapped from wavelength.

## 3. Interaction & persistence
- Created with the Optics toolbar ("Click, move, and click to create optical
  elements"); each element is a `Shape2D` with `Class=Optical*2D`, persisted in
  `<Shapes>` (FILE_FORMAT §8).
- Rays are computed per-frame (`OpticsUtils`), so moving a device updates the
  ray diagram live.
- `OpticalObserver2D` can record where rays land (measurement).

## 4. Example: Prism Dispersion
A white beam enters a custom refractor (prism); dispersion splits the band into
a spectrum; an observer captures the spread. Confirms wavelength-dependent index.

## Cross-refs
GEOMETRY.md (shared Shape2D framework), FILE_FORMAT §8, FEATURES.md (VIS-O###).
