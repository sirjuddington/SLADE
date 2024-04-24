#pragma once

namespace slade
{
struct ColRGBA;

namespace gl
{
	class Shader
	{
	public:
		Shader(string_view name);
		Shader(string_view name, const string& vertex_text, const string& fragment_text);

		// Non-copyable
		Shader(const Shader&)      = delete;
		Shader& operator=(Shader&) = delete;

		unsigned id() const { return id_; }
		bool     isValid() const { return id_ > 0; }
		bool     isCurrent() const { return id_ == current_shader_; }

		void define(string_view name, string_view value = {});
		bool loadVertex(const string& shader_text);
		bool loadVertexFile(const string& filename);
		bool loadFragment(const string& shader_text);
		bool loadFragmentFile(const string& filename);
		bool loadGeometry(const string& shader_text);
		bool loadGeometryFile(const string& filename);
		bool load(
			const string& vertex_text,
			const string& fragment_text,
			const string& geometry_text = {},
			bool          link          = true);
		bool loadFiles(
			const string& vertex_file,
			const string& fragment_file,
			const string& geometry_file = {},
			bool          link          = true);
		bool loadResourceEntries(
			string_view vertex_entry,
			string_view fragment_entry,
			string_view geometry_entry = {},
			bool        link           = true);
		bool link();

		void bind() const;

		int uniformLocation(const string& name) const;

		bool setUniform(const string& name, bool value) const;
		bool setUniform(const string& name, int value) const;
		bool setUniform(const string& name, float value) const;
		bool setUniform(const string& name, uint16_t value) const;
		bool setUniform(const string& name, const glm::vec2& value) const;
		bool setUniform(const string& name, const glm::vec3& value) const;
		bool setUniform(const string& name, const glm::vec4& value) const;
		bool setUniform(const string& name, const glm::mat4& value) const;
		bool setUniform(const string& name, const ColRGBA& value) const;

		static void unbind();

	private:
		unsigned                 id_          = 0;
		unsigned                 id_vertex_   = 0;
		unsigned                 id_fragment_ = 0;
		unsigned                 id_geometry_ = 0;
		string                   name_;
		std::map<string, string> defines_;

		struct UniformLocInfo
		{
			int    location = -1;
			string name;

			UniformLocInfo(string_view name, int location) : location{ location }, name{ name } {}
		};
		mutable std::vector<UniformLocInfo> uniform_locations_;

		static unsigned inline current_shader_ = 0;
	};

	Shader* getShader(string_view name);
	void    reloadShaders();
} // namespace gl
} // namespace slade
