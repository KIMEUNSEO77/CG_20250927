#define _CRT_SECURE_NO_WARNINGS 
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <glew.h>
#include <freeglut.h>
#include <freeglut_ext.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLint width = 1000, height = 800;
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

struct Spiral 
{
	GLuint VAO = 0, VBO = 0;

	float cx, cy;     // �߽�
	float a, b;       // ���� ������, ������ ������
	float theta;      // ���� ����
	int dir;          // ���� (1: �ð�, -1: �ݽð�)
	std::vector<glm::vec2> vertices;

	int drawCount = 0; // ���� �׸� �� ����(�ִϸ��̼�)
};

float mouseX = 0.0f, mouseY = 0.0f;
std::vector<Spiral> spirals;

bool timerActive = false;
bool isLine = false;

float bgColor[3] = { 0.0f, 0.0f, 0.0f };

float randomFloat(float a, float b)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(a, b);

	return dist(gen);
}

void RandomBG()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	bgColor[0] = randomFloat(0.0f, 1.0f);
	bgColor[1] = randomFloat(0.0f, 1.0f);
	bgColor[2] = randomFloat(0.0f, 1.0f);
}

void UpdateSpiral(Spiral& s)
{
	if (!s.VAO) glGenVertexArrays(1, &s.VAO);
	if (!s.VBO) glGenBuffers(1, &s.VBO);

	std::vector<glm::vec3> verts3;
	for (auto& v : s.vertices)
		verts3.push_back(glm::vec3(v.x, v.y, 0.0f));

	glBindVertexArray(s.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, s.VBO);
	glBufferData(GL_ARRAY_BUFFER, verts3.size() * sizeof(glm::vec3),
		verts3.data(), GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Timer(int value)
{
	bool needMore = false;
	for (auto& s : spirals)
	{
		if (s.drawCount < (int)s.vertices.size()) {
			s.drawCount++;
			needMore = true;
		}
	}
	glutPostRedisplay();
	if (needMore)
		glutTimerFunc(10, Timer, 0);
	else
		timerActive = false;
}

void AddSpiral(float x, float y)
{
	Spiral s;
	s.cx = x;
	s.cy = y;
	s.a = 0.002f;         // ���� ������
	s.b = 0.0015f;        // ������ ������
	s.theta = 0.0f;
	s.dir = 1;           // �ð����

	int n = 100;         // �� ����
	float turns = 3.0f;  // �� ���� ����
	s.vertices.clear();

	float theta_out = 0.0f;
	float r_out = 0.0f;

	for (int i = 0; i < n; ++i)
	{
		float t = (float)i / (n - 1);
		float theta = t * turns * 2.0f * 3.141592f * s.dir;
		float r = s.a + s.b * t * n;
		float x = s.cx + r * cos(theta);
		float y = s.cy + r * sin(theta);
		if (i == n - 1) { theta_out = theta; s.a = r; }
		s.vertices.push_back(glm::vec2(x, y));
	}
	float x_end = s.vertices.back().x;
	float y_end = s.vertices.back().y;
	float cx2 = s.cx + 0.2f;
	float cy2 = s.cy;

	// �ٱ� �� ����
	s.cx += 0.3f;
	s.dir = -s.dir;        // �ݽð����
	s.b = -0.0015f;        // ������ ������
	float theta0 = atan2f(y_end - cy2, x_end - cx2) + 3.141592f * 1.85f;

	for (int i = 0; i < n; ++i)
	{
		float t = (float)i / (n - 1);
		float theta = theta0 + t * turns * 2.0f * 3.141592f * s.dir;
		float r = s.a + s.b * t * n;
		float x = s.cx + r * cos(theta);
		float y = s.cy + r * sin(theta);
		s.vertices.push_back(glm::vec2(x, y));
	}

	UpdateSpiral(s);
	spirals.push_back(s);

	if (!timerActive)
	{
		timerActive = true;
		glutTimerFunc(10, Timer, 0);
	}
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

// ���콺 Ŭ���� ��ǥ ����ȭ
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
			AddSpiral(mouseX, mouseY);
			RandomBG();

			glutPostRedisplay();
		}
	}
}

void Reset()
{
	spirals.clear();
	glutPostRedisplay();
}

void RandomSpiral(int index)
{
	for (int i = 0; i < index; ++i)
	{
		float x = randomFloat(-0.8f, 0.6f);
		float y = randomFloat(-0.9f, 0.9f);

		AddSpiral(x, y);
		RandomBG();
		glutPostRedisplay();
	}
}

void Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case '1':
		RandomSpiral(1);
		break;
	case '2':
		RandomSpiral(2);
		break;
	case '3':
		RandomSpiral(3);
		break;
	case '4':
		RandomSpiral(4);
		break;
	case '5':
		RandomSpiral(5);
		break;
	case 'p':
		isLine = false;
		break;
	case 'l':
		isLine = true;
		break;
	case 'c':
		Reset();
		break;
	case 'q':
		exit(0);
		break;
	}
}

void main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("Tesk_11");

	glewExperimental = GL_TRUE;
	glewInit();

	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	glutKeyboardFunc(Keyboard);
	glutMouseFunc(Mouse);
	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
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
	glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgramID);
	GLint locColor = glGetUniformLocation(shaderProgramID, "uColor");
	glUniform4f(locColor, 1.0f, 1.0f, 1.0f, 1.0f);

	for (const auto& s : spirals)
	{
		glBindVertexArray(s.VAO);
		int n = s.drawCount;
		if (n > 1)
		{
			if (isLine)
			{
				glDrawArrays(GL_LINE_STRIP, 0, n);
			}
			else
			{
				// ���� �������θ� �� ����
				glPointSize(2.0f); // �� ũ��
				std::vector<GLuint> indices;
				int dashLen = 6, gapLen = 6;
				int pattern = dashLen + gapLen;
				for (int i = 0; i < n; ++i)
				{
					if ((i % pattern) < dashLen)
						indices.push_back(i);
				}
				// �ӽ÷� ���� ���
				glEnable(GL_PROGRAM_POINT_SIZE);
				glDrawElements(GL_POINTS, (GLsizei)indices.size(), GL_UNSIGNED_INT, indices.data());
				glDisable(GL_PROGRAM_POINT_SIZE);
			}
		}
	}

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}