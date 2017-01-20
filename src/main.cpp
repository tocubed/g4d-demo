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

GLFWwindow* window;

std::string readFile(const char* name)
{
	std::ifstream ifs(name);

	return std::string(
	    std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

std::unique_ptr<ShaderProgram> program;

void initProgram()
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
	glBindAttribLocation(program->getId(), (GLuint)VertexAttribute::Position, 
			"position");
	glBindAttribLocation(program->getId(), (GLuint)VertexAttribute::TexCoord, 
			"texcoord");
	program->link(vertex, geometry, fragment);
	
	assert(program->isLinked());
}

GLuint texture;

void loadTexture()
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_3D, texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	
	int w, h, n;
	void* data = stbi_load("../images/hypercube_texture.png", &w, &h, &n, 0);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, w, w, h / w, 0, GL_RGB,
	             GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_3D);
	
	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_3D, 0);
}

float angle1;
float angle2;
float x_dist;

void handleInput()
{
	float invert = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) ? -1.0 : 1.0;

	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		angle1 += invert * 0.011;
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		angle2 += invert * 0.011;
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		x_dist += invert * 0.011;
}

void setUniforms(Transform view = Transform(), Transform model = Transform())
{
	view.viewSpace(glm::dvec4(0, 1, 0, 0), glm::dvec4(0, 0, 1, 0), 
	               glm::dvec4(0, 0, 0, 1));
	view.lookAt(glm::dvec4(0, 0, 0, -4), glm::dvec4(0, 0, 0, 0),
	            glm::dvec4(0, 1, 0, 0), glm::dvec4(0, 0, 1, 0));
	view.rotate(angle2, glm::dvec4(0, 1, 0, 0), glm::dvec4(0, 0, 1, 0));
	view.rotate(angle1, glm::dvec4(0, 0, 1, 0), glm::dvec4(0, 0, 0, 1));
	view.rotate(x_dist, glm::dvec4(1, 0, 0, 0), glm::dvec4(0, 0, 0, 1));

	Transform model_view = view * model;

	glm::mat4 projection = glm::perspective(45.0f, 960 / 720.0f, 0.1f, 1000.0f);

	glm::mat4 model_view_linear_map = model_view.getLinearMap();
	glm::vec4 model_view_translation = model_view.getTranslation();

	glUniformMatrix4fv(program->getUniformLocation("ModelViewLinearMap"),
	                   1, GL_FALSE, glm::value_ptr(model_view_linear_map));
	glUniform4fv(program->getUniformLocation("ModelViewTranslation"),
	             1, glm::value_ptr(model_view_translation));
	glUniformMatrix4fv(program->getUniformLocation("Projection"),
	                   1, GL_FALSE, glm::value_ptr(projection));
}

std::unique_ptr<DisplayMesh> display_mesh;

struct Vertex
{
	float position[4];
	float texcoord[3];

	static VertexLayout vertex_layout;
};
VertexLayout Vertex::vertex_layout;

void initModel()
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

	Vertex::vertex_layout
		.setSize(sizeof(Vertex))
		.add(VertexElement{offsetof(Vertex, position), VertexAttribute::Position,
				VertexAttributeType::Float, 4, false, false})
		.add(VertexElement{offsetof(Vertex, texcoord), VertexAttribute::TexCoord,
				VertexAttributeType::Float, 3, false, false});

	display_mesh = std::make_unique<GL::GLDisplayMesh>();
	display_mesh->begin();
	display_mesh->setVertexCount(vertex_count);
	display_mesh->setIndexCount(index_count);
	display_mesh->addVertices(&vertices[0]);
	display_mesh->addIndices(&indices[0]);
	display_mesh->end();
}


void drawModel()
{
	glBindTexture(GL_TEXTURE_3D, texture);
	for(unsigned int i = 0; i < 2; i++)
	{
	for(unsigned int j = 0; j < 2; j++)
	{
	for(unsigned int k = 0; k < 2; k++)
	{
	for(unsigned int l = 0; l < 2; l++)
	{
		Transform model;
		model.rotate(x_dist*.73, glm::dvec4(1, 0, 0, 0), glm::dvec4(0, 1, 0, 0));
		model.rotate(x_dist*.39, glm::dvec4(1, 0, 0, 0), glm::dvec4(0, 0, 1, 0));
		model.rotate(x_dist*.97, glm::dvec4(1, 0, 0, 0), glm::dvec4(0, 0, 0, 1));
        model.translate(1.1 * i, 1.1 * j,
                        1.1 * k, 1.1 * l);

		program->bind();

        setUniforms(Transform(), model);
		display_mesh->draw();

		program->release();
	}
	}
	}
	}
}

double last_time;
int frames;

void showFPS(GLFWwindow *window)
{
	double current_time = glfwGetTime();
	double delta = current_time - last_time;

	frames++;
	if (delta >= 1.0)
	{
		auto fps = frames / delta;

		std::stringstream ss;
		ss << "G4D Demo [" << fps << " FPS]";

		glfwSetWindowTitle(window, ss.str().c_str());

		frames = 0;
		last_time = current_time;
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

	initProgram();
	initModel();
	loadTexture();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.529f, 0.808f, 0.98f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		handleInput();
		drawModel();

        glfwSwapBuffers(window);
		showFPS(window);
    }

    glfwTerminate();

    return 0;
}
