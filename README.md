# renderer

Messing about with deferred shading and triangles.

## **Video**

[![](http://img.youtube.com/vi/P54NGWgKg-Q/0.jpg)](https://www.youtube.com/watch?v=P54NGWgKg-Q)

## __Controls:__

| **Input**      | **Action**           |
|----------------|----------------------|
| Mouse movement | Look around          |
| W              | Move forward         |
| S              | Move backward        |
| A              | Move left            |
| D              | Move right           |
| Left shift     | Move down            |
| Spacebar       | Move up              |
| E              | Set light direction  |
| ESC            | Close program        |

## __Dependencies:__

* OpenGL 4.3
* glew
* glfw3
* glm
* c++11 compiler
* cmake
  
## __Building:__

* mkdir build
* cd build
* cmake .. -G "*your platform*"
* cd ..
* cmake --build build --config Release

## __Running:__

* cd bin
* ./main <width> <height> 
