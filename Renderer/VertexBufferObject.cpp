#include "Stdafx.h"

#include "VertexBufferObject.h"

#define ASSERT_GL() assert(glGetError() == GL_NO_ERROR)
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

VertexBufferObject::VertexBufferObject(GLenum mode)
{
	this->m_Mode = mode;

	this->m_IsBuilt = false;
	this->m_IsHidden = false;

	// Defaults
	this->m_Translation = glm::vec3(0.0f, 0.0f, 0.0f);
	this->m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	this->m_Scale = glm::vec3(1.0f, 1.0f, 1.0f);

	this->DefineModelMatrix();

	glGenBuffers(1, &this->m_VertexBufferObject);
	glGenBuffers(1, &this->m_IndexBufferObject);
	glGenVertexArrays(1, &this->m_VertexArrayObject);
	ASSERT_GL();
}

VertexBufferObject::~VertexBufferObject(void)
{
	glDeleteBuffers(1, &this->m_VertexBufferObject);
	glDeleteBuffers(1, &this->m_IndexBufferObject);
	ASSERT_GL();
}

void VertexBufferObject::AddVertex(const Vertex vertex)
{
	this->m_Vertices.push_back(vertex);
}

void VertexBufferObject::AddPoint(const Vertex vertex)
{
	this->AddIndex(this->m_Vertices.size());
	this->m_Vertices.push_back(vertex);
}

void VertexBufferObject::AddIndex(const unsigned int index)
{
	this->m_Indices.push_back(index);
}

void VertexBufferObject::Build(void)
{
	if (this->m_Vertices.size() == 0 || this->m_Indices.size() == 0)
	{
		return;
	}

	glBindVertexArray(this->m_VertexArrayObject);
	ASSERT_GL();

	glBindBuffer(GL_ARRAY_BUFFER, this->m_VertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * this->m_Vertices.size(), &this->m_Vertices.front(), GL_STATIC_DRAW);
	ASSERT_GL();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_IndexBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * this->m_Indices.size(), &this->m_Indices.front(), GL_STATIC_DRAW);
	ASSERT_GL();

	this->m_IsBuilt = true;
}

void VertexBufferObject::Draw(void) const
{
	if (!this->m_IsBuilt || this->m_IsHidden)
	{
		return;
	}

	glBindVertexArray(this->m_VertexArrayObject);
	glBindBuffer(GL_ARRAY_BUFFER, this->m_VertexBufferObject);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (3 * sizeof(float)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_IndexBufferObject);
	glDrawElements(this->m_Mode, this->m_Indices.size(), GL_UNSIGNED_INT, 0);
}

void VertexBufferObject::SetTranslation(glm::vec3 translation)
{
	this->m_Translation = translation;

	this->DefineModelMatrix();
}

void VertexBufferObject::SetRotation(glm::vec3 rotation)
{
	this->m_Rotation = rotation;

	this->DefineModelMatrix();
}

void VertexBufferObject::SetScale(glm::vec3 scale)
{
	this->m_Scale = scale;

	this->DefineModelMatrix();
}

void VertexBufferObject::DefineModelMatrix(void)
{
	glm::mat4 t = glm::translate(
		glm::mat4(1.0f),
		this->m_Translation
	);

	glm::mat4 r = glm::rotate(
		t,
		this->m_Rotation.x,
		glm::vec3(1.0f, 0.0f, 0.0f)
	);
	r = glm::rotate(
		r,
		this->m_Rotation.y,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	r = glm::rotate(
		r,
		this->m_Rotation.z,
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	this->m_ModelMatrix = glm::scale(
		r,
		this->m_Scale
	);
}

unsigned int VertexBufferObject::GetNumVertices(void)
{
	return this->m_Vertices.size();
}

glm::mat4 VertexBufferObject::GetModelMatrix(void) const
{
	return this->m_ModelMatrix;
}

void VertexBufferObject::Print(void)
{
	{
		std::vector<Vertex>::const_iterator it;
		for (it = this->m_Vertices.begin() ; it != this->m_Vertices.end() ; ++it)
		{
			std::cout << "Vertex: (" << (*it).X << " " << (*it).Y << " " << (*it).Z << ")" << std::endl;
		}
	}

	{
		int i = 0;
		std::vector<unsigned int>::const_iterator it;
		for (it = this->m_Indices.begin() ; it != this->m_Indices.end() ; ++it)
		{
			if (i++ % 3 == 0)
			{
				std::cout << std::endl;
			}

			std::cout << "Index: " << *it << std::endl;
		}
	}
}