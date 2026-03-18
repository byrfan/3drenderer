A Model render that takes in various file formats (to be decided)

    `interpret_obj.c` -> will contain structure for serialising the file
        ? global structure system, at least a structure system derived 
        by the obj format. this leads to possible conversion of 3d file
        formats.

# Rendering Performance Checklist 

## 1. Major performance wins
- [ ] Add backface culling
- [ ] Implement near-plane clipping (do not just skip triangles)

## 2. Fill-rate bottleneck checks 
- [ ] Test wireframe rendering (no fill) to confirm pixel cost is the issue
- [ ] Lower screen resolution and compare FPS

## 3. Rasterization efficiency
- [ ] Clamp triangle bounding box to screen before rasterizing
- [ ] Avoid drawing off-screen pixels
- [ ] Optimize triangle fill algorithm
- [ ] Reduce overdraw (avoid drawing the same pixel multiple times)

## 4. Inner loop optimizations
- [ ] Minimize floating-point operations in pixel loops
- [ ] Avoid divisions per pixel (precompute values)
- [ ] Use integer or fixed-point math where possible
- [ ] Remove function calls inside inner loops

## 5. Depth buffer optimization
- [ ] Ensure depth buffer access is cache-friendly (linear memory access)
- [ ] Avoid unnecessary depth checks
- [ ] Render front-to-back where possible (early depth rejection)

## 6. Geometry-level safeguards
- [ ] Skip degenerate (zero-area) triangles
- [ ] Add optional guard for extremely large triangles (debug only)

## 7. General performance improvements
- [ ] Compile with optimizations (`-O2` or `-O3`)
- [ ] Avoid per-frame memory allocations
- [ ] Profile to identify actual hotspots

## 8. Debugging tools (optional but useful)
- [ ] Visualize triangle size (e.g., color by area)
- [ ] Visualize overdraw
- [ ] Log largest triangle per frame

References:
 - https://en.wikipedia.org/wiki/Wavefront_.obj_file
