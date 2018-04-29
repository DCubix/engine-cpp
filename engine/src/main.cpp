#include <iostream>

#include "core/input.h"
#include "core/app.h"
#include "gfx/api.h"
#include "gfx/shader.h"
#include "gfx/mesher.h"
#include "gfx/texture.h"
#include "core/filesys.h"

static const String VS = R"(
#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec4 vColor;
out vec3 oPosition;
out vec3 oNormal;
out vec2 oUV;
uniform mat4 mProj;
uniform mat4 mView;
uniform mat4 mModel;
void main() {
	gl_Position = mProj * mView * mModel * vec4(vPosition, 1.0);
	oPosition = (mView * mModel * vec4(vPosition, 1.0)).xyz;
	oNormal = normalize(mView * mModel * vec4(vNormal, 0.0)).xyz;
	oUV = vTexCoord;
}
)";

static const String FS = R"(
#version 330 core
out vec4 fragColor;
in vec3 oPosition;
in vec3 oNormal;
in vec2 oUV;

uniform sampler2D tTex;

void main() {
	vec3 n = normalize(oNormal);
	vec3 v = normalize(-oPosition);
	float nv = max(dot(n, v), 0.0);
	fragColor = vec4(vec3(nv), 1.0) * texture(tTex, oUV);
}
)";

class TestApp : public IApplicationAdapter {
public:
	void init() {
		VFS::get().mountDefault(); // mounts to where the application resides
		
		proj = Mat4::perspective(radians(30), 640.0f / 480.0f, 0.02f, 600.0f);
		view = Mat4::translation(Vec3(0, 0, -4));
		mdl = Mat4::rotationY(radians(25));

		glClearColor(0.04f, 0.25f, 0.53f, 1.0f);

		prog = Builder<ShaderProgram>::create();
		prog.add(VS, api::ShaderType::VertexShader)
			.add(FS, api::ShaderType::FragmentShader)
			.link();

		model = Builder<Mesh>::create();
		model.addFromFile("cube.obj").flush();
		
		rock_ground = Builder<Texture>::create();
		rock_ground.bind(TextureTarget::Texture2D)
			.setFromFile("rock_ground.jpg")
			.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear)
			.setWrap()
			.unbind();

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
	}

	void update(float timeDelta) {
		if (Input::isKeyPressed(SDLK_ESCAPE)) {
			MessageSystem::ston().submitAndSend("app_quit");
		}
	}

	void render() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		prog.bind();
		
		rock_ground.bind(0);
		
		prog.get("mProj").value().set(proj);
		prog.get("mView").value().set(view);
		prog.get("mModel").value().set(mdl);
		prog.get("tTex").value().set(0);

		model.bind();
		glDrawElements(GL_TRIANGLES, model.indexCount(), GL_UNSIGNED_INT, nullptr);
	}

	ShaderProgram prog;
	Mesh model;
	Texture rock_ground;
	Mat4 proj, view, mdl;
};

int main(int argc, char** argv) {
	Application app(new TestApp());
	app.run();
	return 0;
}