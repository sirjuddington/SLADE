#pragma once

namespace slade::map
{
struct PointLight;
}
namespace slade::strutil
{
struct Token;
}

namespace slade::game
{
class GLDefs
{
public:
	GLDefs()  = default;
	~GLDefs() = default;

	static GLDefs& get();

	void clear();
	void parse(const Archive& archive);

	optional<map::PointLight> pointLight(const string& name, const Vec3d& position) const;
	string                    boundLightName(const string& class_name, const string& sprite) const;

private:
	struct LightDef
	{
		glm::vec3 colour;
		int       radius;
		int       radius_secondary = -1;
		Vec3i     offset;
		bool      subtractive = false;
		bool      attenuated  = false;
		float     intensity   = 1.0f;
	};
	std::unordered_map<string, LightDef> lightdefs_;

	struct ObjectBinding
	{
		string frame;
		string light_name;
	};
	std::unordered_map<string, vector<ObjectBinding>> object_bindings_;

	unsigned parsePointLight(const vector<strutil::Token>& tokens, unsigned idx);
	unsigned parseObjectBinding(const vector<strutil::Token>& tokens, unsigned idx);
};
} // namespace slade::game
