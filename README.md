# Momentous

This is a reimplementation of the particle system from "fr-059: momentum",
using D3D10 GPU hardware.

I was thinking about this a few days back; the original version was using several
hacks to get good performance on shader level 2 hardware, and the actual particle
system ran on the CPU.

Well, the particle system is actually really easy to run on the GPU provided there
is support for floating point render targets and filtering of floating point volume
textures. That used to be a problem in 2007 but not today.

Also, having actual HW instancing support instead of having to get by with shader
instancing sure makes things easier.

Tricks from the original implementation that made it into this version:

* Hard-edged cubes specified using 8 vertices only (to reduce VS load). The original
  version determined the (faceted) normal in the pixel shader using a cube map lookup
  from an interpolated object-space position (passed as an attribute). This was the
  easiest way to do this on shader level 2 hardware, but it did tend to cause some
  sparkling near the edges (exactly which cube map face a near-edge sample landed on
  depended on interpolation rounding, which was a bit dicey).

  This version passes the world-space position as an attribute and then determines the
  face normal as the cross product of the position's derivatives. This is simpler, has
  no texture lookup, and does not have any sparkling. (But it does require derivative
  instructions).
* Instance all the things!
* Trilinear interpolation using smoothstepped weights: much cheaper than spline
  interpolation and looks almost as good.

Bonus: this version also renders each cube as an indexed triangle strip (14 verts plus
primitive restart) - again, the original version was written before primitive restart
was a feature you could rely on. This should not make any significant difference
compared to explicit quads, but it *feels* nicer. :)

This uses D3D10 because I'm writing this on a Laptop with Intel integrated graphics and
drivers that haven't been updated for over 2 years; was I feeling lucky enough to try
GL 3? Evidently not.

Oh and I really need to add some camera control and shadow mapping, like the original
version had.

-Fabian 'ryg' Giesen,
 December 2013