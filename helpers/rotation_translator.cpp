// Translate flip flags to rotation degrees.

#include <stdbool.h>
#include <stdio.h>

int get_rotation_degrees(bool flip_h, bool flip_v, bool flip_d) {
    if (!flip_h && !flip_v && !flip_d) return 0;     // No flip = 0°
    if (flip_h && flip_v && !flip_d) return 180;     // H + V = 180°
    if (!flip_h && flip_v && flip_d) return 90;      // V + D = 90° CCW
    if (flip_h && !flip_v && flip_d) return 90;      // H + D = 90° CW (visually same in Tiled)
    if (flip_h && flip_v && flip_d) return 270;      // H + V + D = 270°

    // The remaining flip_d only or flip_d + something is a mirror, not pure rotation
    return -1; // Not a pure rotation (e.g., mirrored or skewed)
}
