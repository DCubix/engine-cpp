#include <iostream>

#include "core/ecs.h"
#include "components/transform.h"

#include "core/input.h"
#include "core/app.h"
#include "core/filesys.h"
#include "gfx/api.h"
#include "gfx/shader.h"
#include "gfx/mesher.h"
#include "gfx/texture.h"


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

const vec3 lightDir = vec3(1.0);

void main() {
	vec3 n = normalize(oNormal);
	vec3 v = normalize(lightDir);
	float nv = max(dot(n, v), 0.2);
	fragColor = vec4(vec3(nv), 1.0) * texture(tTex, oUV);
}
)";

class TestApp : public IApplicationAdapter {
public:
	void init() {
		VFS::get().mountDefault(); // mounts to where the application resides
		
		Entity& e = eworld.create()
				.assign<Transform>();
		
		e.get<Transform>()->position = Vec3(3.0f, 4.0f, 2.0f);
		
		eworld.each([=](Entity& ent, Transform& t) {
			LogInfo(t.position.x);
		});
		
		proj = Mat4::perspective(radians(30), 640.0f / 480.0f, 0.02f, 600.0f);
		view = Mat4::translation(Vec3(0, 0, -4));
		mdl = Mat4::rotationY(radians(25));

		glClearColor(0.04f, 0.25f, 0.53f, 1.0f);

		prog = Builder<ShaderProgram>::build();
		prog.add(VS, api::ShaderType::VertexShader)
			.add(FS, api::ShaderType::FragmentShader)
			.link();

		model = Builder<Mesh>::build();
		model.addFromFile("test.glb").flush();
		
		texture_smp = Builder<Sampler>::build();
		texture_smp
				.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear)
				.setWrap();
		
		texture = Builder<Texture>::build();
		texture.bind(TextureTarget::Texture2D)
			.setFromFile("tex_col.png")
			.generateMipmaps()
			.unbind();

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
	}

	void update(float timeDelta) {
		if (Input::isKeyPressed(SDLK_ESCAPE)) {
			MessageSystem::get().submit("app_quit");
		}
		
		mdl = mdl * Mat4::rotationY(timeDelta);
	}

	void render() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		prog.bind();
		
		texture.bind(texture_smp, 0);
		
		prog.get("mProj").value().set(proj);
		prog.get("mView").value().set(view);
		prog.get("mModel").value().set(mdl);
		prog.get("tTex").value().set(0);

		model.bind();
		glDrawElements(GL_TRIANGLES, model.indexCount(), GL_UNSIGNED_INT, nullptr);
	}

	ShaderProgram prog;
	Mesh model;
	Texture texture;
	Sampler texture_smp;
	Mat4 proj, view, mdl;
	
	EntityWorld eworld;
};

int main(int argc, char** argv) {
	Application app(new TestApp());
	app.run();
	return 0;
}