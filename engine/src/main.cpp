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
		
		sensitivity = 0.1f;
		mouseLocked = false;
		
		RendererSystem& rsys = eworld.registerSystem<RendererSystem>();
		
		String fxaaF = 
#include "shaders/fxaaF.glsl"
				;
		ShaderProgram fxaaS = Builder<ShaderProgram>::build()
				.add(RendererSystem::POST_FX_VS, ShaderType::VertexShader)
				.add(fxaaF, ShaderType::FragmentShader);
		fxaaS.link();
		rsys.addPostEffect(fxaaS);
		
		Texture envMap = Builder<Texture>::build()
				.bind(TextureTarget::CubeMap)
				.setCubemap("cubemap.jpg")
				.generateMipmaps();
		
		rsys.setEnvironmentMap(envMap);
		
		model = Builder<Mesh>::build();
		model.addFromFile("sphere.obj").flush();
		
		alb0 = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("crystal_alb.png")
				.generateMipmaps();
		
		rme = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("crystal_rme.jpg")
				.generateMipmaps();
		
		nrm = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("crystal_normal.png")
				.generateMipmaps();
		
		disp = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("crystal_disp.png")
				.generateMipmaps();
		
		// Camera
		camera = &eworld.create();
		camera->assign<Camera>(0.02f, 1000.0f, glm::radians(50.0f));
		
		Transform& camt = camera->assign<Transform>();
		
		Material def;
		def.roughness = 0.1f;
		def.metallic = 1.0f;
		def.heightScale = 0.3f;
		
//		def.setTexture(0, rme)
//			.setTextureEnabled(0, true)
//			.setTextureType(0, TextureSlotType::RougnessMetallicEmission);
//	
//		def.setTexture(1, alb0)
//			.setTextureEnabled(1, true)
//			.setTextureType(1, TextureSlotType::Albedo0);
//		
//		def.setTexture(2, nrm)
//			.setTextureEnabled(2, true)
//			.setTextureType(2, TextureSlotType::NormalMap);
//		
//		def.setTexture(3, disp)
//			.setTextureEnabled(3, true)
//			.setTextureType(3, TextureSlotType::HeightMap);
		
		// Models
		mod1 = &eworld.create();
		mod1->assign<Drawable3D>(model, def);
		mod1->assign<Transform>();
		
		// Lights
		Entity& l0 = eworld.create();
		Transform& l0t = l0.assign<Transform>();
		SpotLight& l0p = l0.assign<SpotLight>();
		
		l0t.position = Vec3(-2.0f, 2.0f, 2.0f);
		l0t.lookAt(l0t.position, Vec3(0.0f), Vec3(0, 1, 0));
		l0p.radius = 10.0f;
		l0p.intensity = 1.5f;
		l0p.color = Vec3(1.0f, 0.6f, 0.0f);
		l0p.spotCutOff = 0.8f;
//		
//		Entity& l1 = eworld.create();
//		Transform& l1t = l1.assign<Transform>();
//		PointLight& l1p = l1.assign<PointLight>();
//		
//		l1t.position.x = -6.0f;
//		l1p.radius = 8.0f;
//		l1p.color = Vec3(0.0f, 0.6f, 1.0f);
//		l1p.intensity = 1.0f;
//		
//		Entity& l2 = eworld.create();
//		Transform& l2t = l2.assign<Transform>();
//		SpotLight& l2p = l2.assign<SpotLight>();
//		
//		l2t.position = Vec3(2.0f, 2.0f, 2.0f);
//		l2t.rotation.lookAt(l2t.position * -1.0f, Vec3());
//		l2p.radius = 10.0f;
//		l2p.intensity = 1.5f;
//		l2p.color = Vec3(0.0f, 0.6f, 1.0f);
//		l2p.spotCutOff = 0.8f;
	}

	void update(float timeDelta) {
		t += timeDelta;
		
		const Vec2 center(config.width / 2, config.height / 2);
		
		if (Input::isMouseButtonPressed(SDL_BUTTON_LEFT)) {
			mouseLocked = true;
			Input::setMousePosition(center);
			Input::setCursorVisible(false);
		}
		
		Transform* t = camera->get<Transform>();
		if (mouseLocked) {
			if (Input::isKeyPressed(SDLK_ESCAPE)) {
				mouseLocked = false;
				Input::setCursorVisible(true);
			}
			
			Vec2 delta = Input::getMousePosition() - center;
			bool rotY = delta.x != 0;
			bool rotX = delta.y != 0;
			
			if (rotY) {
				t->rotate(t->up(), glm::radians(delta.x * sensitivity));
			}
			
			if (rotX) {
				t->rotate(t->right(), glm::radians(delta.y * sensitivity));
			}

			if (rotY || rotX) {
				Input::setMousePosition(center);
			}
		}
		
		const float speed = 4.0f;
		if (Input::isKeyDown(SDLK_w)) {
			t->position += t->forward() * speed * timeDelta;
		} else if (Input::isKeyDown(SDLK_s)) {
			t->position += t->back() * speed * timeDelta;
		}
		
		if (Input::isKeyDown(SDLK_a)) {
			t->position += t->left() * speed * timeDelta;
		} else if (Input::isKeyDown(SDLK_d)) {
			t->position += t->right() * speed * timeDelta;
		}
		t->setDirty();
		
		eworld.update(timeDelta);
	}

	void render() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		eworld.render();
	}

	Mesh model;
	Texture rme, alb0, nrm, disp;
	
	Entity* mod1;
	
	Entity* camera;
	float sensitivity;
	bool mouseLocked;
	
	EntityWorld eworld;
	float t;
};

int main(int argc, char** argv) {
	Application app(new TestApp());
	app.run();
	return 0;
}