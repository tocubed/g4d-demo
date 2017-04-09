# g4d-demo

This a proof-of-concept demo for rendering 3D cross-sections of 4D geometry 
in real-time using the GPU.

## Demo
![Demo](/images/demo.gif)

Cross-sections of a 3x3x3x3 arrangement of hypercubes. The texture is a dirt 
cube with grass growing on top, resembling the iconic block from Minecraft.

For a good analogy of cross-sectional rendering, imagine slicing a 3D cube with a 2D plane. 
If you slice it parallel to one of its faces, you will get a perfect square as the cross-section. 
However, if you slice it at an arbitrary angle, you might get a triangle, quadrilateral, 
or a hexagon. Similarly, in four-dimensions, if you slice a 4D hypercube with a 3D hyperplane,
you will get various solid 3D shapes for the cross-sections.

As the above arrangement of hybercubes is rotated in 4D space, different 3D sections of 
each hypercube become visible. Additionally, the surface of each slice is textured 
using 2D slices of a 3D volume texture. The texture used in this demo can be seen, 
in flattened (2D) form, [here](/images/hypercube_texture.png).

## Building and Running
When cloning, there are several submodules in `deps/` that must also be cloned. After that, just
```
mkdir build && cd build
cmake .. && make
``` 
and the project should build. The resulting binary must be run from within the `build` directory, as
the shaders and textures are loaded from file and the paths are currently hard-coded.
