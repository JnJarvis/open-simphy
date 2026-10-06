# Minimal 2D math

Implements accepted math v1 using only the standard library. Include
opensim/math/math.hpp and link opensim::math. Vec2 is binary64, x right/y up;
angles are counterclockwise radians. Raw algebra can overflow and is not validation.

normalized returns no value for zero, nonfinite values or an unrepresentable norm.
near requires explicit finite tolerances, with absolute >=0 and relative in [0,1].
It follows the accepted max-bound formula, including rejection when subtraction
overflows. No implicit epsilon or unit conversion is introduced.

Transform2::create rejects nonfinite values; identity is explicit. point applies
rotation and translation; direction applies rotation only. Both return optional
values and reject nonfinite results. compose(A,B) applies B first, then A.
inverse is checked. Stored angles are not wrapped. No graphics or platform types,
mutable caches or third-party dependencies are present.
