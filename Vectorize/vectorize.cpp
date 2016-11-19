#define GL_GLEXT_PROTOTYPES

#define GLFW_DLL
#define GLFW_INCLUDE_GLU

#define NOMINMAX
#include <Windows.h>

#include <GL/glew.h>
#include <GLFW\glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <numeric>
#include <random>
#include <string>

#include "image_util.hpp"

using namespace std;

const int kMaxPolygons = 255;

const float kAddPolygonRate = 1.0f / 700;
const float kRemovePolygonRate = 1.0f / 1500;
const float kSwapPolygonRate = 1.0f / 1000;

const float kAlphaMutationRate = 1.0f / 750;
const float kAlphaMutationSigma = 0.02f;
const float kAlphaMin = 30.0f / 255;
const float kAlphaMax = 60.0f / 255;

const float kRedMutationRate = 1.0f / 750;
const float kRedMutationSigma = 0.1f;
const float kRedMin = 0.0f;
const float kRedMax = 1.0f;

const float kGreenMutationRate = 1.0f / 750;
const float kGreenMutationSigma = 0.1f;
const float kGreenMin = 0.0f;
const float kGreenMax = 1.0f;

const float kBlueMutationRate = 1.0f / 750;
const float kBlueMutationSigma = 0.1f;
const float kBlueMin = 0.0f;
const float kBlueMax = 1.0f;

const float kVertexMutationRate = 1.0f / 750;
const float kVertexMutationSigma = 0.1f;
const float kVertexMin = 0.0f;
const float kVertexMax = 1.0f;

const float kAddVertexRate = 1.0f / 1000;
const float kRemoveVertexRate = 1.0f / 1500;
const float kSwapVertexRate = 1.0f / 1000;

struct Colour
{
	float r, g, b, a;
};

struct Vertex
{
	float x, y;
};

struct Poly
{
	Poly() : colour({}), vertex_count(0), vertices(NULL) {}

	~Poly() {
		if (vertices)
			delete[] vertices;
	}

	Colour colour;

	int vertex_count;
	Vertex *vertices;
};

struct GeneImage : Image
{
	GeneImage() : Image(), polygon_count(0), gene(NULL), fitness(numeric_limits<float>::max()) {}

	~GeneImage() {
		if (gene)
			delete[] gene;
	}

	int polygon_count;
	Poly *gene;

	float fitness;
};

struct SimulationState
{
	Image source_image;
	GeneImage gene_image;
};

void error_callback(int error, const char *description) {
	fputs(description, stderr);
}

void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods) {
	SimulationState *state = (SimulationState*)glfwGetWindowUserPointer(window);
	switch (key)
	{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
	}
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
	SimulationState *state = (SimulationState*)glfwGetWindowUserPointer(window);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	SimulationState *state = (SimulationState*)glfwGetWindowUserPointer(window);
}

float BoltzmannProbability(double current_fitness, double new_fitness, float temperature) {
	return temperature <= 0 ? 0 : exp(-(new_fitness - current_fitness) / temperature);
}

double Fitness(const float *buffer1, const float *buffer2, int width, int height, int channels) {
	float sum = 0;
	for (int i = 0; i < width * height * channels; ++i) {
		if (i % 4 != 3) {
			float diff = (buffer1[i] - buffer2[i]);
			sum += diff * diff;
		}
	}
	return sum / (width * height * 3.0);
}

int PixelIndex(int x, int y, int width, int channels) {
	return (y * width + x) * channels;

}

void InitRandomVertex(Vertex &vertex, float center_x, float center_y) {
	vertex.x = center_x + Randf(-0.001f, 0.001f);
	vertex.y = center_y + Randf(-0.001f, 0.001f);
	clamp(&vertex.x, kVertexMin, kVertexMax);
	clamp(&vertex.y, kVertexMin, kVertexMax);
}

void InitRandomPolygon(SimulationState &state, Poly &polygon, int vertices) {
	float center_x = Randf(kVertexMin, kVertexMax);
	float center_y = Randf(kVertexMin, kVertexMax);

	int pixel_x = center_x * state.source_image.width;
	int pixel_y = center_y * state.source_image.height;

	int pixel_index = PixelIndex(pixel_x, pixel_y, state.source_image.width, 
		state.source_image.channels);
	polygon.colour.r = state.source_image.data[pixel_index + 0];
	polygon.colour.g = state.source_image.data[pixel_index + 1];
	polygon.colour.b = state.source_image.data[pixel_index + 2];
	polygon.colour.a = Randf(kAlphaMin, kAlphaMax);

	polygon.vertices = new Vertex[vertices];
	polygon.vertex_count = vertices;
	for (int i = 0; i < polygon.vertex_count; ++i) {
		InitRandomVertex(polygon.vertices[i], center_x, center_y);
	}
}

void CopyVertex(Vertex &v1, Vertex &v2) {
	v1.x = v2.x;
	v1.y = v2.y;
}

void CopyVertices(Vertex *v1, Vertex *v2, int num) {
	for (int i = 0; i < num; ++i) {
		CopyVertex(v1[i], v2[i]);
	}
}

void CopyPolygon(Poly &p1, const Poly &p2) {
	p1.colour.r = p2.colour.r;
	p1.colour.g = p2.colour.g;
	p1.colour.b = p2.colour.b;
	p1.colour.a = p2.colour.a;

	if (p1.vertices)
		delete[] p1.vertices;

	p1.vertex_count = p2.vertex_count;
	p1.vertices = p2.vertices ? new Vertex[p2.vertex_count] : NULL;

	CopyVertices(p1.vertices, p2.vertices, p2.vertex_count);
}

void CopyPolygons(Poly *p1, Poly *p2, int num) {
	for (int i = 0; i < num; ++i) {
		CopyPolygon(p1[i], p2[i]);
	}
}

void CalculateCentroid(Poly &polygon, float &x, float &y) {
	x = 0;
	y = 0;
	for (int i = 0; i < polygon.vertex_count; ++i) {
		x += polygon.vertices[i].x;
		y += polygon.vertices[i].y;
	}
	x /= polygon.vertex_count;
	y /= polygon.vertex_count;
}

void AddVertex(Poly &polygon, int num) {
	Vertex *prev_vertices = polygon.vertices;
	polygon.vertices = new Vertex[polygon.vertex_count + num];

	CopyVertices(polygon.vertices, prev_vertices, polygon.vertex_count);

	float center_x, center_y;
	CalculateCentroid(polygon, center_x, center_y);
	for (int i = 0; i < num; ++i) {
		InitRandomVertex(polygon.vertices[polygon.vertex_count + i], center_x, center_y);
	}

	polygon.vertex_count += num;

	delete[] prev_vertices;
}

void RemoveVertex(Poly &polygon, int num) {
	// TODO(orglofch): fix for num
	Vertex *prev_vertices = polygon.vertices;
	polygon.vertices = new Vertex[polygon.vertex_count - 1];

	int index = rand() % polygon.vertex_count;
	CopyVertices(polygon.vertices, prev_vertices, index);
	CopyVertices(polygon.vertices + index, prev_vertices + index + 1,
		polygon.vertex_count - index - 1);

	polygon.vertex_count -= 1;

	delete[] prev_vertices;
}

void SwapVertex(Poly &polygon) {
	int i1 = rand() % polygon.vertex_count;
	int i2 = rand() % polygon.vertex_count;

	Vertex temp;
	CopyVertex(temp, polygon.vertices[i1]);
	CopyVertex(polygon.vertices[i1], polygon.vertices[i2]);
	CopyVertex(polygon.vertices[i2], temp);
}

void AddPolygon(SimulationState &state, GeneImage *gene_image, int num) {
	Poly *prev_gene = gene_image->gene;
	gene_image->gene = new Poly[gene_image->polygon_count + num];

	CopyPolygons(gene_image->gene, prev_gene, gene_image->polygon_count);

	for (int i = 0; i < num; ++i) {
		InitRandomPolygon(state, gene_image->gene[gene_image->polygon_count + i], 8);
	}

	gene_image->polygon_count += num;

	delete[] prev_gene;
}

void RemovePolygon(GeneImage *gene_image, int num) {
	// TODO(orglofch): fix for num
	Poly *prev_gene = gene_image->gene;
	gene_image->gene = new Poly[gene_image->polygon_count - 1];

	int index = rand() % gene_image->polygon_count;
	CopyPolygons(gene_image->gene, prev_gene, index);
	CopyPolygons(gene_image->gene + index, prev_gene + index + 1, 
		         gene_image->polygon_count - index - 1);

	gene_image->polygon_count -= 1;

	delete[] prev_gene;
}

void SwapPolygon(GeneImage *gene_image) {
	int i1 = rand() % gene_image->polygon_count;
	int i2 = rand() % gene_image->polygon_count;

	Poly temp;
	CopyPolygon(temp, gene_image->gene[i1]);
	CopyPolygon(gene_image->gene[i1], gene_image->gene[i2]);
	CopyPolygon(gene_image->gene[i2], temp);
}

bool ShouldMutate(float rate) {
	float prob = Randf(0, 1);
	return prob <= rate;
}

void MutatePolygon(Poly &polygon, double sigma_modifier, double rate_modifier) {
	if (polygon.vertex_count > 3 && ShouldMutate(kRemoveVertexRate)) {
		RemoveVertex(polygon, 1);
	} else if (polygon.vertex_count < kMaxPolygons && ShouldMutate(kAddVertexRate)) {
		AddVertex(polygon, 1);
	} else if (polygon.vertex_count > 1 && ShouldMutate(kSwapVertexRate)) {
		SwapVertex(polygon);
	} else {
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		default_random_engine generator(t.QuadPart);
		normal_distribution<float> red_dist(0, kRedMutationSigma * sigma_modifier);
		if (ShouldMutate(kRedMutationRate * rate_modifier)) {
			float value = red_dist(generator);
			polygon.colour.r += value;
			clamp(&polygon.colour.r, kRedMin, kRedMax);
		}
		normal_distribution<float> green_dist(0, kGreenMutationSigma * sigma_modifier);
		if (ShouldMutate(kGreenMutationRate * rate_modifier)) {
			float value = green_dist(generator);
			polygon.colour.g += value;
			clamp(&polygon.colour.g, kGreenMin, kGreenMax);
		}
		normal_distribution<float> blue_dist(0, kBlueMutationSigma * sigma_modifier);
		if (ShouldMutate(kBlueMutationRate * rate_modifier)) {
			float value = blue_dist(generator);
			polygon.colour.b += value;
			clamp(&polygon.colour.b, kBlueMin, kBlueMax);
		}
		normal_distribution<float> alpha_dist(0, kAlphaMutationSigma * sigma_modifier);
		if (ShouldMutate(kAlphaMutationRate * rate_modifier)) {
			float value = alpha_dist(generator);
			polygon.colour.a += value;
			clamp(&polygon.colour.a, kAlphaMin, kAlphaMax);
		}
		normal_distribution<float> vertex_dist(0, kVertexMutationSigma * sigma_modifier);
		for (int i = 0; i < polygon.vertex_count; ++i) {
			Vertex &vertex = polygon.vertices[i];
			if (ShouldMutate(kVertexMutationRate * rate_modifier)) {
				float value = vertex_dist(generator);
				vertex.x += value;
				clamp(&vertex.x, kVertexMin, kVertexMax);
			}
			if (ShouldMutate(kVertexMutationRate * rate_modifier)) {
				float value = vertex_dist(generator);
				vertex.y += value;
				clamp(&vertex.y, kVertexMin, kVertexMax);
			}
		}
	}
}

void Mutate(SimulationState &state) {
	if (state.gene_image.polygon_count > 1 && ShouldMutate(kRemovePolygonRate)) {
		RemovePolygon(&state.gene_image, 1);
	} else if (state.gene_image.polygon_count < kMaxPolygons && ShouldMutate(kAddPolygonRate)) {
		AddPolygon(state, &state.gene_image, 1);
	} else if (state.gene_image.polygon_count > 1 && ShouldMutate(kSwapPolygonRate)) {
		SwapPolygon(&state.gene_image);
	} else {
		for (int i = 0; i < state.gene_image.polygon_count; ++i) {
			MutatePolygon(state.gene_image.gene[i], 
				1.0 - state.gene_image.fitness,
				1.0 - state.gene_image.fitness);
		}
	}
}

void RenderPolygon(const Poly &polygon, int width, int height) {
	glColor4f(polygon.colour.r, polygon.colour.g, polygon.colour.b, polygon.colour.a);
	glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i < polygon.vertex_count; ++i) {
			Vertex &vertex = polygon.vertices[i];
			glVertex2f(vertex.x * width, vertex.y * height);
		}
	glEnd();
}

void Render(const GeneImage &gene_image) {
	for (int i = 0; i < gene_image.polygon_count; ++i) {
		RenderPolygon(gene_image.gene[i], gene_image.width, gene_image.height);
	}
}

void ReadBuffer(float *buffer, int width, int height, int channels) {
	glReadPixels(0, 0, width, height, channels == 4 ? GL_RGBA : GL_RGB, GL_FLOAT, buffer);
}

void UpdateAndRender(SimulationState &state, float temperature, float dt) {
	Poly *copy_gene = new Poly[state.gene_image.polygon_count];
	int old_polygon_count = state.gene_image.polygon_count;
	
	CopyPolygons(copy_gene, state.gene_image.gene, state.gene_image.polygon_count);
	
	Mutate(state);

	glClear(GL_COLOR_BUFFER_BIT);
	Render(state.gene_image);
	ReadBuffer(state.gene_image.data, state.gene_image.width, 
		state.gene_image.height, state.gene_image.channels);

	double new_fitness = Fitness(state.source_image.data, state.gene_image.data,
		state.gene_image.width, state.gene_image.height, state.gene_image.channels);
	if (new_fitness < state.gene_image.fitness ||
		Randf(0, 1) < BoltzmannProbability(state.gene_image.fitness, new_fitness, temperature)) {
		delete[] copy_gene;

		state.gene_image.fitness = new_fitness;
	} else {
		state.gene_image.polygon_count = old_polygon_count;

		delete[] state.gene_image.gene;
		state.gene_image.gene = copy_gene;
	}
}

void InitGeneImage(SimulationState &state, Image *image, int polygons, int vertices, GeneImage *gene_image) {
	gene_image->width = image->width;
	gene_image->height = image->height;
	gene_image->channels = image->channels;

	gene_image->data = new float[gene_image->width * gene_image->height * gene_image->channels];

	gene_image->polygon_count = polygons;
	gene_image->gene = new Poly[gene_image->polygon_count];
	for (int i = 0; i < polygons; ++i) {
		Poly &polygon = gene_image->gene[i];
		InitRandomPolygon(state, polygon, vertices);
	}
}

int main(int argc, char **argv) {
	srand(time(NULL));

	string input_file = "me2.png";

	SimulationState state;

	if (!LoadPNG(input_file, &state.source_image)) {
		LOG("Failed to load %s\n", input_file.c_str());
		exit(EXIT_FAILURE);
	}
	InitGeneImage(state, &state.source_image, 3, 8, &state.gene_image);

	GLFWwindow *window;
	if (!glfwInit()) {
		LOG("Failed to initialize glfw\n");
		exit(EXIT_FAILURE);
	}

	glfwSetErrorCallback(error_callback);

	window = glfwCreateWindow(state.source_image.width, 
		state.source_image.height, "Vectorize", NULL, NULL);
	if (!window) {
		LOG("Failed to create window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetWindowUserPointer(window, &state);

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		LOG("Failed to initialize glew\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glViewport(0, 0, state.source_image.width, state.source_image.height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluOrtho2D(0, state.source_image.width, 0, state.source_image.height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_MULTISAMPLE);
	glShadeModel(GL_SMOOTH);

	glEnable(GL_NORMALIZE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glDisable(GL_LIGHTING);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float temperature = 1.0f;
	double time = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		UpdateAndRender(state, temperature, glfwGetTime() - time);

		glfwSwapBuffers(window);

		while (glfwGetTime() - time < 1.0f / 60) {};
		time = glfwGetTime();
		temperature -= 0.001f;
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}