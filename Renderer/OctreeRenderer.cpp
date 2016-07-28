#include "Stdafx.h"

#include "OctreeRenderer.h"
#include "Exception.h"

#define ASSERT_GL() assert(glGetError() == GL_NO_ERROR)

OctreeRenderer::OctreeRenderer(Octree *octree, const unsigned int width, const unsigned int height) : m_Width(width), m_Height(height), m_NumVertices(0)
{
	this->m_Octree = octree;
	this->m_Camera = new Camera(glm::vec2(width, height));

	// Reset keys
	for (int i = 0 ; i < 1024 ; ++i)
	{
		this->m_Keys[i] = false;
	}

	// Initialize GLFW
	if (!glfwInit())
	{
		throw_line("Failed to initialize GLFW");
	}
	
	// Use OpenGL 4.1
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window
	if (!(this->m_Window = glfwCreateWindow(width, height, "OctreeRenderer", NULL, NULL)))
	{
		throw_line("Failed to create rendering window");
	}
	glfwMakeContextCurrent(this->m_Window);

	// Set error callback
	glfwSetWindowUserPointer(this->m_Window, this);
	glfwSetErrorCallback(OctreeRenderer::ErrorCallback);
	glfwSetScrollCallback(this->m_Window, OctreeRenderer::ScrollCallback);
	glfwSetKeyCallback(this->m_Window, OctreeRenderer::KeyCallback);
	glfwSetFramebufferSizeCallback(this->m_Window, OctreeRenderer::FramebufferSizeCallback);
	glfwSetCursorPosCallback(this->m_Window, OctreeRenderer::CursorPosCallback);
	glfwSetMouseButtonCallback(this->m_Window, OctreeRenderer::MouseButtonCallback);

	// Disable cursor
	glfwSetInputMode(this->m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Init GLEW
	glewExperimental = true;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		std::cout << glewGetErrorString(err) << std::endl;

		throw_line("Failed to initialize GLEW, see error above");
	}
	glGetError();

	// Enable alpha blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

	// Enable culling
	glEnable(GL_CULL_FACE);

	// Create VBO
	this->CreateVboVaoFromOctree();

	// Initialize shaders
	this->InitShaders();
	
	// Go
	this->Render();
}

OctreeRenderer::~OctreeRenderer()
{
	glDeleteShader(this->m_VertexShader);
	glDeleteShader(this->m_FragmentShader);
	glDeleteProgram(this->m_ShaderProgram);

	// Destroy window
	glfwDestroyWindow(this->m_Window);

	// De-initialize GLFW
	glfwTerminate();

	delete this->m_Camera;

	std::map<std::string, VertexBufferObject*>::const_iterator it;
	for (it = this->m_Objects.begin() ; it != this->m_Objects.end() ; ++it)
	{
		delete (*it).second;
	}
	this->m_Objects.clear();
}

void OctreeRenderer::Stop(void)
{
	glfwSetWindowShouldClose(this->m_Window, GL_TRUE);
}

void OctreeRenderer::Render(void)
{
	for (; !glfwWindowShouldClose(this->m_Window) ;)
	{
		GLfloat currentFrame = glfwGetTime();
		this->m_DeltaTime = currentFrame - this->m_LastFrame;
		this->m_LastFrame = currentFrame;

		// Check if movement is required, ideally we would want to have this in a key manager blablabla
		if (this->m_Keys[GLFW_KEY_W])
		{
			this->m_Camera->HandleMovement(this->m_DeltaTime, FORWARD);
		}
		if (this->m_Keys[GLFW_KEY_A])
		{
			this->m_Camera->HandleMovement(this->m_DeltaTime, LEFT);
		}
		if (this->m_Keys[GLFW_KEY_S])
		{
			this->m_Camera->HandleMovement(this->m_DeltaTime, BACKWARD);
		}
		if (this->m_Keys[GLFW_KEY_D])
		{
			this->m_Camera->HandleMovement(this->m_DeltaTime, RIGHT);
		}
		if (this->m_Keys[GLFW_KEY_Q])
		{
			this->m_Camera->HandleMovement(this->m_DeltaTime, DOWN);
		}
		if (this->m_Keys[GLFW_KEY_E])
		{
			this->m_Camera->HandleMovement(this->m_DeltaTime, UP);
		}

		ASSERT_GL();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 v = this->m_Camera->GetView();
		glm::mat4 p = this->m_Camera->GetProjection();
		
		std::map<std::string, VertexBufferObject*>::const_iterator it;
		for (it = this->m_Objects.begin() ; it != this->m_Objects.end() ; ++it)
		{
			glm::mat4 m = (*it).second->GetModelMatrix();

			glm::mat4 mvp = p * v * m;

			// Set uniforms
			GLint vLoc = glGetUniformLocation(this->m_ShaderProgram, "v");
			glUniformMatrix4fv(vLoc, 1, GL_FALSE, glm::value_ptr(v));
			ASSERT_GL();

			GLint mLoc = glGetUniformLocation(this->m_ShaderProgram, "m");
			glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(m));
			ASSERT_GL();

			GLint mvpLoc = glGetUniformLocation(this->m_ShaderProgram, "mvp");
			glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
			ASSERT_GL();

			(*it).second->Draw();
		}

		glfwSwapBuffers(this->m_Window);
        glfwPollEvents();
	}
}

char *OctreeRenderer::ReadFile(char *path)
{
    FILE *fptr;
    long length;
    char *buf;
 
    fptr = fopen(path, "rb");
    if (!fptr) return NULL;
    fseek(fptr, 0, SEEK_END);
    length = ftell(fptr);
    buf = (char*)malloc(length + 1); 
    fseek(fptr, 0, SEEK_SET);
    fread(buf, length, 1, fptr);
    fclose(fptr);
    buf[length] = 0;
 
    return buf;
}

void OctreeRenderer::InitShaders(void)
{
	ASSERT_GL();

	// Vertex shader
	char *vertexShaderSource = this->ReadFile("Resources/Shaders/octree_renderer_default.vs");
	this->m_VertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(this->m_VertexShader, 1, &vertexShaderSource, 0);
	glCompileShader(this->m_VertexShader);
	ASSERT_GL();

	int isCompiledVs, maxLength;
	glGetShaderiv(this->m_VertexShader, GL_COMPILE_STATUS, &isCompiledVs);
    if (isCompiledVs == false)
    {
        glGetShaderiv(this->m_VertexShader, GL_INFO_LOG_LENGTH, &maxLength);
 
        char *vertexInfoLog = (char*) malloc(maxLength);
 
        glGetShaderInfoLog(this->m_VertexShader, maxLength, &maxLength, vertexInfoLog);
		std::cout << "Vertex shader compile fail log:" << std::endl;
		std::cout << vertexInfoLog << std::endl;

		throw_line("Failed to compile vertex shader, see above log");
    }

	// Fragment shader
	char *fragmentShaderSource = this->ReadFile("Resources/Shaders/octree_renderer_default.ps");
	this->m_FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(this->m_FragmentShader, 1, &fragmentShaderSource, 0);
	glCompileShader(this->m_FragmentShader);
	ASSERT_GL();

	int isCompiledPs;
	glGetShaderiv(this->m_FragmentShader, GL_COMPILE_STATUS, &isCompiledPs);
    if (isCompiledPs == false)
    {
        glGetShaderiv(this->m_FragmentShader, GL_INFO_LOG_LENGTH, &maxLength);
 
        char *fragmentInfoLog = (char*) malloc(maxLength);
 
        glGetShaderInfoLog(this->m_FragmentShader, maxLength, &maxLength, fragmentInfoLog);
		std::cout << "Fragment shader compile fail log:" << std::endl;
		std::cout << fragmentInfoLog << std::endl;

		throw_line("Failed to compile fragment shader, see above log");
    }

	// Create shader program
	this->m_ShaderProgram = glCreateProgram();
	glAttachShader(this->m_ShaderProgram, this->m_VertexShader);
	glAttachShader(this->m_ShaderProgram, this->m_FragmentShader);
	ASSERT_GL();

	// Bind attribute locations
	glBindAttribLocation(this->m_ShaderProgram, 0, "in_Position");
	glBindAttribLocation(this->m_ShaderProgram, 1, "In_Color");
	ASSERT_GL();

	// Link
	glLinkProgram(this->m_ShaderProgram);
	
	int isLinked;
	glGetProgramiv(this->m_ShaderProgram, GL_LINK_STATUS, (int *)&isLinked);
    if (isLinked == false)
    {
        glGetProgramiv(this->m_ShaderProgram, GL_INFO_LOG_LENGTH, &maxLength);
 
        char *shaderProgramInfoLog = (char *)malloc(maxLength);
 
        glGetProgramInfoLog(this->m_ShaderProgram, maxLength, &maxLength, shaderProgramInfoLog);
		std::cout << "Shader link fail log:" << std::endl;
		std::cout << shaderProgramInfoLog << std::endl;

		throw_line("Failed to link program, see above log");
    }

	// Use program
	glUseProgram(this->m_ShaderProgram);

	ASSERT_GL();
}

void OctreeRenderer::NodeToVertexList(OctreeNode *node, VertexBufferObject *vbo)
{
	vbo->AddVertex(Vertex(node->Center.x - node->Halfsize.x, node->Center.y - node->Halfsize.y, node->Center.z - node->Halfsize.z, 1.0f, 1.0f, 1.0f, 0.2f));
	vbo->AddVertex(Vertex(node->Center.x - node->Halfsize.x, node->Center.y + node->Halfsize.y, node->Center.z - node->Halfsize.z, 1.0f, 1.0f, 1.0f, 0.2f));
	vbo->AddVertex(Vertex(node->Center.x + node->Halfsize.x, node->Center.y + node->Halfsize.y, node->Center.z - node->Halfsize.z, 1.0f, 1.0f, 1.0f, 0.2f));
	vbo->AddVertex(Vertex(node->Center.x + node->Halfsize.x, node->Center.y - node->Halfsize.y, node->Center.z - node->Halfsize.z, 1.0f, 1.0f, 1.0f, 0.2f));
	vbo->AddVertex(Vertex(node->Center.x - node->Halfsize.x, node->Center.y - node->Halfsize.y, node->Center.z + node->Halfsize.z, 1.0f, 1.0f, 1.0f, 0.2f));
	vbo->AddVertex(Vertex(node->Center.x - node->Halfsize.x, node->Center.y + node->Halfsize.y, node->Center.z + node->Halfsize.z, 1.0f, 1.0f, 1.0f, 0.2f));
	vbo->AddVertex(Vertex(node->Center.x + node->Halfsize.x, node->Center.y + node->Halfsize.y, node->Center.z + node->Halfsize.z, 1.0f, 1.0f, 1.0f, 0.2f));
	vbo->AddVertex(Vertex(node->Center.x + node->Halfsize.x, node->Center.y - node->Halfsize.y, node->Center.z + node->Halfsize.z, 1.0f, 1.0f, 1.0f, 0.2f));
}

void OctreeRenderer::NodeToIndexList(const unsigned int offset, VertexBufferObject *vbo)
{
	vbo->AddIndex(offset + 0); vbo->AddIndex(offset + 3); vbo->AddIndex(offset + 1);
	vbo->AddIndex(offset + 3); vbo->AddIndex(offset + 2); vbo->AddIndex(offset + 1);

	vbo->AddIndex(offset + 3); vbo->AddIndex(offset + 7); vbo->AddIndex(offset + 2);
	vbo->AddIndex(offset + 7); vbo->AddIndex(offset + 6); vbo->AddIndex(offset + 2);

	vbo->AddIndex(offset + 7); vbo->AddIndex(offset + 4); vbo->AddIndex(offset + 6);
	vbo->AddIndex(offset + 4); vbo->AddIndex(offset + 5); vbo->AddIndex(offset + 6);

	vbo->AddIndex(offset + 4); vbo->AddIndex(offset + 0); vbo->AddIndex(offset + 5);
	vbo->AddIndex(offset + 0); vbo->AddIndex(offset + 1); vbo->AddIndex(offset + 5);

	vbo->AddIndex(offset + 1); vbo->AddIndex(offset + 2); vbo->AddIndex(offset + 5);
	vbo->AddIndex(offset + 2); vbo->AddIndex(offset + 6); vbo->AddIndex(offset + 5);
	 
	vbo->AddIndex(offset + 4); vbo->AddIndex(offset + 7); vbo->AddIndex(offset + 3);
	vbo->AddIndex(offset + 0); vbo->AddIndex(offset + 4); vbo->AddIndex(offset + 3);
}

void OctreeRenderer::BuildBuffers(OctreeNode *node, VertexBufferObject *points, VertexBufferObject *root, VertexBufferObject *nodes)
{
	if (node->IsRoot)
	{
		this->NodeToVertexList(node, root);
		this->NodeToIndexList(0, root);
	}
	else
	{
		int i = nodes->GetNumVertices();
		this->NodeToVertexList(node, nodes);
		this->NodeToIndexList(i, nodes);
	}

	if (node->IsSplit)
	{
		OctreeNode *child = node->FirstChild;
		do
		{
			this->BuildBuffers(child, points, root, nodes);
		}
		while ((child = child->NextSibling) != 0);
	}
	else
	{
		std::list<VisibleVoxel*>::const_iterator it;
		for (it = node->Objects.begin() ; it != node->Objects.end() ; ++it)
		{
			Vertex v;
			v.X = (*it)->X;
			v.Y = (*it)->Y;
			v.Z = (*it)->Z;
			v.R = float((*it)->R) / 255.0;
			v.G = float((*it)->G) / 255.0;
			v.B = float((*it)->B) / 255.0;
			v.A = 1.0f;

			points->AddPoint(v);

			++this->m_NumVertices;
		}
	}
}

void OctreeRenderer::CreateVboVaoFromOctree(void)
{
	ASSERT_GL();

	VertexBufferObject *octreePoints = new VertexBufferObject(GL_POINTS);
	VertexBufferObject *root = new VertexBufferObject(GL_LINE_STRIP);
	VertexBufferObject *nodes = new VertexBufferObject(GL_LINE_STRIP);

	// Create vertices from octree
	this->BuildBuffers(this->m_Octree->GetRoot(), octreePoints, root, nodes);

	octreePoints->Build();
	root->Build();
	nodes->Build();

	root->SetRotation(glm::vec3(0, 0, 5));
	octreePoints->SetRotation(glm::vec3(0, 0, 5));
	nodes->SetRotation(glm::vec3(0, 0, 5));

	this->m_Objects["Root"] = root;
	this->m_Objects["Points"] = octreePoints;
	this->m_Objects["Nodes"] = nodes;
}

void OctreeRenderer::ErrorCallback(int error, const char *description)
{
	std::stringstream ss;
	ss << "GLFW reports error (" << error << "): " << description; 

	throw_line(ss.str());
}

void OctreeRenderer::OnKey(GLFWwindow *window, int key, int scancode, int action, int mod)
{
	if (key == GLFW_KEY_ESCAPE)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	// Want to have a key manager blablabla
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			this->m_Keys[key] = true;

			if (key == GLFW_KEY_1)
			{
				this->m_Objects["Root"]->HideToggle();
			}
			else if (key == GLFW_KEY_2)
			{
				this->m_Objects["Points"]->HideToggle();
			}
			else if (key == GLFW_KEY_3)
			{
				this->m_Objects["Nodes"]->HideToggle();
			}
		}
		else if (action == GLFW_RELEASE)
		{
			this->m_Keys[key] = false;
		}
	}
}

void OctreeRenderer::OnFramebufferSize(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}
void OctreeRenderer::OnScroll(GLFWwindow *window, double x, double y)
{
	this->m_Camera->HandleScroll(y);
}
void OctreeRenderer::OnMouseButton(GLFWwindow * window, int button, int action, int mods)
{

}
void OctreeRenderer::OnCursorPos(GLFWwindow *window, double x, double y)
{
	this->m_Camera->HandleMouse(x, y);
}

void OctreeRenderer::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod)
{
	OctreeRenderer *tthis = (OctreeRenderer*) glfwGetWindowUserPointer(window);
	tthis->OnKey(window, key, scancode, action, mod);
}

void OctreeRenderer::FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
	OctreeRenderer *tthis = (OctreeRenderer*) glfwGetWindowUserPointer(window);
	tthis->OnFramebufferSize(window, width, height);
}

void OctreeRenderer::ScrollCallback(GLFWwindow *window, double x, double y)
{
	OctreeRenderer *tthis = (OctreeRenderer*) glfwGetWindowUserPointer(window);
	tthis->OnScroll(window, x, y);
}

void OctreeRenderer::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
	OctreeRenderer *tthis = (OctreeRenderer*) glfwGetWindowUserPointer(window);
	tthis->OnMouseButton(window, button, action, mods);
}

void OctreeRenderer::CursorPosCallback(GLFWwindow *window, double x, double y)
{
	OctreeRenderer *tthis = (OctreeRenderer*) glfwGetWindowUserPointer(window);
	tthis->OnCursorPos(window, x, y);
}
