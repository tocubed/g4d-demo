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
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

void setUniforms()
{
	float invert = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) ? -1.0 : 1.0;

	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		angle1 += invert * 0.011;
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		angle2 += invert * 0.011;
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		x_dist += invert * 0.111;

	Transform view;
	view.viewSpace(glm::dvec4(0, 1, 0, 0), glm::dvec4(0, 0, 1, 0), 
	               glm::dvec4(0, 0, 0, 1));
	view.lookAt(glm::dvec4(), glm::dvec4(0, 0, 0, 1),
	            glm::dvec4(0, 1, 0, 0), glm::dvec4(0, 0, 1, 0));

	Transform model;
	model.translate(x_dist, 0, 0, 20);
	model.rotate(angle1, glm::dvec4(1, 0, 1, 0), glm::dvec4(0, 1, 0, 1));
	model.rotate(angle2, glm::dvec4(0, 1, 1, 0), glm::dvec4(1, 0, 0, 1));
	model.scale(10, 10, 10, 10);

	Transform model_view = view * model;

	glm::mat4 projection = glm::perspective(45.0f, 480 / 320.0f, 0.1f, 100.0f);

	program->bind();

	glm::mat4 model_view_linear_map = model_view.getLinearMap();
	glm::vec4 model_view_translation = model_view.getTranslation();

	glUniformMatrix4fv(program->getUniformLocation("ModelViewLinearMap"),
	                   1, GL_FALSE, glm::value_ptr(model_view_linear_map));
	glUniform4fv(program->getUniformLocation("ModelViewTranslation"),
	             1, glm::value_ptr(model_view_translation));
	glUniformMatrix4fv(program->getUniformLocation("Projection"),
	                   1, GL_FALSE, glm::value_ptr(projection));

	program->release();
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
	static const float hypercube[] = {
		  -0.5,  -0.5,  -0.5,  -0.5,
		  -0.5,  -0.5,  -0.5,   0.5,
		  -0.5,  -0.5,   0.5,  -0.5,
		  -0.5,  -0.5,   0.5,   0.5,
		  -0.5,   0.5,  -0.5,  -0.5,
		  -0.5,   0.5,  -0.5,   0.5,
		  -0.5,   0.5,   0.5,  -0.5,
		  -0.5,   0.5,   0.5,   0.5,
		   0.5,  -0.5,  -0.5,  -0.5,
		   0.5,  -0.5,  -0.5,   0.5,
		   0.5,  -0.5,   0.5,  -0.5,
		   0.5,  -0.5,   0.5,   0.5,
		   0.5,   0.5,  -0.5,  -0.5,
		   0.5,   0.5,  -0.5,   0.5,
		   0.5,   0.5,   0.5,  -0.5,
		   0.5,   0.5,   0.5,   0.5
	};

	static const float texcoords[] = {
		  0,  0,  0,  
		  0,  0,  1, 
		  0,  1,  0, 
		  0,  1,  1, 
		  1,  0,  0, 
		  1,  0,  1, 
		  1,  1,  0, 
		  1,  1,  1, 
		  1,  1,  1, 
		  1,  1,  0, 
		  1,  0,  1, 
		  1,  0,  0, 
		  0,  1,  1, 
		  0,  1,  0, 
		  0,  0,  1, 
		  0,  0,  0
	};

	Vertex::vertex_layout
		.setSize(sizeof(Vertex))
		.add(VertexElement{offsetof(Vertex, position), VertexAttribute::Position,
				VertexAttributeType::Float, 4, false, false})
		.add(VertexElement{offsetof(Vertex, texcoord), VertexAttribute::TexCoord,
				VertexAttributeType::Float, 3, false, false});

	std::vector<Vertex> vertices;
	for(std::size_t i = 0; i < 16; i++)
	{
		vertices.push_back(Vertex{
			{hypercube[4*i], hypercube[4*i+1], hypercube[4*i+2], hypercube[4*i+3]},
			{texcoords[3*i], texcoords[3*i+1], texcoords[3*i+2]}});
	}
	
	static const unsigned int indices[] = {
		9 , 13 , 14 , 15 ,
		12 , 9 , 13 , 14 ,
		9 , 11 , 13 , 15 ,
		11 , 9 , 14 , 15 ,
		9 , 10 , 11 , 14 ,
		12 , 9 , 14 , 8 ,
		9 , 12 , 13 , 8 ,
		9 , 10 , 14 , 8 ,
		10 , 9 , 11 , 8 ,
		6 , 2 , 4 , 0 ,
		6 , 4 , 8 , 0 ,
		2 , 6 , 8 , 0 ,
		6 , 10 , 14 , 2 ,
		12 , 6 , 14 , 4 ,
		12 , 6 , 4 , 8 ,
		6 , 12 , 14 , 8 ,
		6 , 10 , 2 , 8 ,
		10 , 6 , 14 , 8 ,
		4 , 5 , 8 , 0 ,
		1 , 5 , 4 , 0 ,
		5 , 1 , 8 , 0 ,
		9 , 5 , 13 , 1 ,
		5 , 12 , 4 , 8 ,
		12 , 5 , 13 , 8 ,
		9 , 5 , 1 , 8 ,
		5 , 9 , 13 , 8 ,
		13 , 5 , 14 , 15 ,
		7 , 5 , 13 , 15 ,
		5 , 12 , 13 , 14 ,
		5 , 7 , 14 , 15 ,
		6 , 5 , 7 , 14 ,
		5 , 12 , 14 , 4 ,
		6 , 5 , 14 , 4 ,
		5 , 6 , 7 , 4 ,
		3 , 2 , 8 , 0 ,
		1 , 3 , 8 , 0 ,
		10 , 3 , 2 , 8 ,
		3 , 10 , 11 , 8 ,
		3 , 9 , 1 , 8 ,
		9 , 3 , 11 , 8 ,
		2 , 3 , 4 , 0 ,
		3 , 1 , 4 , 0 ,
		3 , 6 , 2 , 4 ,
		6 , 3 , 7 , 4 ,
		5 , 3 , 1 , 4 ,
		3 , 5 , 7 , 4 ,
		3 , 11 , 14 , 15 ,
		7 , 3 , 14 , 15 ,
		10 , 3 , 11 , 14 ,
		3 , 6 , 7 , 14 ,
		10 , 3 , 14 , 2 ,
		3 , 6 , 14 , 2 ,
		11 , 3 , 13 , 15 ,
		3 , 7 , 13 , 15 ,
		3 , 9 , 11 , 13 ,
		5 , 3 , 7 , 13 ,
		3 , 9 , 13 , 1 ,
		5 , 3 , 13 , 1
	};

	display_mesh = std::make_unique<GL::GLDisplayMesh>();
	display_mesh->begin();
	display_mesh->setVertexCount(16);
	display_mesh->setIndexCount(58 * 4);
	display_mesh->addVertices(&vertices[0]);
	display_mesh->addIndices(indices);
	display_mesh->end();
}


void drawModel()
{
	glBindTexture(GL_TEXTURE_3D, texture);
	display_mesh->draw();
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

	window = glfwCreateWindow(480, 320, "G4D Demo", nullptr, nullptr);
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

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		setUniforms();

		program->bind();

		drawModel();

		program->release();

        glfwSwapBuffers(window);
		showFPS(window);
    }

    glfwTerminate();

    return 0;
}
