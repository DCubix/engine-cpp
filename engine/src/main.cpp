#include <iostream>

#include "core/ecs.h"
#include "components/transform.h"
#include "systems/renderer.h"

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
uniform mat4 mProjection;
uniform mat4 mView;
uniform mat4 mModel;
void main() {
	gl_Position = mProjection * mView * mModel * vec4(vPosition, 1.0);
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

const vec3 lightDir = vec3(1.0);
const float R = 0.5;

#define PI 3.141592654

vec3 Diffuse_OrenNayar(vec3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH) {
	float a = Roughness * Roughness;
	float s = a;// / ( 1.29 + 0.5 * a );
	float s2 = s * s;
	float VoL = 2 * VoH * VoH - 1;		// double angle identity
	float Cosri = VoL - NoV * NoL;
	float C1 = 1 - 0.5 * s2 / (s2 + 0.33);
	float C2 = 0.45 * s2 / (s2 + 0.09) * Cosri * ( Cosri >= 0 ? 1.0 / max(NoL, NoV) : 1 );
	return DiffuseColor / PI * ( C1 + C2 ) * ( 1 + Roughness * 0.5 );
}

float D_GGX(float Roughness, float NoH) {
	float a = Roughness * Roughness;
	float a2 = a * a;
	float d = ( NoH * a2 - NoH ) * NoH + 1;	// 2 mad
	return a2 / ( PI*d*d );					// 4 mul, 1 rcp
}

float Vis_Smith( float Roughness, float NoV, float NoL ) {
	float a = ( Roughness * Roughness );
	float a2 = a*a;

	float Vis_SmithV = NoV + sqrt( NoV * (NoV - NoV * a2) + a2 );
	float Vis_SmithL = NoL + sqrt( NoL * (NoL - NoL * a2) + a2 );
	return 1.0 / ( Vis_SmithV * Vis_SmithL );
}

vec3 F_Schlick( vec3 SpecularColor, float VoH ) {
	float Fc = pow( 1 - VoH, 5.0 );					// 1 sub, 3 mul
	return clamp( 50.0 * SpecularColor.g, 0.0, 1.0 ) * Fc + (1 - Fc) * SpecularColor;
	
}

float saturate(float v) {
	return max(min(v, 1.0), 0.0);
}

void main() {
	vec3 n = normalize(oNormal);
	vec3 v = normalize(-oPosition);
	vec3 l = normalize(lightDir);
	vec3 h = normalize(v + l);
	float nv = saturate(dot(n, v));
	float nl = saturate(dot(n, l) + 0.1);
	float vh = saturate(dot(v, h));
	float nh = saturate(dot(n, h));
	
	float D = D_GGX( R, nh );
	float Vis = Vis_Smith( R, nv, nl );
	vec3 F = F_Schlick( vec3(1.0), vh );

	vec3 Diffuse = Diffuse_OrenNayar(vec3(1.0), R, nv, nl, vh);

	fragColor = vec4((Diffuse + (D * Vis) * F) * nl, 1.0);
}
)";

class TestApp : public IApplicationAdapter {
public:
	void init() {
		VFS::get().mountDefault(); // mounts to where the application resides
		
		eworld.registerSystem<RendererSystem>();
		
		prog = Builder<ShaderProgram>::build();
		prog.add(VS, api::ShaderType::VertexShader)
			.add(FS, api::ShaderType::FragmentShader)
			.link();

		model = Builder<Mesh>::build();
		model.addFromFile("test.glb").flush();
		
		// Camera
		Entity &cam = eworld.create()
			.assign<Transform>()
			.assign<Camera>(0.02f, 1000.0f, 40.0f);
		cam.get<Transform>()->position.z = 4.0f;
		
		// Models
		mod1 = &eworld.create()
			.assign<Transform>()
			.assign<Drawable3D>(&model, &prog);
		mod1->get<Transform>()->position.x = -1.5f;
		
		Entity& mod2 = eworld.create()
			.assign<Transform>()
			.assign<Drawable3D>(&model, &prog);
		mod2.get<Transform>()->position.x = 1.5f;
	}

	void update(float timeDelta) {
		if (Input::isKeyPressed(SDLK_ESCAPE)) {
			MessageSystem::get().submit("app_quit");
		}
		
		mod1->get<Transform>()->rotation.rotate(Vec3(0, 1, 0), radians(timeDelta*50.0f));
		mod1->get<Transform>()->setDirty();
		
		eworld.update(timeDelta);
	}

	void render() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		eworld.render();
	}

	ShaderProgram prog;
	Mesh model;
	Entity* mod1;
	EntityWorld eworld;
};

int main(int argc, char** argv) {
	Application app(new TestApp());
	app.run();
	return 0;
}