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
		
		eworld.registerSystem<RendererSystem>();
		
		model = Builder<Mesh>::build();
		model.addFromFile("test.glb").flush();
		
		alb0 = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("tex_col.png")
				.generateMipmaps();
		
		rme = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("tex_rough_metal.png")
				.generateMipmaps();
		
		// Camera
		Entity &cam = eworld.create()
			.assign<Transform>()
			.assign<Camera>(0.02f, 1000.0f, 40.0f);
		cam.get<Transform>()->position.z = 4.0f;
		
		Material def;
		def.roughness = 0.3f;
		def.metallic = 0.4f;
		
		def.setTexture(1, alb0)
			.setTextureEnabled(1, true)
			.setTextureType(1, TextureSlotType::Albedo0);
		
		// Models
		mod1 = &eworld.create()
			.assign<Transform>()
			.assign<Drawable3D>(model, def);
		mod1->get<Transform>()->position.x = -1.5f;
		
		Entity& mod2 = eworld.create()
			.assign<Transform>()
			.assign<Drawable3D>(model, def);
		mod2.get<Transform>()->position.x = 1.5f;
		
		// Lights
		Entity& l0 = eworld.create()
				.assign<Transform>()
				.assign<PointLight>();
		
		l0.get<Transform>()->position.x = 5.0f;
		l0.get<PointLight>()->radius = 5.0f;
		l0.get<PointLight>()->color = Vec3(1.0f, 0.5f, 0.0f);
		l0.get<PointLight>()->intensity = 1.0f;
		
		Entity& l1 = eworld.create()
				.assign<Transform>()
				.assign<PointLight>();
		
		l1.get<Transform>()->position.x = -5.0f;
		l1.get<PointLight>()->radius = 5.0f;
		l1.get<PointLight>()->color = Vec3(0.0f, 0.5f, 1.0f);
		l1.get<PointLight>()->intensity = 1.2f;
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
	Texture rme, alb0;
	
	Entity* mod1;
	EntityWorld eworld;
};

int main(int argc, char** argv) {
	Application app(new TestApp());
	app.run();
	return 0;
}