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

#include "gfx/filter.h"

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
		Filter fxaa;
		fxaa.setSource(fxaaF);
		fxaa.addPass([](Filter& self) {  }); // Empty pass

		rsys.addPostEffect(fxaa);

		Texture envMap = Builder<Texture>::build()
				.bind(TextureTarget::CubeMap)
				.setCubemap("skybox.jpg")
				.generateMipmaps();

		rsys.setEnvironmentMap(envMap);

		// Floor
		Material floorMat;
		floorMat.baseColor = Vec3(0.7f, 1.0f, 0.7f);
		floorMat.metallic = 0.0f;
		floorMat.roughness = 1.0f;

		Mesh floor = Builder<Mesh>::build();
		floor.addPlane(Axis::Y, 64.0f, Vec3(0.0f)).flush();

		Entity& floorEnt = eworld.create();
		floorEnt.assign<Transform>();
		floorEnt.assign<Drawable3D>(floor, floorMat);
		//

		// Column
		Material columnMat;
		columnMat.baseColor = Vec3(1.0f, 0.7f, 0.7f);
		columnMat.metallic = 0.0f;
		columnMat.roughness = 1.0f;

		Mesh column = Builder<Mesh>::build();
		column.addFromFile("column.obj").flush();

		Entity& columnEnt = eworld.create();
		columnEnt.assign<Drawable3D>(column, columnMat);
		Transform& ct = columnEnt.assign<Transform>();
		ct.position = Vec3(-7.0f, 2.0f, 8.0f);
		ct.rotate(Vec3(1, 0, 0), glm::radians(45.0f));
		//

		model = Builder<Mesh>::build();
		model.addFromFile("test.glb").flush();

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
		camt.position = Vec3(-7.5f, 2.0f, 5.0f);

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
		Material def;
		def.roughness = 1.0f;
		def.metallic = 0.0f;
		def.instanced = true;

		const i32 COUNT = 10;
		for (i32 i = 0; i < COUNT; i++) {
			float fact = float(i) / float(COUNT-1);

//			Material def;
//			def.roughness = (1.0f - fact) + Epsilon;
//			def.metallic = fact;

			Entity& mod1 = eworld.create();
			mod1.assign<Drawable3D>(model, def);
			Transform& modt = mod1.assign<Transform>();
			modt.rotate(Vec3(0, 0, 1), glm::radians(180.0f));
			modt.rotate(Vec3(0, 1, 0), glm::radians(45.0f));
			modt.position = Vec3((fact * 2.0f - 1.0f) * COUNT * 1.5f, 1, 0);
		}

		// Lights
		Entity& l0 = eworld.create();
		Transform& l0t = l0.assign<Transform>();
		DirectionalLight& l0p = l0.assign<DirectionalLight>();

		l0t.position = Vec3(0.0f, 2.0f, 2.0f);
		l0t.lookAt(l0t.position, Vec3(0.0f), Vec3(0, 1, 0));
		l0p.intensity = 1.5f;
		l0p.shadows = true;
		l0p.shadowFrustumSize = 20.0f;
		l0p.size = 0.6f;
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
				t->rotate(Vec3(0, 1, 0), glm::radians(-delta.x * sensitivity));
			}

			if (rotX) {
				t->rotate(Vec3(1, 0, 0), glm::radians(-delta.y * sensitivity));
			}

			if (rotY || rotX) {
				Input::setMousePosition(center);
			}
		}

		const float speed = 4.0f;
		if (Input::isKeyDown(SDLK_w)) {
			t->position += t->forward() * speed * timeDelta;
		} else if (Input::isKeyDown(SDLK_s)) {
			t->position -= t->forward() * speed * timeDelta;
		}

		if (Input::isKeyDown(SDLK_a)) {
			t->position -= t->right() * speed * timeDelta;
		} else if (Input::isKeyDown(SDLK_d)) {
			t->position += t->right() * speed * timeDelta;
		}

		eworld.update(timeDelta);
	}

	void render() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		eworld.render();
	}

	Mesh model;
	Texture rme, alb0, nrm, disp;

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
