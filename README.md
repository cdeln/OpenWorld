# OpenWorld: A graphics project
This is a graphics project for the course [Computer Graphics](https://www.computer-graphics.se/TSBK07.html)
held by [Ingemar Ragnemalm](https://www.lysator.liu.se/~ingemar/) at Linköping University, Sweden during 2017.
The project aimed at generating *open worlds*, which was not completely successful,
but many basic and intermediate graphics features were implemented.
Read the [paper](doc/ComputerGraphicsProjectReport.pdf) for an elaborate report of the project.
OpenGL context is provided by [MicroGlut](http://ragnemalm.se/lightweight/aboutmicroglut.html).

## Dependencies
* OpenGL (version >= 4.2)
* OpenCV (2 or 3)
* curl
* glm (if not installed on system it is available as a submodule)
* fftw3 (downloaded in make, or can be installed on system)

## Installation
* On debian
	- `apt-get install libgl1-mesa-dev libxt-dev libopencv-dev curl`
* `git submodule init`
* `git submodule update` (wait for clone to finish)
* `make` (will download fftw3 using curl if not installed on system).

*Make will not try to install the dependencies under system directories, they will only be installed locally in the project folder.*

## Project structure
* Source files are located under src
* Deprecated test files are located under src/slask and src/unsupported
* Common library functions are located under src/lib
* GLUT is located under common (Ingemar Ragnemalm version)
* Other external dependencies are located under ext
* Shaders are located in shaders
* Deprecated test shaders are located in shaders/slask and shaders/unsupported
* Scripts are located under scripts
* Documentation is located under doc
* Textures are located under data/textures
* Recorded data are located under data/record
* Build files are located under build

## Running the project
* Run the project by `./run`
* Settings are located in the beginning of main source file ([src/open_world_final.cpp](src/open_world_final.cpp))

There are various controls implemented, all listed below
* 1 - Decrease shadow map margin
* 2 - Increase shadow map margin
* 3 - Decrease shadow map filter kernel size, minimum 0
* 4 - Increase shadow map filter kernel size, unbounded so can be very slow for big sizes
* 5 - Decrease water alpha (reflectivity), minimum 0
* 6 - Increate water alpha ( reflectivity), maximum 1
* 7 - Decrease geomipmap base resolution, minimum 0
* 8 - Increase geomipmap base resolution, unbounded so can be slow for big values
* 9 - Decrease current time by 6 minutes
* 0 - Increase current time by 8 minutes
* w,a,s,d  + Mouse - Classic move controls
* f - Toggle fullscreen
* q - Quit
* z - Toggle wart pointer
* c - Toggle camera fly mode
* Space - Move fast
* Return/Enter - Take picture (data/record/)
* m - Toggle shadows
* m - Toggle shadow map debug mode (render shadow map)
* p - Toggle particles
* r - Toggle Rayleigh scattering
* t - Toggle Mie scattering
* y - Toggle clouds
* u - Toggle cloud perspective correction
* i - Toggle cloud horizon fog
* g - Toggle gloss mapping
* n - Toggle normal mapping
* v - Toggle tangent basis visualization
* l - Toggle blinn-phong vs phong specular shading

## Known bugs
* Changing the time while particles being rendered gives rise to weird effects. Toggling particles after changing time resolved the problem.
* The calculation of barycentric coordinates for moving camera in ground mode has some bug when moving over triangle borders.

## External resources
Textures have been used from [textures.com](https://www.textures.com), all credits for the textures goes there.
