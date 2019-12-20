
#include "Main.h"
#include "Shader.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "OpenGL.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
#include <glm/gtc/type_ptr.hpp>

using namespace OpenGL;


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace
{
std::map<GLenum, string> shader_types = { { GL_VERTEX_SHADER, "vertex" },
										  { GL_FRAGMENT_SHADER, "fragment" },
										  { GL_GEOMETRY_SHADER, "geometry" } };
}
CVAR(String, gl_glsl_version, "330 core", CVar::Save)


// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------
namespace
{
bool loadShader(const string& shader_text, GLenum type, GLuint& shader_id, const std::map<string, string>& defines)
{
	if (shader_text.empty())
		return false;

	// Delete existing shader if it exists
	if (shader_id > 0)
	{
		glDeleteShader(shader_id);
		shader_id = 0;
	}

	// Prepend version to shader
	string shader_processed = fmt::format("#version {}\n\n", gl_glsl_version);

	// Add #defines
	for (const auto& def : defines)
	{
		if (def.second.empty())
			shader_processed += fmt::format("#define {}\n", def.first);
		else
			shader_processed += fmt::format("#define {} {}\n", def.first, def.second);
	}

	// Add shader source text
	shader_processed += shader_text;

	// Create+compile shader
	auto shader_text_cstr = shader_processed.c_str();
	shader_id             = glCreateShader(type);
	glShaderSource(shader_id, 1, &shader_text_cstr, nullptr);
	glCompileShader(shader_id);

	// Check for errors
	int success;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char log[512];
		glGetShaderInfoLog(shader_id, 512, nullptr, log);
		Log::error("Error compiling {} shader, see below:", shader_types[type]);
		Log::error(log);
		return false;
	}

	return true;
}

// ----------------------------------------------------------------------------
// Loads and compiles a shader of [type] from [filename] into [shader_id].
// ----------------------------------------------------------------------------
bool loadShaderFile(const string& filename, GLenum type, GLuint& shader_id, const std::map<string, string>& defines)
{
	// Read text from file
	string shader_text;
	FileUtil::readFileToString(filename, shader_text);
	if (shader_text.empty())
		return false;

	// Load shader
	return loadShader(shader_text, type, shader_id, defines);
}
} // namespace


// ----------------------------------------------------------------------------
//
// Shader Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Shader class constructor taking vertex and fragment shader text strings
// ----------------------------------------------------------------------------
Shader::Shader(string_view name, const string& vertex_text, const string& fragment_text) : name_{ name }
{
	loadVertex(vertex_text);
	loadFragment(fragment_text);
	link();
}

void OpenGL::Shader::define(string_view name, string_view value)
{
	if (isValid())
	{
		Log::warning("Attempting to add a #define for a loaded shader - this will have no effect");
		return;
	}

	auto def_name      = StrUtil::upper(name);
	defines_[def_name] = value;
}

// ----------------------------------------------------------------------------
// Loads and compiles the vertex shader from [shader_text]
// ----------------------------------------------------------------------------
bool Shader::loadVertex(const string& shader_text)
{
	return loadShader(shader_text, GL_VERTEX_SHADER, id_vertex_, defines_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the vertex shader from a file [filename]
// ----------------------------------------------------------------------------
bool Shader::loadVertexFile(const string& filename)
{
	return loadShaderFile(filename, GL_VERTEX_SHADER, id_vertex_, defines_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the fragment shader from [shader_text]
// ----------------------------------------------------------------------------
bool Shader::loadFragment(const string& shader_text)
{
	return loadShader(shader_text, GL_FRAGMENT_SHADER, id_fragment_, defines_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the fragment shader from a file [filename]
// ----------------------------------------------------------------------------
bool Shader::loadFragmentFile(const string& filename)
{
	return loadShaderFile(filename, GL_FRAGMENT_SHADER, id_fragment_, defines_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the geometry shader from [shader_text]
// ----------------------------------------------------------------------------
bool OpenGL::Shader::loadGeometry(const string& shader_text)
{
	return loadShader(shader_text, GL_GEOMETRY_SHADER, id_geometry_, defines_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the geometry shader from a file [filename]
// ----------------------------------------------------------------------------
bool OpenGL::Shader::loadGeometryFile(const string& filename)
{
	return loadShaderFile(filename, GL_GEOMETRY_SHADER, id_geometry_, defines_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the vertex+fragment shaders from [vertex_text] and
// [fragment_text], and links the program if [link] is true
// ----------------------------------------------------------------------------
bool Shader::load(const string& vertex_text, const string& fragment_text, const string& geometry_text, bool link)
{
	if (!loadVertex(vertex_text))
		return false;
	if (!loadFragment(fragment_text))
		return false;
	if (!geometry_text.empty())
		if (!loadGeometry(geometry_text))
			return false;
	if (link)
	{
		if (!this->link())
			return false;
	}

	return true;
}

// ----------------------------------------------------------------------------
// Loads and compiles the vertex+fragment shaders from [vertex_file] and
// [fragment_file] files, and links the program if [link] is true
// ----------------------------------------------------------------------------
bool Shader::loadFiles(const string& vertex_file, const string& fragment_file, const string& geometry_file, bool link)
{
	if (!loadVertexFile(vertex_file))
		return false;
	if (!loadFragmentFile(fragment_file))
		return false;
	if (!geometry_file.empty())
		if (!loadGeometryFile(geometry_file))
			return false;
	if (link)
	{
		if (!this->link())
			return false;
	}

	return true;
}

// ----------------------------------------------------------------------------
// Links the shader program from previously loaded vertex/fragment shaders
// ----------------------------------------------------------------------------
bool Shader::link()
{
	// Delete existing program if it exists
	if (id_ > 0)
	{
		glDeleteProgram(id_);
		id_ = 0;
	}

	// Create+link program
	id_ = glCreateProgram();
	glAttachShader(id_, id_vertex_);
	glAttachShader(id_, id_fragment_);
	if (id_geometry_ > 0)
		glAttachShader(id_, id_geometry_);
	glLinkProgram(id_);

	// Check for link errors
	int success;
	glGetProgramiv(id_, GL_LINK_STATUS, &success);
	if (!success)
	{
		// Log error
		char log[512];
		glGetProgramInfoLog(id_, 512, nullptr, log);
		Log::error(fmt::format(R"(Error linking program for shader "{}", see below:)", name_));
		Log::error(log);

		// Clean up
		glDeleteProgram(id_);
		id_ = 0;

		return false;
	}

	// Clean up
	glDeleteShader(id_vertex_);
	glDeleteShader(id_fragment_);
	if (id_geometry_ > 0)
		glDeleteShader(id_geometry_);

	return true;
}

// ----------------------------------------------------------------------------
// Binds this shader for use in OpenGL
// ----------------------------------------------------------------------------
void Shader::bind() const
{
	if (current_shader_ == id_)
		return;

	glUseProgram(id_);
	current_shader_ = id_;
}

// ----------------------------------------------------------------------------
// Returns the location of the shader uniform [name]
// (see glGetUniformLocation for possible return values)
// ----------------------------------------------------------------------------
int Shader::uniformLocation(const string& name) const
{
	// Check cache
	for (auto& loc_info : uniform_locations_)
		if (loc_info.name == name)
			return loc_info.location;

	// Not in cache, get location
	auto location = glGetUniformLocation(id_, name.c_str());
	if (location < 0)
		Log::warning(fmt::format(R"(Uniform "{}" does not exist in shader {})", name, name_));
	else if (location == GL_INVALID_VALUE)
		Log::warning(fmt::format("Shader {} has an invalid program id", name_));
	else if (location == GL_INVALID_OPERATION)
		Log::warning(fmt::format("Shader {} is not a valid shader program or is not yet linked", name_));

	// Add to cache
	uniform_locations_.emplace_back(name, location);

	return location;
}

// ----------------------------------------------------------------------------
// Sets the boolean (int) uniform [name] to [value]
// ----------------------------------------------------------------------------
bool Shader::setUniform(const string& name, bool value) const
{
	if (auto loc = uniformLocation(name); loc >= 0)
	{
		glUniform1i(loc, static_cast<int>(value));
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Sets the int uniform [name] to [value]
// ----------------------------------------------------------------------------
bool Shader::setUniform(const string& name, int value) const
{
	if (auto loc = uniformLocation(name); loc >= 0)
	{
		glUniform1i(loc, value);
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Sets the float uniform [name] to [value]
// ----------------------------------------------------------------------------
bool Shader::setUniform(const string& name, float value) const
{
	if (auto loc = uniformLocation(name); loc >= 0)
	{
		glUniform1f(loc, value);
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Sets the vec2 uniform [name] to [value]
// ----------------------------------------------------------------------------
bool Shader::setUniform(const string& name, const glm::vec2& value) const
{
	if (auto loc = uniformLocation(name); loc >= 0)
	{
		glUniform2fv(loc, 1, glm::value_ptr(value));
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Sets the vec3 uniform [name] to [value]
// ----------------------------------------------------------------------------
bool Shader::setUniform(const string& name, const glm::vec3& value) const
{
	if (auto loc = uniformLocation(name); loc >= 0)
	{
		glUniform3fv(loc, 1, glm::value_ptr(value));
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Sets the vec4 uniform [name] to [value]
// ----------------------------------------------------------------------------
bool Shader::setUniform(const string& name, const glm::vec4& value) const
{
	if (auto loc = uniformLocation(name); loc >= 0)
	{
		glUniform4fv(loc, 1, glm::value_ptr(value));
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Sets the mat4 uniform [name] to [value]
// ----------------------------------------------------------------------------
bool Shader::setUniform(const string& name, const glm::mat4& value) const
{
	if (auto loc = uniformLocation(name); loc >= 0)
	{
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Sets the vec4 uniform [name] to [value]
// ----------------------------------------------------------------------------
bool OpenGL::Shader::setUniform(const string& name, const ColRGBA& value) const
{
	if (auto loc = uniformLocation(name); loc >= 0)
	{
		glUniform4fv(loc, 1, glm::value_ptr(glm::vec4(value.fr(), value.fg(), value.fb(), value.fa())));
		return true;
	}

	return false;
}


// ----------------------------------------------------------------------------
//
// Shader Class Static Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Unbinds the current shader
// ----------------------------------------------------------------------------
void Shader::unbind()
{
	glUseProgram(0);
	current_shader_ = 0;
}
