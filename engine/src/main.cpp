#include <iostream>

#include "components/transform.h"
#include "components/light.h"

#include "systems/renderer.h"
#include "systems/physics_system.h"

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

#include "imgui/imgui.h"

#include <cmath>

class TestApp : public IApplicationAdapter {
public:
	void init() {
		VFS::get().mountDefault(); // mounts to where the application resides

		sensitivity = 0.1f;
		mouseLocked = false;

		RendererSystem& rsys = eworld.registerSystem<RendererSystem>(config.width, config.height);
		eworld.registerSystem<PhysicsSystem>();

//		String tonemapF =
//#include "shaders/tonemapF.glsl"
//				;
//		Filter tonemap;
//		tonemap.setSource(tonemapF);
//		tonemap.addPass([](Filter& self) {  }); // Empty pass

//		rsys.addPostEffect(tonemap);

		String fxaaF =
#include "shaders/fxaaF.glsl"
				;
		Filter fxaa;
		fxaa.setSource(fxaaF);
		fxaa.addPass([](Filter& self) {  }); // Empty pass

		rsys.addPostEffect(fxaa);

		Texture envMap = Builder<Texture>::build()
				.bind(TextureTarget::CubeMap)
				.setCubemap("envmap.tga")
				.generateMipmaps();

		rsys.setEnvironmentMap(envMap);

		// Floor
		Material floorMat;
		floorMat.baseColor = Vec3(0.7f, 1.0f, 0.7f);
		floorMat.metallic = 0.0f;
		floorMat.roughness = 1.0f;

		Mesh floor = Builder<Mesh>::build();
		floor.addPlane(Axis::Y, 32.0f, Vec3(0.0f)).calculateNormals().calculateTangents().flush();

		Entity& floorEnt = eworld.create();
		floorEnt.assign<Drawable3D>(floor, floorMat);
		Transform &t = floorEnt.assign<Transform>();

		// Physics
//		floorEnt.assign<CollisionShape>(BoxShape::create(Vec3(20, 0.02f, 20)));
		floorEnt.assign<CollisionShape>(PlaneShape::create(Vec3(0, -1, 0), 0.0f));
		floorEnt.assign<RigidBody>(0.0f);
		//

		model = Builder<Mesh>::build();
//		model.addCube(1.0f);
//		model.calculateNormals().calculateTangents();
		model.addFromFile("test.glb");
//		model.addFromFile("fcube.obj");
//		model.addFromFile("logo.obj");
//		Vector<Vertex> vertices = model.vertexData();
//		Vector<u32> indices = model.indexData();
		model.flush();

//		alb0 = Builder<Texture>::build()
//				.bind(TextureTarget::Texture2D)
//				.setFromFile("box_albedo.png")
//				.generateMipmaps();

//		rme = Builder<Texture>::build()
//				.bind(TextureTarget::Texture2D)
//				.setFromFile("box_rme.png")
//				.generateMipmaps();

//		nrm = Builder<Texture>::build()
//				.bind(TextureTarget::Texture2D)
//				.setFromFile("box_normal.png")
//				.generateMipmaps();

//		disp = Builder<Texture>::build()
//				.bind(TextureTarget::Texture2D)
//				.setFromFile("box_height.png")
//				.generateMipmaps();

		// Camera
		camera = &eworld.create();
		camera->assign<Camera>(0.02f, 1000.0f, glm::radians(50.0f));

		Transform& camt = camera->assign<Transform>();
		camt.position = Vec3(0, 2.0f, 5.0f);

		// Models
		Material def;
		def.roughness = 0.04f;
		def.metallic = 0.25f;
		def.heightScale = 0.02f;
		def.instanced = true;

//		def.setTexture(0, rme)
//			.setTextureEnabled(0, true)
//			.setTextureType(0, TextureSlotType::RougnessMetallicEmission);

//		def.setTexture(1, alb0)
//			.setTextureEnabled(1, true)
//			.setTextureType(1, TextureSlotType::Albedo0);

//		def.setTexture(2, nrm)
//			.setTextureEnabled(2, true)
//			.setTextureType(2, TextureSlotType::NormalMap);

//		def.setTexture(3, disp)
//			.setTextureEnabled(3, true)
//			.setTextureType(3, TextureSlotType::HeightMap);

//		ShapeWrapper shape = BoxShape::create(model.aabb().max() - model.aabb().min());
//		ShapeWrapper shape = ConvexHullShape::create(vertices);
//		ShapeWrapper shape = CylinderShape::create(Vec3(1.21f, 0.18f, 1.21f));

		const i32 COUNT = 10;
		const i32 COUNT_1 = COUNT > 1 ? COUNT-1 : 1;
		for (i32 i = 0; i < COUNT; i++) {
			float fact = float(i) / float(COUNT_1);

			Entity& mod1 = eworld.create();
			mod1.assign<Drawable3D>(model, def);
			Transform& modt = mod1.assign<Transform>();
			modt.rotate(Vec3(0, 0, 1), glm::radians(180.0f));
			modt.rotate(Vec3(0, 1, 0), glm::radians(45.0f));
			modt.position = Vec3((fact * 2.0f - 1.0f) * COUNT * 1.6f, 1.0f, 0);

//			mod1.assign<CollisionShape>(shape);
//			mod1.assign<RigidBody>(1.0f);
		}

		// Lights
		Entity& s0 = eworld.create();
		Transform& s0t = s0.assign<Transform>();
		DirectionalLight& s0p = s0.assign<DirectionalLight>();

		s0t.position = Vec3(0.0f, 4.0f, 4.0f);
		s0t.lookAt(s0t.position, Vec3(0.0f), Vec3(0, 1, 0));
		s0p.intensity = 1.0f;
		s0p.shadows = true;
		s0p.shadowFrustumSize = 20.0f;
	}

	void update(float timeDelta) {
		t += timeDelta;

		const Vec2 center(config.width / 2, config.height / 2);

		bool mouseBusy = ImGui::IsAnyItemHovered() || ImGui::IsMouseHoveringAnyWindow();
		if (Input::isMouseButtonPressed(SDL_BUTTON_LEFT) && !mouseBusy) {
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
//		ImGui::BeginDock("Dock 1");
//		ImGui::Text("Hello World!");
//		ImGui::Button("Click!");
//		ImGui::EndDock();

//		ImGui::BeginDock("Dock 2");
//		ImGui::Text("Hello World from Dock 2!");
//		ImGui::Button("Click!");
//		ImGui::EndDock();

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
