Small sample/test application for creating a diffuse texture from kinect-rgb image. 

- Reading the 3D Mesh from an .obj file

- Using the Kinect API to map from 3D to 2D-Color coordinates 
-- saves the created mapping for testing without the need of a real Kinect

- Creates a texture by rasterizing UV-triangles (corresponding to the faces of the mesh) into the result texture 
-- buggy! (real project uses OpenGL for creating the texture)


