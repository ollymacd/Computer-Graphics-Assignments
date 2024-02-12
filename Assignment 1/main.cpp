/* CSCI 4471 A01
	Olly MacDonald
	A00459039
	Implementing backward ray tracing with
		Shadow rays
		Supersampling
	Based on code provided by Dr. Jiju Poovvancheri
*/

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include "Triangle.h"
#include "Image.h"
#include "Texture.h"
#include<cstdlib>
#include<list>
#include <iostream>
#include <fstream>
using namespace std;

//Data types
typedef Eigen::Matrix<float, 3, 1> Vec3;
typedef Eigen::Matrix<float, 2, 1> Vec2;

//Color functions
using Colour = cv::Vec3b; // BGR Value: Type aliasing (Colour is just an alias for cv:Vec3b type)
Colour white() { return Colour(255, 255, 255); }
Colour black() { return Colour(0, 0, 0); }

// Bounding the channel wise pixel color between 0 to 255
unsigned char Clamp(int pixelCol)
{
	if (pixelCol < 0) return 0;
	if (pixelCol >= 255) return 255;
	return pixelCol;
}

int main(int, char**) {
	// Definition of image
	Image image = Image(512, 512);
	Vec3 llc = Vec3(-1.0f, -1.0f, -1.0f);
	Vec3 urc = Vec3(1.0f, 1.0f, -1.0f);
	int width = urc(0.0f) - llc(0.0f);
	int height = urc(1) - llc(1);
	Vec2 pixelUV = Vec2((float)width / image.cols, (float)height / image.rows);

	// Definition of camera and light
	Vec3 cameraPoint = Vec3(0.0f, 0.0f, 0.0f);
	Vec3 lightSource = Vec3(1.0f, -0.25f, -0.25f);
	Vec3 ambLight = Vec3(1.0f, 0.2f, 0.5f);
	Vec3 diffLight = Vec3(1.0f, 1.0f, 1.0f);
	Vec3 specLight = Vec3(1.0f, 1.0f, 1.0f);

	// Definition of sphere
	Vec3 spherePos = Vec3(0.0, 0.0, -2.5);
	float sphereRadius = 1.0;
	Vec3 objectColor = Vec3(0.0f, 0.5f, 1.0f);
	// Reflection coefficients of sphere
	Vec3 k_a = Vec3(0.5f, 0.5f, 0.5f);
	Vec3 k_d = Vec3(0.3f, 0.8f, 1.0f);
	Vec3 k_s = Vec3(1.0f, 1.0f, 1.0f);

	// Definition of triangles for planes
	list<Triangle>  tri_list{
		Triangle(Vec3(-10, -1.5, -1.5), Vec3(10, -1.5, -1.5), Vec3(-10, -1.5, -10)),
		Triangle(Vec3(10, -1.5, -1.5), Vec3(10, -1.5, -10), Vec3(-10, -1.5, -10)),
		Triangle(Vec3(-10, -1.5, -10), Vec3(10, -1.5, -10), Vec3(-10, 10, -10)),
		Triangle(Vec3(10, -1.5, -10), Vec3(10, 11, -10), Vec3(-10, 10, -10)) };

	//Texture
	//Texture tex = Texture();
	//tex.loadTexture("Textures/rock.png");

	/// Reflection coefficients of tris
	Vec3 triColor = Vec3(0.3f, 0.0f, 0.4f);
	Vec3 tk_a = Vec3(0.1f, 0.3f, 0.8f);
	Vec3 tk_d = Vec3(0.8f, 0.8f, 0.8f);
	Vec3 tk_s = Vec3(1.0f, 1.0f, 1.0f);

	// Declaring structures used within loop
	Vec3 Origin = cameraPoint;
	Vec3 pixelPos, direction, dir, offset, normal, intersection, lightVector, halfVector;
	float A, B, C, discriminant, t1, t2, t, diffuseTerm, specTerm;
	// Declaring structures used within loops: floor
	float u, v, w;
	float epsilon = 0.00001;
	Vec3 Norm, Intersection, baryCoords;
	// Declaring structures for supersampling sphere
	int h_samples = 8;
	int v_samples = 8;
	float h_sp;
	float v_sp;

	//ofstream f;
	//f.open("test.txt");

	// Loop over entire image
	for (int i = 0; i < image.rows; ++i) {
		for (int j = 0; j < image.cols; ++j) {
			// If no intersection, colour white
			Colour pixelColor = Colour(255, 255, 255);

			// Define pixel being coloured and the ray direction
			u = llc(0) + pixelUV(0) * (j + 0.5);
			v = llc(1) + 2 - pixelUV(1) * (i + 0.5);
			w = -1;
			pixelPos = Vec3(u, v, w);
			direction = pixelPos - Origin;
			direction.normalize();

			// Loop over triangles
			for (auto it = tri_list.begin(); it != tri_list.end(); ++it) {

				//Ray-plane intersection
				Vec3 tNormal = it->normal_;
				tNormal.normalize();
				Scalar t = (float)(it->vertex1_ - Origin).dot(tNormal) / direction.dot(tNormal);

				//Ray-triangle intersection
				Intersection = Origin + t * direction;
				baryCoords = it->BaryCentric(Intersection);
				if (fabs(baryCoords.sum() - 1) < epsilon && (baryCoords[0] <= 1 && baryCoords[0] >= 0) &&
					(baryCoords[1] <= 1 && baryCoords[1] >= 0) && (baryCoords[2] <= 1 && baryCoords[2] >= 0)) {

					// Generate shadow ray
					dir = lightSource - Intersection;
					dir.normalize();
					offset = Intersection + 0.0001 * dir;

					// Ray-sphere intersection
					A = dir.dot(dir);
					B = 2 * (dir.dot(offset - spherePos));
					C = (offset - spherePos).dot(offset - spherePos) - (sphereRadius * sphereRadius);
					discriminant = (B * B) - (4 * (A * C));

					if (discriminant > 0) {
						// If shadow ray hits object->ambient only
						// f << "Pixel " << i << " " << j << "YES in shadow \n";
						pixelColor[0] = Clamp(tk_a[0] * ambLight[0] * triColor[0] * 255);
						pixelColor[1] = Clamp(tk_a[1] * ambLight[1] * triColor[1] * 255);
						pixelColor[2] = Clamp(tk_a[2] * ambLight[2] * triColor[2] * 255);
					}
					else {
						// else, floor is illuminated
						// f << "Pixel " << i << " " << j << "NOT in shadow \n";
						lightVector = lightSource - Intersection;
						lightVector.normalize();
						diffuseTerm = lightVector.dot(tNormal);
						if (diffuseTerm < 0) diffuseTerm = 0;
						halfVector = lightVector + (-direction);
						halfVector.normalize();
						specTerm = halfVector.dot(tNormal);

						pixelColor[0] = Clamp(((tk_a[0] * ambLight[0] + tk_d[0] * diffLight[0] * diffuseTerm + tk_s[0] * specLight[0] * pow(specTerm, 1000)) * triColor[0]) * 255);
						pixelColor[1] = Clamp(((tk_a[1] * ambLight[1] + tk_d[1] * diffLight[1] * diffuseTerm + tk_s[1] * specLight[1] * pow(specTerm, 1000)) * triColor[1]) * 255);
						pixelColor[2] = Clamp(((tk_a[2] * ambLight[2] + tk_d[2] * diffLight[2] * diffuseTerm + tk_s[2] * specLight[2] * pow(specTerm, 1000)) * triColor[2]) * 255);
					}
					image(i, j) = pixelColor;
				}
			}
			// Loops for supersampling sphere
			for (int p = 0; p < h_samples; ++p) {
				for (int q = 0; q < v_samples; ++q) {
					// Define sampling position and generate ray
					h_sp = j + (p + ((float)rand() / RAND_MAX)) / h_samples;
					v_sp = i + (q + ((float)rand() / RAND_MAX)) / v_samples;
					pixelPos = Vec3(llc(0) + pixelUV(0) * h_sp, llc(1) + pixelUV(1) * v_sp, -1);
					direction = pixelPos - Origin;
					direction.normalize();

					// Ray-sphere intersection
					A = 1;
					B = 2 * (direction.dot(Origin - spherePos));
					C = (Origin - spherePos).dot(Origin - spherePos) - (sphereRadius * sphereRadius);
					discriminant = (B * B) - (4 * (A * C));

					if (discriminant > 0) {
						// Interesction found. Find intersection point closer to ray origin
						t1 = (-B - sqrt(discriminant)) / (2 * A);
						t2 = (-B + sqrt(discriminant)) / (2 * A);
						t = std::min(t1, t2);

						if (t > 0) {
							// Closest intersection not within camera. Illuminate pixel.
							intersection = Origin + direction * t;
							normal = intersection - spherePos;
							normal.normalize();
							lightVector = lightSource - intersection;
							lightVector.normalize();
							diffuseTerm = lightVector.dot(normal);
							if (diffuseTerm < 0) diffuseTerm = 0;
							halfVector = lightVector + (-direction);
							halfVector.normalize();
							specTerm = halfVector.dot(normal);


							Colour colour(0, 0, 0);
							colour[0] = Clamp(((k_a[0] * ambLight[0] + k_d[0] * diffLight[0] * diffuseTerm + k_s[0] * specLight[0] * pow(specTerm, 64)) * objectColor[0]) * 255);
							colour[1] = Clamp(((k_a[1] * ambLight[1] + k_d[1] * diffLight[1] * diffuseTerm + k_s[1] * specLight[1] * pow(specTerm, 64)) * objectColor[1]) * 255);
							colour[2] = Clamp(((k_a[2] * ambLight[2] + k_d[2] * diffLight[2] * diffuseTerm + k_s[2] * specLight[2] * pow(specTerm, 64)) * objectColor[2]) * 255);
							image(i, j) = colour;
						}
						else {
							// Camera is within sphere
							image(i, j) = black();
						}
					}
				}
			}
		}
	}
	//f.close();
	image.save("./result2.png");
	image.display();

	return EXIT_SUCCESS;
}

