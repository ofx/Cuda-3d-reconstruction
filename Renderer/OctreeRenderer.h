#pragma once

#include <list>
#include <map>

#include "VertexBufferObject.h"
#include "Camera.h"

#include "Octree.h"

class OctreeRenderer
{
private:
	const unsigned int m_Width, m_Height;
	
	unsigned int m_NumVertices;

	GLFWwindow *m_Window;

	GLuint m_VertexShader, m_FragmentShader, m_ShaderProgram;

	std::map<std::string, VertexBufferObject*> m_Objects;

	std::thread *m_RenderThread;

	Octree *m_Octree;

	GLfloat m_DeltaTime, m_LastFrame;

	bool m_Keys[1024];

	Camera *m_Camera;

	char *ReadFile(char *path);

	void Render(void);

	void NodeToVertexList(OctreeNode *node, VertexBufferObject *vbo);
	void NodeToIndexList(const unsigned int offset, VertexBufferObject *vbo);

	void InitShaders(void);
	void BuildBuffers(OctreeNode *node, VertexBufferObject *points, VertexBufferObject *root, VertexBufferObject *nodes);
	void CreateVboVaoFromOctree(void);
public:
	OctreeRenderer(Octree *octree, const unsigned int width = 1280, const unsigned int height = 800);
	~OctreeRenderer(void);

	bool ShouldStop(void) { return glfwWindowShouldClose(this->m_Window); }

	void Stop(void);

	void OnKey(GLFWwindow *window, int key, int scancode, int action, int mod);
	void OnFramebufferSize(GLFWwindow *window, int width, int height);
	void OnScroll(GLFWwindow *window, double x, double y);
	void OnMouseButton(GLFWwindow * window, int button, int action, int mods);
	void OnCursorPos(GLFWwindow *window, double x, double y);

	static void ErrorCallback(int error, const char *description);
	static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod);
	static void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
	static void ScrollCallback(GLFWwindow *window, double x, double y);
	static void MouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
	static void CursorPosCallback(GLFWwindow *window, double x, double y);
};