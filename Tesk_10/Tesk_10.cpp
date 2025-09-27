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

GLuint axisVAO = 0, axisVBO = 0;   // line �� vao, vbo
int addedSpace = -1;  // ��� �߰��� �ﰢ�� ��и�
int spaceCount[4] = { 1, 1, 1, 1 };   // �ش� ��и鿡 �ﰢ���� �� �� �ִ���

bool isFill = true;
bool moving[4];  // true�� �ε������ ������
struct Pos
{
	float x;
	float y;
};
// �簢 �����̷� ������ ��ǥ
Pos spiralRect[15] = { {-0.9f, 1.0f}, {-0.9f, -0.8f}, {0.9f, -0.8f}, {0.9f, 0.8f},
	{-0.7f, 0.8f }, {-0.7f, -0.6f}, {0.7f, -0.6f}, {0.7f, 0.6f}, 
	{-0.5f, 0.6f}, {-0.5f, -0.4f}, {0.5f, -0.4f}, {0.5f, 0.4f},
	{-0.3f, 0.4f}, {-0.3f, -0.2f}, {-0.3f, -0.2f} };

struct Tng
{
	GLuint VAO = 0, VBO = 0;
	vector<glm::vec3> vertices;
	glm::vec4 color;
	int vertCount() const { return (int)vertices.size(); }
	int space;   // �� ��и�����

	float vx, vy;    // �̵� �ӵ�
	int spiralIdx = 0; // ���� ��ǥ 
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
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(a, b);

	return dist(gen);
}

// ó�� �� ��и鿡 �ﰢ�� �׸���
void InitTng()
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
		tng.space = i;
		tng.spiralIdx = 0;
		updateTng(tng);
		tngs.push_back(tng);
	}
}

// �ﰢ�� �߰�
void AddTriangle(float nx, float ny)
{
	Tng tng;
	float r = randomFloat(0.0f, 1.0f);
	float g = randomFloat(0.0f, 1.0f);
	float b = randomFloat(0.0f, 1.0f);
	tng.color = { glm::vec4(r, g, b, 1.0) };
	tng.vertices = { glm::vec3(nx, ny, 0.0f),
		glm::vec3(nx - 0.05f, ny - 0.2f, 0.0f),
		glm::vec3(nx + 0.05f, ny - 0.2f, 0.0f) };
	// ��и� �Ǻ�
	int spaceIdx = -1;
	for (int i = 0; i < 4; ++i)
	{
		if (nx >= spaces[i].left && nx <= spaces[i].right &&
			ny - 0.1f >= spaces[i].bottom && ny - 0.1f <= spaces[i].top)
		{
			spaceIdx = i;
			break;
		}
	}
	tng.space = spaceIdx;
	tng.spiralIdx = 0;

	updateTng(tng);
	tngs.push_back(tng);
	addedSpace = tng.space;
}

// �ﰢ�� ����
void RemoveTriangle()
{
	for (int i = 0; i < tngs.size(); i++)
	{
		if (addedSpace == tngs[i].space)
		{
			tngs.erase(tngs.begin() + i);
			break; // ���� �� �ݺ��� ����
		}
	}
}

// �ﰢ�� �߰�(������ ��ư)
void AddTriangle2(float nx, float ny)
{
	Tng tng;
	float r = randomFloat(0.0f, 1.0f);
	float g = randomFloat(0.0f, 1.0f);
	float b = randomFloat(0.0f, 1.0f);
	tng.color = { glm::vec4(r, g, b, 1.0) };
	tng.vertices = { glm::vec3(nx, ny, 0.0f),
		glm::vec3(nx - 0.05f, ny - 0.2f, 0.0f),
		glm::vec3(nx + 0.05f, ny - 0.2f, 0.0f) };
	// ��и� �Ǻ�
	int spaceIdx = -1;
	for (int i = 0; i < 4; ++i)
	{
		if (nx >= spaces[i].left && nx <= spaces[i].right &&
			ny - 0.1f >= spaces[i].bottom && ny - 0.1f <= spaces[i].top)
		{
			spaceIdx = i;
			break;
		}
	}
	tng.space = spaceIdx;
	if (tng.space >= 0 && tng.space < 4)
	{
		spaceCount[tng.space] += 1;
	}

	if (spaceCount[tng.space] <= 4)
	{
		updateTng(tng);
		tngs.push_back(tng);
	}
}

// �� �׸���
void InitCenterCross()
{
	// ���߾� ����: (x=0�� ���μ�) + (y=0�� ���μ�)
	const float verts[] = {
		-1.0f,  0.0f, 0.0f,   1.0f,  0.0f, 0.0f,   // ���μ�: (-1,0) -> (1,0)
		 0.0f, -1.0f, 0.0f,   0.0f,  1.0f, 0.0f    // ���μ�: (0,-1) -> (0,1)
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
			AddTriangle(mouseX, mouseY);
			RemoveTriangle();
		}
	}
	else if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			PixelTrans(x, y, mouseX, mouseY);
			AddTriangle2(mouseX, mouseY);
		}
	}
	glutPostRedisplay();
}

void Reset()
{
	tngs.clear();
	addedSpace = -1;
	InitTng();
	for (int i = 0; i < 4; i++)
		spaceCount[i] = 1;
	glutPostRedisplay();
}

bool IsWallX(Tng& tng, float leftX, float rightX)
{
	float n1 = leftX + tng.vx;
	float n2 = rightX + tng.vx;

	if (n1 >= -1.0f && n2 <= 1.0f) return true;
	return false;
}

bool IsWallY(Tng& tng, float bottomY, float topY)
{
	float n1 = bottomY + tng.vy;
	float n2 = topY + tng.vy;

	if (n1 >= -1.0f && n2 <= 1.0f) return true;
	return false;
}

void Moving1()
{
	// �ﰢ�� ��ġ �̵�
	for (int i = 0; i < tngs.size(); i++)
	{
		if (!IsWallX(tngs[i], tngs[i].vertices[1].x, tngs[i].vertices[2].x))
			tngs[i].vx = -tngs[i].vx;
		if (!IsWallY(tngs[i], tngs[i].vertices[1].y, tngs[i].vertices[0].y))
			tngs[i].vy = -tngs[i].vy;
		for (auto& v : tngs[i].vertices)
		{
			v.x += tngs[i].vx;
			v.y += tngs[i].vy;
		}
		updateTng(tngs[i]); // VBO ����
	}
}

void SetSpiral()
{
	// �ﰢ������ ���� ��ġ�� �� ���� �̵�
	float sx = -0.2f;
	float sy = 1.0f;
	for (int i = 0; i < tngs.size(); i++)
	{
		float dx = sx - tngs[i].vertices[0].x;
		float dy = sy - tngs[i].vertices[0].y;
		for (auto& v : tngs[i].vertices)
		{
			v.x += dx;
			v.y += dy;
		}
		tngs[i].spiralIdx = 0; // �����̷� �ε����� 0���� �ʱ�ȭ
		updateTng(tngs[i]);
		sx += 0.2f;
	}
}

// �簢 �����̷�
void Moving2()
{
	float moveSpeed = 0.01f; // �̵� �ӵ�(���� ����)

	for (int i = 0; i < tngs.size(); i++)
	{
		Tng& t = tngs[i];
		// spiralRect�� ��ǥ ��ǥ
		int idx = t.spiralIdx;
		if (idx >= 15) continue; // spiralRect ���� �ʰ� ����

		float tx = spiralRect[idx].x;
		float ty = spiralRect[idx].y;

		// ���� �ﰢ���� ������(ù ��° ������) ��ġ
		float cx = t.vertices[0].x;
		float cy = t.vertices[0].y;

		// ��ǥ�������� ����
		float dx = tx - cx;
		float dy = ty - cy;
		float dist = sqrt(dx * dx + dy * dy);

		// ��ǥ���� ���� �����ϸ� ���� �ε�����
		if (dist < moveSpeed)
		{
			// ��Ȯ�� ��ǥ���� ���߰� ���� ��ǥ��
			float ddx = tx - cx;
			float ddy = ty - cy;
			for (auto& v : t.vertices)
			{
				v.x += ddx;
				v.y += ddy;
			}
			t.spiralIdx++;
			if (t.spiralIdx >= 15) 
			{
				// ��� ���� ��ġ�� ����
				SetSpiral();
				//t.spiralIdx = 1; // ���� ��ǥ�� spiralRect[1]
			}
		}
		else
		{
			// ���� �������ͷ� moveSpeed��ŭ �̵�
			float mx = dx / dist * moveSpeed;
			float my = dy / dist * moveSpeed;
			for (auto& v : t.vertices)
			{
				v.x += mx;
				v.y += my;
			}
		}
		updateTng(t);
	}
}

void Timer(int value)
{
	if (moving[0] || moving[1]) Moving1();
	if (moving[2]) Moving2();

	glutPostRedisplay();
	glutTimerFunc(16, Timer, 0); // ���� Ÿ�̸� ���� (16ms �� 60fps)
}

void Animation()
{
	if (moving[0])
	{
		float speed = 0.03f;
		for (auto& t : tngs)
		{
			t.vx = speed;
			t.vy = speed;
		}
		glutTimerFunc(16, Timer, 0);
	}
	else if (moving[1])
	{
		float dx = 0.05f;
		float dy = -0.002f;
		for (auto& t : tngs)
		{
			t.vx = dx;
			t.vy = dy;
		}
		glutTimerFunc(16, Timer, 0);
	}
	else if (moving[2])
	{
		SetSpiral();
		glutTimerFunc(16, Timer, 0);
	}
}

void Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case '1':
		moving[0] = !moving[0];
		for (int i = 1; i < 4; i++)
			moving[i] = false;
		Animation();
		break;
	case '2':
		moving[1] = !moving[1];
		for (int i = 0; i < 4; i++)
		{
			if (i == 1) continue;
			moving[i] = false;
		}
		Animation();
		break;
	case '3':
		moving[2] = !moving[2];
		for (int i = 0; i < 4; i++)
		{
			if (i == 2) continue;
			moving[i] = false;
		}
		Animation();
		break;
	case '4':
		break;
	case 'a':
		isFill = true;
		glutPostRedisplay();
		break;
	case 'b':
		isFill = false;
		glutPostRedisplay();
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
	glutCreateWindow("Tesk_09");

	glewExperimental = GL_TRUE;
	glewInit();

	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	InitCenterCross();  // ó���� ��и� ��
	InitTng();          // ó���� �� ��и鿡 �ﰢ�� �׸���

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutMouseFunc(Mouse);
	glutKeyboardFunc(Keyboard);
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

	// --- �߾� ���ڼ� �׸��� ---
	glUniform4f(locColor, 0.0f, 0.0f, 0.0f, 1.0f);   // ������
	glBindVertexArray(axisVAO);
	glLineWidth(1.0f);                                // ����̹� ���� >1.0�� ���õ� �� ����
	glDrawArrays(GL_LINES, 0, 4);                     // 2���� ����(�� 4�� ����)
	glBindVertexArray(0);

	if (isFill)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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