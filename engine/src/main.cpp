#include <iostream>

#include "components/transform.h"
#include "components/light.h"

#include "systems/renderer.h"

#include "core/ecs.h"
#include "core/input.h"
#include "core/app.h"
#include "core/filesys.h"

#include "gfx/api.h"
#include "gfx/material.h"
#include "gfx/shader.h"
#include "gfx/mesher.h"
#include "gfx/texture.h"

class TestApp : public IApplicationAdapter {
public:
	void init() {
		VFS::get().mountDefault(); // mounts to where the application resides
		
		RendererSystem& rsys = eworld.registerSystem<RendererSystem>();
		
		String tonemapF = 
#include "shaders/tonemapF.glsl"
				;
//		ShaderProgram toneMapS = Builder<ShaderProgram>::build()
//				.add(RendererSystem::POST_FX_VS, ShaderType::VertexShader)
//				.add(tonemapF, ShaderType::FragmentShader);
//		toneMapS.link();
//		rsys.addPostEffect(toneMapS);
		
		Texture envMap = Builder<Texture>::build()
				.bind(TextureTarget::CubeMap)
				.setCubemap("cubemap.png")
				.generateMipmaps();
		
		//rsys.setEnvironmentMap(envMap);
		
		model = Builder<Mesh>::build();
		model.addFromFile("test.glb").flush();
		
		alb0 = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("metal_col.png")
				.generateMipmaps();
		
		rme = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("metal_rme.png")
				.generateMipmaps();
		
		nrm = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("metal_normal.png")
				.generateMipmaps();
		
		// Camera
		Entity &cam = eworld.create();
		cam.assign<Camera>(0.02f, 1000.0f, 40.0f);
		
		Transform& camt = cam.assign<Transform>();
		camt.position.z = 4.0f;
		
		Material def;
		def.roughness = 1.0f;
		def.metallic = 1.0f;
		
		def.setTexture(0, rme)
			.setTextureEnabled(0, true)
			.setTextureType(0, TextureSlotType::RougnessMetallicEmission)
			.setTextureUVTransform(0, Vec4(0, 0, 3, 3));
		
		def.setTexture(1, alb0)
			.setTextureEnabled(1, true)
			.setTextureType(1, TextureSlotType::Albedo0)
			.setTextureUVTransform(1, Vec4(0, 0, 3, 3));
		
		def.setTexture(2, nrm)
			.setTextureEnabled(2, true)
			.setTextureType(2, TextureSlotType::NormalMap)
			.setTextureUVTransform(2, Vec4(0, 0, 3, 3));
		
		// Models
		mod1 = &eworld.create();
		mod1->assign<Drawable3D>(model, def);
		
		Transform& mod1t = mod1->assign<Transform>();
		mod1t.position.x = -1.5f;
		
		Entity& mod2 = eworld.create();
		mod2.assign<Drawable3D>(model, def);
		
		Transform& mod2t = mod2.assign<Transform>();
		mod2t.position.x = 1.5f;
		
		// Lights
		Entity& l0 = eworld.create();
		Transform& l0t = l0.assign<Transform>();
		PointLight& l0p = l0.assign<PointLight>();
		
		l0t.position.x = 6.0f;
		l0p.radius = 8.0f;
		l0p.color = Vec3(1.0f, 0.4f, 0.0f);
		l0p.intensity = 1.8f;
		
		Entity& l1 = eworld.create();
		Transform& l1t = l1.assign<Transform>();
		PointLight& l1p = l1.assign<PointLight>();
		
		l1t.position.x = -6.0f;
		l1p.radius = 8.0f;
		l1p.color = Vec3(0.0f, 0.5f, 1.0f);
		l1p.intensity = 1.8f;
		
		Entity& l2 = eworld.create();
		Transform& l2t = l2.assign<Transform>();
		DirectionalLight& l2p = l2.assign<DirectionalLight>();
		
		l2t.rotation.lookAt(Vec3(0, 0, 0), Vec3(0, 0, 1));
		l2p.intensity = 1.0f;
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

	Mesh model;
	Texture rme, alb0, nrm;
	
	Entity* mod1;
	EntityWorld eworld;
};

int main(int argc, char** argv) {
	Application app(new TestApp());
	app.run();
	return 0;
}