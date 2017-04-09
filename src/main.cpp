#include <g4d/Render/DisplayMesh.hpp>
#include <g4d/Render/VertexLayout.hpp>
#include <g4d/Render/GL/GLDisplayMesh.hpp>
#include <g4d/ShaderProgram.hpp>
#include <g4d/Transform.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <stb/stb_image.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

using namespace g4d;


/* Global resources */
GLFWwindow* window;
std::unique_ptr<DisplayMesh> model;
std::unique_ptr<ShaderProgram> program;
GLuint texture;


/* Layout of a vertex in CPU/GPU memory */
struct Vertex
{
	float position[4];
	float texcoord[3];

	static VertexLayout vertex_layout;
};
VertexLayout Vertex::vertex_layout;

/* Load 4D model with above vertex layout */
void loadModel()
{
	std::ifstream model_file("../models/cube.m4d", std::ios::binary);

	std::vector<Vertex> vertices;
	std::uint32_t vertex_count;

	model_file.read(reinterpret_cast<char*>(&vertex_count), 4);
	for(std::uint32_t i = 0; i < vertex_count; i++)
	{
		Vertex vertex;
		model_file.read(reinterpret_cast<char*>(vertex.position), 4 * 4);
		model_file.read(reinterpret_cast<char*>(vertex.texcoord), 4 * 3);
		vertices.push_back(vertex);
	}
	std::cout << vertex_count << " vertices\n";

	std::vector<std::uint32_t> indices;
	std::uint32_t index_count;

	model_file.read(reinterpret_cast<char*>(&index_count), 4);
	indices.resize(index_count);
	model_file.read(reinterpret_cast<char*>(&indices[0]), 4 * index_count);
	std::cout << index_count << " indices\n";

	Vertex::vertex_layout.setSize(sizeof(Vertex))
		.add(VertexElement{offsetof(Vertex, position), VertexAttribute::Position, VertexAttributeType::Float, 4, false, false})
		.add(VertexElement{offsetof(Vertex, texcoord), VertexAttribute::TexCoord, VertexAttributeType::Float, 3, false, false});

	model = std::make_unique<GL::GLDisplayMesh>();
	model->begin();
	model->setVertexCount(vertex_count);
	model->setIndexCount(index_count);
	model->addVertices(&vertices[0]);
	model->addIndices(&indices[0]);
	model->end();
}


/* Read a file directly into a std::string */
std::string readFile(const char* name)
{
	std::ifstream ifs(name);
	return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

/* Load 4D rendering shaders */
void loadProgram()
{
	Shader vertex(Shader::Type::Vertex);
	Shader geometry(Shader::Type::Geometry);
	Shader fragment(Shader::Type::Fragment);

	vertex.compile(readFile("../deps/g4d/shaders/textured/vert.glsl"));
	geometry.compile(readFile("../deps/g4d/shaders/textured/geom.glsl"));
	fragment.compile(readFile("../deps/g4d/shaders/textured/frag.glsl"));

	assert(vertex.isCompiled());
	assert(geometry.isCompiled());
	assert(fragment.isCompiled());

	program = std::make_unique<ShaderProgram>();
	glBindAttribLocation(program->getId(), (GLuint)VertexAttribute::Position, "position");
	glBindAttribLocation(program->getId(), (GLuint)VertexAttribute::TexCoord, "texcoord");
	program->link(vertex, geometry, fragment);
	
	assert(program->isLinked());
}


/* Load 3D volume texture */
void loadTexture()
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_3D, texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	
	int w, h, n;
	void* data = stbi_load("../images/hypercube_texture.png", &w, &h, &n, 4);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, w, w, h / w, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_3D);
	
	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_3D, 0);
}


/* Values which update every frame for animation purposes */
float angle1 = 0.0;
float angle2 = 0.0;
float angle3 = 0.0;

void updateAnimation()
{
	float invert = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) ? -1.0 : 1.0;

	angle1 += invert * 0.007;
	angle2 += invert * 0.003;
	angle3 += invert * 0.011;
}


/* 4D view transform */
Transform getViewTransform()
{
	Transform view;

	view.viewSpace(glm::dvec4(0, 1, 0, 0), glm::dvec4(0, 0, 1, 0), glm::dvec4(0, 0, 0, 1));
	view.lookAt(glm::dvec4(1, 1, 1, 8), glm::dvec4(1, 1, 1, 0), glm::dvec4(0, 1, 0, 0), glm::dvec4(0, 0, 1, 0));
	view.rotate(angle1, glm::dvec4(0, 0, 1, 0), glm::dvec4(0, 0, 0, 1));
	view.rotate(angle2, glm::dvec4(0, 1, 0, 0), glm::dvec4(0, 0, 1, 0));
	
	return view;
}

/* Set view-model and projection uniforms */
void setUniforms(Transform view, Transform model)
{
	Transform model_view = view * model;

	glm::mat4 model_view_linear_map = model_view.getLinearMap();
	glm::vec4 model_view_translation = model_view.getTranslation();
	glUniformMatrix4fv(program->getUniformLocation("ModelViewLinearMap"), 1, GL_FALSE, glm::value_ptr(model_view_linear_map));
	glUniform4fv(program->getUniformLocation("ModelViewTranslation"), 1, glm::value_ptr(model_view_translation));

	glm::mat4 projection = glm::perspective(45.0f, 960 / 720.0f, 0.1f, 1000.0f);
	glUniformMatrix4fv(program->getUniformLocation("Projection"), 1, GL_FALSE, glm::value_ptr(projection));
}


/* Draw a 3x3x3x3 arrangement of hypercubes */
void drawModel()
{
	glBindTexture(GL_TEXTURE_3D, texture);
	program->bind();

	for(unsigned int i = 0; i < 3; i++)
	{
		for(unsigned int j = 0; j < 3; j++)
		{
			for(unsigned int k = 0; k < 3; k++)
			{
				for(unsigned int l = 0; l < 3; l++)
				{
					Transform model_transform;
					model_transform.rotate(angle3*.73, glm::dvec4(1, 0, 0, 0), glm::dvec4(0, 1, 0, 0));
					model_transform.rotate(angle3*.39, glm::dvec4(1, 0, 0, 0), glm::dvec4(0, 0, 1, 0));
					model_transform.rotate(angle3*.97, glm::dvec4(1, 0, 0, 0), glm::dvec4(0, 0, 0, 1));
					model_transform.translate(1.2 * i, 1.2 * j, 1.2 * k, 1.2 * l);

					setUniforms(getViewTransform(), model_transform);
					model->draw();
				}
			}
		}
	}
}


int frames;
double prev_time;

/* Display FPS in window title string */
void showFPS(GLFWwindow *window)
{
	double current_time = glfwGetTime();
	double delta = current_time - prev_time;

	frames++;
	if (delta >= 1.0)
	{
		auto fps = frames / delta;

		std::stringstream ss;
		ss << "G4D Demo [" << fps << " FPS]";

		glfwSetWindowTitle(window, ss.str().c_str());

		frames = 0;
		prev_time = current_time;
	}
}


int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window = glfwCreateWindow(960, 720, "G4D Demo", nullptr, nullptr);
	glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

	loadProgram();
	loadModel();
	loadTexture();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.529f, 0.808f, 0.98f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		updateAnimation();
		drawModel();

        glfwSwapBuffers(window);
		showFPS(window);
    }

    glfwTerminate();

    return 0;
}
