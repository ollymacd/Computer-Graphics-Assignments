/* CSCI 4471 A02
	Olly MacDonald
	A00459039
	Implementing procedural terrain generation with
		Hybrid multifractal fBm
	Based on code provided by Dr. Jiju Poovvancheri
*/

#include "utility.h"
#include "Skybox.h"
#include "Camera.h"
#include "Terrain.h"
#include "Water.h"

using namespace OpenGP;
const int width = 1280, height = 720;

int main(int, char**) {

	Application app;

	// Make skybox seamless
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	// enable depth test (z-buffer)
	glEnable(GL_DEPTH_TEST);

	//Initial parameters (grid size, water height etc.)
	float waterHeight = 0.1f;
	Vec3 skyColor = Vec3(0.6, 0.7, 0.8);
	Vec3 lightPos = Vec3(30.0f, 30.0f, 30.0f);
	float size_grid_x = 20, size_grid_y = 20;

	//Objects instantiation, Skybox, Camera, Terrain and Water	
	Skybox skybox(skyColor);
	Camera camera(width, height);
	Terrain terrain(size_grid_x, size_grid_y, waterHeight, lightPos);
	Water water(size_grid_x, size_grid_y, waterHeight);


	// clipping plane for shading, reflection and refraction FBO
	Vec3 clipPlaneNormal = Vec3(0, 0, -1);
	float clipPlaneHeight = 100;
	Vec3 reflectionClipPlaneNormal = Vec3(0, 0, 1);
	float reflectionClipPlaneHeight = -waterHeight;
	Vec3 refractionClipPlaneNormal = Vec3(0, 0, -1);
	float refractionClipPlaneHeight = waterHeight;


	// Display callback
	Window& window = app.create_window([&](Window&) {
		float time = glfwGetTime();

		glClearColor(0.6f, 0.8f, 0.9f, 1.0f);
		glEnable(GL_CLIP_DISTANCE0);

		//Reflection texture. Everything above the waterHeight will be rendered
		// for reflection, we draw the whole scene in the FBO from a camera that is positioned below the current eye position and pointing upwards
		// essentially we get angle of incidence = angle of reflection
		// to get the reflection, the camera needs to move down and point upwards => move by current distance * 2 down and flip
		// this draws the reflection into the texture		
		float distance = 2 * (camera.cameraPos.z() - waterHeight);
		camera.cameraPos.z() -= distance;
		camera.invertPitch();
		water.reflectionFBO->bind();
		glViewport(0, 0, water.reflectionWidth, water.reflectionHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		terrain.draw(camera, reflectionClipPlaneNormal, reflectionClipPlaneHeight);
		water.reflectionFBO->unbind();
		camera.cameraPos.z() += distance;
		camera.invertPitch();

		//Refraction texture. Everything under the waterHeight will be rendered
		// for refraction, we draw the scene below the water height => we will blend this with 
		// the reflection texture to create a water effect
		water.refractionFBO->bind();
		glViewport(0, 0, water.refractionWidth, water.refractionHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		terrain.draw(camera, refractionClipPlaneNormal, refractionClipPlaneHeight);
		water.refractionFBO->unbind();

		// actual drawing of sky, terrain and water meshes
		glDisable(GL_CLIP_DISTANCE0);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		terrain.draw(camera, clipPlaneNormal, clipPlaneHeight);
		water.draw(camera, time);
		skybox.draw(camera, time);

		});
	window.set_title("A02 Output");
	window.set_size(width, height);


	//Camera control via mouse
	Vec2 mouse(0, 0);
	window.add_listener<MouseMoveEvent>([&](const MouseMoveEvent& m) {
		Vec2 changeInMousePosition = m.position - mouse;
		camera.updateCameraAngles(changeInMousePosition);
		mouse = m.position;
		});

	//Camera control via keyboard
	window.add_listener<KeyEvent>([&](const KeyEvent& k) {
		camera.updateCamera(k);
		});

	return app.run();
}