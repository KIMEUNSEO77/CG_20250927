#define _CRT_SECURE_NO_WARNINGS 
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <glew.h>
#include <freeglut.h>
#include <freeglut_ext.h> 
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
using namespace std;

void make_vertexShaders();
void make_fragmentShaders();

GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);

int width = 1000;
int height = 800;

GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

float mouseX = 0.0f, mouseY = 0.0f;

GLuint axisVAO = 0, axisVBO = 0;   // line 용 vao, vbo

struct Tng
{
	GLuint VAO = 0, VBO = 0;
	vector<glm::vec3> vertices;
	glm::vec4 color;
	int vertCount() const { return (int)vertices.size(); }
};
vector<Tng> tngs;

struct Space
{
	float left; float right; float top; float bottom;
};
Space spaces[4] = {
	{-1.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 0.0f},
	{-1.0f, 0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f, -1.0f}
};

void updateTng(Tng& tng)
{
	if (!tng.VAO) glGenVertexArrays(1, &tng.VAO);
	if (!tng.VBO) glGenBuffers(1, &tng.VBO);

	glBindVertexArray(tng.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, tng.VBO);
	glBufferData(GL_ARRAY_BUFFER, tng.vertices.size() * sizeof(glm::vec3), tng.vertices.data(), GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

float randomFloat(float a, float b)
{
	std::random_device rd;
	std::mt19937 gen(rd());  // Mersenne Twister 엔진
	std::uniform_real_distribution<float> dist(a, b);

	return dist(gen);
}

// 처음 각 사분면에 삼각형 그리기
void InitRng()
{
	for (int i = 0; i < 4; i++)
	{
		Tng tng;
		float nx = randomFloat(spaces[i].left + 0.2f, spaces[i].right - 0.2f);
		float ny = randomFloat(spaces[i].bottom + 0.2f, spaces[i].top - 0.2f);
		tng.vertices = { glm::vec3(nx, ny, 0.0f), 
			glm::vec3(nx - 0.05f, ny - 0.2f, 0.0f), 
			glm::vec3(nx + 0.05f, ny - 0.2f, 0.0f) };
		float r = randomFloat(0.0f, 1.0f);
		float g = randomFloat(0.0f, 1.0f);
		float b = randomFloat(0.0f, 1.0f);
		tng.color = { glm::vec4(r, g, b, 1.0) };
		updateTng(tng);
		tngs.push_back(tng);
	}
}

// 삼각형 추가
void AddTriangle(float nx, float ny)
{
	if (tngs.size() >= 10) return; // 최대 10개 도형
	Tng tng;
    tng.color = glm::vec4(0.0, 0.0, 1.0, 1.0); // 파란색
	tng.vertices = { glm::vec3(nx, ny, 0.0f), 
		glm::vec3(nx - 0.05f, ny - 0.2f, 0.0f), 
		glm::vec3(nx + 0.05f, ny - 0.2f, 0.0f) };
	updateTng(tng);
	tngs.push_back(tng);
}

// 선 그리기
void InitCenterCross()
{
	// 정중앙 십자: (x=0의 세로선) + (y=0의 가로선)
	const float verts[] = {
		-1.0f,  0.0f, 0.0f,   1.0f,  0.0f, 0.0f,   // 가로선: (-1,0) -> (1,0)
		 0.0f, -1.0f, 0.0f,   0.0f,  1.0f, 0.0f    // 세로선: (0,-1) -> (0,1)
	};

	glGenVertexArrays(1, &axisVAO);
	glGenBuffers(1, &axisVBO);

	glBindVertexArray(axisVAO);
	glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

char* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb");
	if (!fptr)
		return NULL;
	fseek(fptr, 0, SEEK_END);
	length = ftell(fptr);
	buf = (char*)malloc(length + 1);
	fseek(fptr, 0, SEEK_SET);
	fread(buf, length, 1, fptr);
	fclose(fptr);
	buf[length] = 0;
	return buf;
	// Open file for reading 
	// Return NULL on failure 
	// Seek to the end of the file 
	// Find out how many bytes into the file we are 
	// Allocate a buffer for the entire length of the file and a null terminator 
	// Go back to the beginning of the file 
	// Read the contents of the file in to the buffer 
	// Close the file 
	// Null terminator 
	// Return the buffer 
}

// 마우스 클릭한 좌표 정규화
void PixelTrans(int px, int py, float& nx, float& ny)
{
	nx = 2.0f * px / width - 1.0f;
	ny = -2.0f * py / height + 1.0f;
}

void Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			PixelTrans(x, y, mouseX, mouseY);
			AddTriangle(mouseX, mouseY);
		}
	}
	glutPostRedisplay();
}

void main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("Tesk_09");

	glewExperimental = GL_TRUE;
	glewInit();

	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	InitCenterCross();  // 처음에 사분면 선
	InitRng();          // 처음에 각 사분면에 삼각형 그리기

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutMouseFunc(Mouse);
	glutMainLoop();
}

void make_vertexShaders()
{
	GLchar* vertexSource;
	vertexSource = filetobuf("vertex.glsl");
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cerr << "Error: vertex shader            \n" << errorLog << std::endl;
		return;
	}
}

void make_fragmentShaders()
{
	GLchar* fragmentSource;
	fragmentSource = filetobuf("fragment.glsl");
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: frag_shader            \n" << errorLog << std::endl;
		return;
	}
}

GLuint make_shaderProgram()
{
	GLint result;
	GLchar* errorLog = NULL;
	GLuint shaderID;
	shaderID = glCreateProgram();

	glAttachShader(shaderID, vertexShader);
	glAttachShader(shaderID, fragmentShader);

	glLinkProgram(shaderID);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program          \n" << errorLog << std::endl;
		return false;
	}
	glUseProgram(shaderID);
	return shaderID;
}

GLvoid drawScene()
{
	glClearColor(1.0, 1.0, 1.0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgramID);
	GLint locColor = glGetUniformLocation(shaderProgramID, "uColor");

	// --- 중앙 십자선 그리기 ---
	glUniform4f(locColor, 0.0f, 0.0f, 0.0f, 1.0f);   // 검정색
	glBindVertexArray(axisVAO);
	glLineWidth(1.0f);                                // 드라이버 따라 >1.0은 무시될 수 있음
	glDrawArrays(GL_LINES, 0, 4);                     // 2개의 선분(총 4개 정점)
	glBindVertexArray(0);

	for (int i = 0; i < (int)tngs.size(); ++i)
	{
		auto& s = tngs[i];
		glUniform4fv(locColor, 1, glm::value_ptr(s.color));
		glBindVertexArray(s.VAO);

		GLenum mode = GL_TRIANGLES;

		glDrawArrays(mode, 0, s.vertCount());
	}

	glBindVertexArray(0);
	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}