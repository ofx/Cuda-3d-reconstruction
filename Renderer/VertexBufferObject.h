#pragma once

typedef struct Vertex
{
	float X, Y, Z;
	float R, G, B, A;

	Vertex() {}
	Vertex(float3 xyz) { this->X = xyz.x; this->Y = xyz.y; this->Z = xyz.z; this->R = 1.0f; this->G = 1.0f; this->B = 1.0f; this->A = 1.0f; }
	Vertex(float x, float y, float z) { this->X = x; this->Y = y; this->Z = z; this->R = 1.0f; this->G = 1.0f; this->B = 1.0f; this->A = 1.0f; }
	Vertex(float3 xyz, float4 rgba) { this->X = xyz.x; this->Y = xyz.y; this->Z = xyz.z; this->R = rgba.x; this->G = rgba.y; this->B = rgba.z; this->A = rgba.w; }
	Vertex(float x, float y, float z, float r, float g, float b, float a) { this->X = x; this->Y = y; this->Z = z; this->R = r; this->G = g; this->B = b; this->A = a; }
} Vertex;

class VertexBufferObject
{
private:
	std::vector<Vertex> m_Vertices;

	std::vector<unsigned int> m_Indices;

	glm::vec3 m_Translation, m_Rotation, m_Scale;

	glm::mat4 m_ModelMatrix;

	GLuint m_VertexArrayObject;
	GLuint m_VertexBufferObject;
	GLuint m_IndexBufferObject;

	GLenum m_Mode;

	bool m_IsHidden;

	bool m_IsBuilt;

	void DefineModelMatrix(void);
public:
	VertexBufferObject(GLenum mode);
	~VertexBufferObject(void);

	void AddVertex(const Vertex vertex);
	void AddPoint(const Vertex vertex);

	void AddIndex(const unsigned int index);

	glm::vec3 GetTranslation(void) { return this->m_Translation; }
	glm::vec3 GetRotation(void) { return this->m_Rotation; }
	glm::vec3 GetScale(void) { return this->m_Scale; }

	void SetTranslation(glm::vec3 translation);
	void SetRotation(glm::vec3 rotation);
	void SetScale(glm::vec3 scale);

	unsigned int GetNumVertices(void);

	glm::mat4 GetModelMatrix(void) const;

	void Print(void);

	void Build(void);
	void Draw(void) const;

	void Hide(void) { this->m_IsHidden = true; }
	void Unhide(void) { this->m_IsHidden = false; }
	void HideToggle(void) { this->m_IsHidden = !this->m_IsHidden; }

	bool IsHidden(void) { return this->m_IsHidden; }
};

