#ifndef INCLUDED_RENDER_UTILS_H
#define INCLUDED_RENDER_UTILS_H

// Renders a textured quad by spliting it up into a regular grid of smaller quads.
// This is needed to prevent fog artifacts on Radeon cards.
void RenderSplitUpQuadTextured(
		float posNorth, float posSouth, float posEast, float posWest, float height,
		float texNorth, float texSouth, float texEast, float texWest, int steps);

// Same as above but two sets of texture coords
void RenderSplitUpQuadMultiTextured(
		float posNorth, float posSouth, float posEast, float posWest, float height,
		float texNorth1, float texSouth1, float texEast1, float texWest1,
		float texNorth2, float texSouth2, float texEast2, float texWest2, int steps);

#endif
