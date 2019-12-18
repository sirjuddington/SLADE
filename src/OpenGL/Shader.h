#pragma once

namespace OpenGL
{
class Shader
{
public:
	Shader(string_view name) : name_{ name } {}
	Shader(string_view name, const string& vertex_text, const string& fragment_text);

	unsigned id() const { return id_; }
	bool     isValid() const { return id_ > 0; }
	bool     isCurrent() const { return id_ == current_shader_; }

	bool loadVertex(const string& shader_text);
	bool loadVertexFile(const string& filename);
	bool loadFragment(const string& shader_text);
	bool loadFragmentFile(const string& filename);
	bool load(const string& vertex_text, const string& fragment_text, bool link = true);
	bool loadFiles(const string& vertex_file, const string& fragment_file, bool link = true);
	bool link();

	void bind() const;

	int uniformLocation(const string& name) const;

	bool setUniform(const string& name, bool value) const;
	bool setUniform(const string& name, int value) const;
	bool setUniform(const string& name, float value) const;
	bool setUniform(const string& name, glm::vec2 value) const;
	bool setUniform(const string& name, glm::vec3 value) const;
	bool setUniform(const string& name, glm::vec4 value) const;
	bool setUniform(const string& name, glm::mat4 value) const;

	static void          unbind();
	//static const Shader& default2D(bool textured = true);

private:
	unsigned id_          = 0;
	unsigned id_vertex_   = 0;
	unsigned id_fragment_ = 0;
	string   name_;

	struct UniformLocInfo
	{
		int    location = -1;
		string name;

		UniformLocInfo(string_view name, int location) : location{ location }, name{ name } {}
	};
	mutable std::vector<UniformLocInfo> uniform_locations_;

	static unsigned inline current_shader_ = 0;
};
} // namespace OpenGL
