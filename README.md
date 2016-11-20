Vectorize
============

Polygonizes PNGs via hill-climbing.

Language: C++

Libraries: libpng, GLFW, glew

---

The algorithm proceeds as follows:

Initialize a gene for the polygon image. The gene consists of a set of polygons.
Each polygon consists of a set of vertices and and a colour.

Each iteration:

1. Each iteration, randomly mutate the image (E.g. add a vertex, change a polygons
   colour, etc.). 

2. Calculate the fitness of the new image by taking the sum of the squared differences
   between the polygon image and the source image.
   
3. Select the more fit of the two images and continue.

![cooling-schedules](https://github.com/orglofch/Vectorize/blob/master/example.png)
