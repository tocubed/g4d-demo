# G4D

G4D is an efficient rendering engine for displaying 3D cross-sections of 
4D geometry, using a unique GPU pipeline for real-time performance. 

G4D is currently closed source, and this repository is only meant to 
showcase G4D's capabilities. This means you will not be able to compile 
or run any binaries from this repository unless G4D is made public. 

## Demo
![Demo](/images/demo.gif)

Cross-sections of a 3x3x3x3 arrangement of hypercubes. The texture is meant to 
reflect the iconic grass block from the popular video game, Minecraft.

In the same manner that slicing a 3D object results in a 2D cross-section, 
G4D slices each 4D object and computes the shape of the corresponding 3D 
cross-section. As the above arrangement of hypercubes is rotated, different 3D
slices of each hypercube become visible. 

Additionally, the surface of each slice is textured by taking 2D slices 
of a 3D volume texture. The texture used in this demo can be seen, 
in flattened form, [here](/images/hypercube_texture.png).
