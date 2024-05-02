#pragma once

namespace slade
{
namespace gl
{
	enum class Blend
	{
		Normal,
		Additive,
		Invert,

		Ignore
	};

	enum class Primitive
	{
		Points      = GL_POINTS,
		Lines       = GL_LINES,
		LineLoop    = GL_LINE_LOOP,
		Triangles   = GL_TRIANGLES,
		TriangleFan = GL_TRIANGLE_FAN
	};

	enum class PointSpriteType
	{
		Textured,
		Circle,
		CircleOutline,
		RoundedSquare,
		RoundedSquareOutline
	};

	struct Info
	{
		string vendor;
		string renderer;
		string version;

		Info() { vendor = renderer = version = "OpenGL not initialised"; }
	};

	wxGLContext*   getContext(wxGLCanvas* canvas = nullptr);
	bool           init();
	bool           contextCreationFailed();
	bool           np2TexSupport();
	bool           fboSupport();
	bool           validTexDimension(unsigned dim);
	unsigned       maxTextureSize();
	bool           isInitialised();
	wxGLAttributes getWxGLAttribs();
	void           setBlend(Blend blend);
	void           resetBlend();
	Info           sysInfo();

	inline glm::mat4 identityMatrix()
	{
		return glm::mat4{ 1.0f };
	}

	// Buffers
	unsigned createBuffer();
	void     deleteBuffer(unsigned id);

	// VBO
	unsigned currentVBO();
	void     bindVBO(unsigned id);
	void     deleteVBO(unsigned id);

	// EBO
	unsigned currentEBO();
	void     bindEBO(unsigned id);
	void     deleteEBO(unsigned id);

	// VAO
	unsigned currentVAO();
	unsigned createVAO();
	void     bindVAO(unsigned id);
	void     deleteVAO(unsigned id);

	// Drawing
	void     resetDrawCallCount();
	unsigned drawCallCount();
	void     drawArrays(Primitive primitive, unsigned first, unsigned count);
	void     drawArraysInstanced(Primitive primitive, unsigned first, unsigned count, unsigned instance_count);
	void     drawElements(Primitive primitive, unsigned count, GLenum type, const void* indices = nullptr);
	void     drawElementsInstanced(
			Primitive   primitive,
			unsigned    count,
			GLenum      type,
			unsigned    instance_count,
			const void* indices = nullptr);
} // namespace gl
} // namespace slade
