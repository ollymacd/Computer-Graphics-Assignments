#ifndef TERRAIN_H
#define TERRAIN_H

#include "utility.h"
#include "Camera.h"

const char* terrain_vshader =
#include "terrain_vshader.glsl"
;

const char* terrain_fshader =
#include "terrain_fshader.glsl"
;

class Terrain {
public:
	//Shader, Mesh and Texture pointers
	std::unique_ptr<Shader> terrainShader;
	std::unique_ptr<GPUMesh> terrainMesh;
	std::unique_ptr<RGBA8Texture> sandTexture;
	std::unique_ptr<RGBA8Texture> grassTexture;
	std::unique_ptr<RGBA8Texture> snowTexture;
	std::unique_ptr<RGBA8Texture> rockTexture;
	float waterHeight;

	// the model matrix is always an identity, we displace manually
	Mat4x4 M = Mat4x4::Identity();

	Vec3  lightPos;

	//Constructor
	Terrain(float size_grid_x, float size_grid_y, float _waterHeight, Vec3 _lightPos)
		: waterHeight(_waterHeight), lightPos(_lightPos) {
		terrainShader = std::unique_ptr<Shader>(new Shader());
		terrainShader->verbose = true;
		terrainShader->add_vshader_from_source(terrain_vshader);
		terrainShader->add_fshader_from_source(terrain_fshader);
		terrainShader->link();


		//Terrain texture loading and binding
		loadTexture(sandTexture, "sand.png");
		sandTexture->bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		loadTexture(grassTexture, "grass.png");
		grassTexture->bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		loadTexture(snowTexture, "snow.png");
		snowTexture->bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		loadTexture(rockTexture, "rock.png");
		rockTexture->bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		/// Terrain mesh generation. Create grid mesh vertices and triangle indices.
		terrainMesh = std::unique_ptr<GPUMesh>(new GPUMesh());
		// Resolution for grid
		int n_width = 512;
		int n_height = 512;

		std::vector<Vec3> points;
		std::vector<unsigned int> indices;

		for (int j = 0; j < n_width; ++j) {
			for (int i = 0; i < n_height; ++i) {
				// Create verts
				float vertX = -size_grid_x / 2 + j / (float)n_width * size_grid_x;
				float vertY = -size_grid_y / 2 + i / (float)n_height * size_grid_y;
				float vertZ = 0.0f;
				points.push_back(Vec3(vertX, vertY, vertZ));
			}
		}

		// Build grid of tris with GL_TRIANGLE_STRIP and GL_PRIMITIVE_RESTART
		for (int j = 0; j < n_width - 1; ++j) {
			float baseX = j * n_width;
			indices.push_back(baseX);
			float baseY = ((j + 1) * n_height);
			indices.push_back(baseY);

			for (int i = 1; i < n_height; ++i) {
				float tempX = i + j * n_width;
				indices.push_back(tempX);
				float tempY = i + (j + 1) * n_height;
				indices.push_back(tempY);
			}

			// A new strip will begin when this index is reached
			indices.push_back(indexRestart);
		}

		terrainMesh->set_vbo<Vec3>("vposition", points);
		terrainMesh->set_triangles(indices);
	}

	void draw(Camera camera, Vec3 clipPlaneNormal, float clipPlaneHeight) {
		terrainShader->bind();

		terrainShader->set_uniform("M", M);
		terrainShader->set_uniform("V", camera.viewMatrix());
		terrainShader->set_uniform("P", camera.projectionMatrix());
		terrainShader->set_uniform("viewPos", camera.cameraPos);

		// set clipping plane for the reflection and refraction
		terrainShader->set_uniform("clipPlaneNormal", clipPlaneNormal);
		terrainShader->set_uniform("clipPlaneHeight", clipPlaneHeight);
		terrainShader->set_uniform("waterHeight", waterHeight);
		terrainShader->set_uniform("lightPos", lightPos);

		glActiveTexture(GL_TEXTURE0);
		sandTexture->bind();
		terrainShader->set_uniform("sand", 0);

		glActiveTexture(GL_TEXTURE1);
		grassTexture->bind();
		terrainShader->set_uniform("grass", 1);

		glActiveTexture(GL_TEXTURE2);
		snowTexture->bind();
		terrainShader->set_uniform("snow", 2);

		glActiveTexture(GL_TEXTURE3);
		rockTexture->bind();
		terrainShader->set_uniform("rock", 3);

		// Draw terrain using triangle strips
		terrainMesh->set_attributes(*terrainShader);
		terrainMesh->set_mode(GL_TRIANGLE_STRIP);
		glEnable(GL_PRIMITIVE_RESTART);
		glPrimitiveRestartIndex(indexRestart);
		terrainMesh->draw();

		terrainShader->unbind();
	}
};

#endif