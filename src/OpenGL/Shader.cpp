
#include "Main.h"
#include "Shader.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "OpenGL.h"
#include "Utility/FileUtils.h"
#include <glm/gtc/type_ptr.hpp>

using namespace OpenGL;


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace
{
std::map<GLenum, std::string> shader_types = { { GL_VERTEX_SHADER, "vertex" },
											   { GL_FRAGMENT_SHADER, "fragment" },
											   { GL_GEOMETRY_SHADER, "geometry" } };
}


// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------
namespace
{
bool loadShader(const std::string& shader_text, GLenum type, GLuint& shader_id)
{
	if (shader_text.empty())
		return false;

	// Delete existing shader if it exists
	if (shader_id > 0)
	{
		glDeleteShader(shader_id);
		shader_id = 0;
	}

	// Create+compile shader
	auto shader_text_cstr = shader_text.c_str();
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
bool loadShaderFile(const std::string& filename, GLenum type, GLuint& shader_id)
{
	// Read text from file
	string shader_text;
	FileUtil::readFileToString(filename, shader_text);
	if (shader_text.empty())
		return false;

	// Load shader
	return loadShader(shader_text, type, shader_id);
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
Shader::Shader(string_view name, const std::string& vertex_text, const std::string& fragment_text) : name_{ name }
{
	loadVertex(vertex_text);
	loadFragment(fragment_text);
	link();
}

// ----------------------------------------------------------------------------
// Loads and compiles the vertex shader from [shader_text]
// ----------------------------------------------------------------------------
bool Shader::loadVertex(const string& shader_text)
{
	return loadShader(shader_text, GL_VERTEX_SHADER, id_vertex_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the vertex shader from a file [filename]
// ----------------------------------------------------------------------------
bool Shader::loadVertexFile(const std::string& filename)
{
	return loadShaderFile(filename, GL_VERTEX_SHADER, id_vertex_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the fragment shader from [shader_text]
// ----------------------------------------------------------------------------
bool Shader::loadFragment(const string& shader_text)
{
	return loadShader(shader_text, GL_FRAGMENT_SHADER, id_fragment_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the fragment shader from a file [filename]
// ----------------------------------------------------------------------------
bool Shader::loadFragmentFile(const std::string& filename)
{
	return loadShaderFile(filename, GL_FRAGMENT_SHADER, id_fragment_);
}

// ----------------------------------------------------------------------------
// Loads and compiles the vertex+fragment shaders from [vertex_text] and
// [fragment_text], and links the program if [link] is true
// ----------------------------------------------------------------------------
bool Shader::load(const string& vertex_text, const string& fragment_text, bool link)
{
	if (!loadVertex(vertex_text))
		return false;
	if (!loadFragment(fragment_text))
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
bool Shader::loadFiles(const std::string& vertex_file, const std::string& fragment_file, bool link)
{
	if (!loadVertexFile(vertex_file))
		return false;
	if (!loadFragmentFile(fragment_file))
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
int Shader::uniformLocation(const std::string& name) const
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
bool Shader::setUniform(const std::string& name, bool value) const
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
bool Shader::setUniform(const std::string& name, int value) const
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
bool Shader::setUniform(const std::string& name, float value) const
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
bool Shader::setUniform(const std::string& name, glm::vec2 value) const
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
bool Shader::setUniform(const std::string& name, glm::vec3 value) const
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
bool Shader::setUniform(const std::string& name, glm::vec4 value) const
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
bool Shader::setUniform(const std::string& name, glm::mat4 value) const
{
	if (auto loc = uniformLocation(name); loc >= 0)
	{
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
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

//// ----------------------------------------------------------------------------
//// Returns the 'default' 2D shader, creating and loading it if needed
//// ----------------------------------------------------------------------------
//const Shader& Shader::default2D(bool textured)
//{
//	static Shader shader_2d("default2d");
//	static Shader shader_2d_notex("default2d_notex");
//
//	if (!shader_2d.isValid())
//	{
//		string shader_vert, shader_frag, shader_frag_notex;
//
//		auto program_resource = App::archiveManager().programResourceArchive();
//		if (program_resource)
//		{
//			auto entry_vert       = program_resource->entryAtPath("shaders/default2d.vert");
//			auto entry_frag       = program_resource->entryAtPath("shaders/default2d.frag");
//			auto entry_frag_notex = program_resource->entryAtPath("shaders/default2d_notex.frag");
//			if (entry_vert && entry_frag && entry_frag_notex)
//			{
//				shader_vert.assign(reinterpret_cast<const char*>(entry_vert->rawData()), entry_vert->size());
//				shader_frag.assign(reinterpret_cast<const char*>(entry_frag->rawData()), entry_frag->size());
//				shader_frag_notex.assign(
//					reinterpret_cast<const char*>(entry_frag_notex->rawData()), entry_frag_notex->size());
//				shader_2d.load(shader_vert, shader_frag);
//				shader_2d_notex.load(shader_vert, shader_frag_notex);
//			}
//			else
//				Log::error("Unable to find default 2d shaders in the program resource!");
//		}
//	}
//
//	return textured ? shader_2d : shader_2d_notex;
//}
//
//const Shader& OpenGL::Shader::current()
//{
//	// TODO: insert return statement here
//}
