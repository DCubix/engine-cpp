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
#include "gfx/imm.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl.h"

#include <cmath>

void drawTransformEditor(Transform& t) {
	ImGui::InputFloat3("Tr", glm::value_ptr(t.position), 4);

	Vec3 euler = glm::eulerAngles(t.rotation);
	if (ImGui::InputFloat3("Rt", glm::value_ptr(euler), 4)) {
		t.rotation = glm::quat(euler);
	}

	ImGui::InputFloat3("Sc", glm::value_ptr(t.scale), 4);
}

void drawCameraEditor(Camera& t) {
	const char* cameraTypes[] = { "Orthographic", "Perspective"  };
	static int cameraType = (int) t.type;
	if (ImGui::Combo("Type", &cameraType, cameraTypes, 2)) {
		t.type = (CameraType)cameraType;
	}
	if (t.type == CameraType::Perspective) {
		ImGui::SliderAngle("FOV", &t.FOV, 2.0f, 180.0f);
	} else {
		float os = t.orthoScale;
		if (ImGui::InputFloat("Scale", &os, 0.01f, 0.1f)) {
			t.orthoScale = os;
		}
	}

	ImGui::Separator();

	ImGui::DragFloat("zNear", &t.zNear, 0.1f, 0.0001f, 9999.0f, "%.4f");
	ImGui::DragFloat("zFar", &t.zFar, 0.1f, 0.0001f, 9999.0f, "%.4f");
}

void drawDrawable3DEditor(const String& entity, Drawable3D& d) {
	if (ImGui::TreeNode("Mesh")) {
		static bool openMeshDialog = false;

		ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
		if (ImGui::Button("Load...")) {
			openMeshDialog = true;
		}
		ImGui::PopItemWidth();

		if (openMeshDialog) {
			if (ImGui::FileDialog("Load Mesh", ".obj;.glb;.dae;.ply;.fbx")) {
				if (ImGui::FileDialogOk()) {
//					if (d.mesh.valid()) {
//						Builder<Mesh>::destroy(d.mesh);
//					}
					d.mesh = Builder<Mesh>::build()
							 .addFromFile(ImGui::GetFileDialogFileName());
					d.mesh.flush();
				}
				openMeshDialog = false;
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Material")) {
		if (ImGui::CollapsingHeader("Parameters")) {
			ImGui::PushItemWidth(160.0f);
			ImGui::ColorPicker3(
						"Base",
						glm::value_ptr((&d)->material.baseColor),
						ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_PickerHueWheel
			);

			ImGui::SliderFloat("Roughness", &d.material.roughness, 0.0001f, 1.0f);
			ImGui::SliderFloat("Metallic", &d.material.metallic, 0.0f, 1.0f);
			ImGui::SliderFloat("Emission", &d.material.emission, 0.0f, 1.0f);

			ImGui::Separator();

			ImGui::Checkbox("Instanced", &d.material.instanced);
			ImGui::Checkbox("Casts Shadow", &d.material.castsShadow);

			ImGui::Separator();

			ImGui::SliderFloat("Height Scale", &d.material.heightScale, 0.0f, 1.0f);
			ImGui::Checkbox("Discard Parallax Edges", &d.material.discardParallaxEdges);

			ImGui::PopItemWidth();
		}

		if (ImGui::CollapsingHeader("Textures")) {
			Drawable3D *dp = &d;
			for (u32 i = 0; i < TextureSlotType::TextureSlotCount; i++) {
				String texTitle = Util::strCat("Texture #", i);
				if (ImGui::TreeNode(texTitle.c_str())) {
					static bool openTexDialog = false;

					TextureSlot* slot = &dp->material.textures[i];
					ImGui::Checkbox("Enabled", &slot->enabled);
					if (slot->enabled) {
						const char* textureTypes[] = { "Albedo #0", "Albedo #1", "Normal Map", "RME", "Height Map" };
						int selectedTextureType = (int) slot->type;
						if (ImGui::Combo("Type", &selectedTextureType, textureTypes, TextureSlotType::TextureSlotCount)) {
							slot->type = (TextureSlotType) selectedTextureType;
						}

						if (ImGui::ImageButton(
								(ImTextureID)slot->texture.id(),
								ImVec2(100.0f, 100.0f)
								))
						{
							openTexDialog = true;
						}

						ImGui::DragFloat4("Transform", glm::value_ptr(slot->uvTransform), 0.0025f);

						if (openTexDialog) {
							if (ImGui::FileDialog("Load Texture", ".png;.jpg;.tga")) {
								if (ImGui::FileDialogOk()) {
									if (slot->texture.id() != 0) {
										Builder<Texture>::destroy(slot->texture);
										slot->texture.invalidate();
									}
									slot->texture = Builder<Texture>::build();
									slot->texture.bind(TextureTarget::Texture2D);
									slot->texture.setFromFile(ImGui::GetFileDialogFileName());
									slot->texture.generateMipmaps();
								}
								openTexDialog = false;
							}
						}
					}
					ImGui::TreePop();
				}
			}
		}

		ImGui::TreePop();
	}
}

class TestApp : public IApplicationAdapter {
public:
	void init() {
		VFS::get().mountDefault(); // mounts to where the application resides

		sensitivity = 0.1f;
		mouseLocked = false;

		rsys = &eworld.registerSystem<RendererSystem>(config.width, config.height);

		sceneFbo = Builder<FrameBuffer>::build()
				.setSize(config.width, config.height)
				.addRenderBuffer(TextureFormat::Depthf, Attachment::DepthAttachment)
				.addColorAttachment(TextureFormat::RGB, TextureTarget::Texture2D)
				.addDepthAttachment();

		eworld.registerSystem<PhysicsSystem>();

		String fxaaF =
#include "shaders/fxaaF.glsl"
				;
		Filter fxaa;
		fxaa.setSource(fxaaF);
		fxaa.addPass([](Filter& self) {  }); // Empty pass

		rsys->addPostEffect(fxaa);

		Texture envMap = Builder<Texture>::build()
				.bind(TextureTarget::CubeMap)
				.setCubemap("envmap.tga")
				.generateMipmaps();

		rsys->setEnvironmentMap(envMap);

		// Floor
		Material floorMat;
		floorMat.baseColor = Vec3(0.7f, 1.0f, 0.7f);
		floorMat.metallic = 0.0f;
		floorMat.roughness = 1.0f;

		Mesh floor = Builder<Mesh>::build();
		floor.addPlane(Axis::Y, 32.0f, Vec3(0.0f)).calculateNormals().calculateTangents().flush();

		Entity& floorEnt = eworld.create("ground");
		floorEnt.assign<Drawable3D>(floor, floorMat);
		Transform &t = floorEnt.assign<Transform>();

		// Physics
		floorEnt.assign<CollisionShape>(PlaneShape::create(Vec3(0, -1, 0), 0.0f));
		floorEnt.assign<RigidBody>(0.0f);
		//

		model = Builder<Mesh>::build();
		model.addFromFile("test.glb");
		model.flush();

		// Camera
		camera = &eworld.create("camera");
		camera->assign<Camera>(0.02f, 1000.0f, glm::radians(50.0f));

		Transform& camt = camera->assign<Transform>();
		camt.position = Vec3(0, 2.0f, 5.0f);

		// Models
		Material def;
		def.roughness = 0.04f;
		def.metallic = 0.25f;
		def.heightScale = 0.02f;

		const i32 COUNT = 10;
		const i32 COUNT_1 = COUNT > 1 ? COUNT-1 : 1;
		for (i32 i = 0; i < COUNT; i++) {
			float fact = float(i) / float(COUNT_1);

			Entity& mod1 = eworld.create(Util::strCat("dragon_", i));
			mod1.assign<Drawable3D>(model, def);
			Transform& modt = mod1.assign<Transform>();
			modt.rotate(Vec3(0, 0, 1), glm::radians(180.0f));
			modt.rotate(Vec3(0, 1, 0), glm::radians(45.0f));
			modt.position = Vec3((fact * 2.0f - 1.0f) * COUNT * 1.6f, 1.0f, 0);
		}

		// Lights
		Entity& s0 = eworld.create("dir_light0");
		Transform& s0t = s0.assign<Transform>();
		DirectionalLight& s0p = s0.assign<DirectionalLight>();

		s0t.position = Vec3(0.0f, 4.0f, 4.0f);
		s0t.lookAt(s0t.position, Vec3(0.0f), Vec3(0, 1, 0));
		s0p.intensity = 1.0f;
		s0p.shadows = true;
		s0p.shadowFrustumSize = 20.0f;

		ImGui::LoadDock();
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

	void windowResized(u32 w, u32 h) {
		ww = w;
		wh = h;
	}

	void applicationExited() {
		ImGui::SaveDock();
	}

	void gui() {
		ImGui::StyleColorsDark();

		ImGui::RootDock(ImVec2(0, 0), ImVec2(ww, wh));

		ImGui::BeginDock("Scene");
			ImVec2 wsz = ImGui::GetWindowSize();
			wsz.x -= 5;
			wsz.y -= 36;

			if (!screenResizing) {
				ImGui::Image(
							(ImTextureID)sceneFbo.getColorAttachment(0).id(),
							wsz,
							ImVec2(0, 1), ImVec2(1, 0)
				);
			}

			u32 nw = u32(wsz.x), nh = u32(wsz.y);
			viewportW = nw;
			viewportH = nh;

			i32 dx = i32(lastSz.x - wsz.x);
			i32 dy = i32(lastSz.y - wsz.y);
			if (std::abs(dx) > 0 || std::abs(dy) > 0) {
				LogInfo("Resizing...");
				rsys->resizeBuffers(nw, nh);
				sceneFbo.resize(nw, nh);
				screenResizing = true;
			} else {
				screenResizing = false;
			}
			lastSz = wsz;
		ImGui::EndDock();

		ImGui::BeginDock("GBuffer");
		const u32 downscale = 2;
		u32 inW = viewportW;
		u32 inH = viewportH;
		u32 outW = inW / downscale;
		u32 outH = inH / downscale;

		float ri = float(inW) / float(inH);
		float di = float(outW) / float(outH);

		u32 fW = 0, fH = 0;

		if (di > ri) {
			fW = u32(float(inW) * float(outH) / float(inH));
			fH = outH;
		} else {
			fW = outW;
			fH = u32(float(inH) * float(outW) / float(inW));
		}

		ImVec2 sz(fW, fH);

		ImGui::Text("Normals");
		ImGui::Image(
					(ImTextureID)rsys->GBuffer().getColorAttachment(0).id(),
					sz, ImVec2(0, 1), ImVec2(1, 0)
		);

		ImGui::Text("RME");
		ImGui::Image(
					(ImTextureID)rsys->GBuffer().getColorAttachment(2).id(),
					sz, ImVec2(0, 1), ImVec2(1, 0)
		);

		ImGui::Text("Albedo");
		ImGui::Image(
					(ImTextureID)rsys->GBuffer().getColorAttachment(1).id(),
					sz, ImVec2(0, 1), ImVec2(1, 0)
		);

		ImGui::Text("Depth");
		ImGui::Image(
					(ImTextureID)rsys->GBuffer().getDepthAttachment().id(),
					sz, ImVec2(0, 1), ImVec2(1, 0)
		);

		ImGui::EndDock();

		ImGui::BeginDock("Entity World");

		// Entities
		for (uptr<Entity>& e : eworld.entities()) {
			u64 id = e->id();
			String title = e->name().empty() ? Util::strCat("ent_", id) : e->name();
			if (ImGui::CollapsingHeader(title.c_str())) {
				u32 id = 0;
				for (auto const& [k, v] : e->components()) {
					if (ImGui::TreeNode(&k, k.name())) {
						if (k == getTypeIndex<Transform>()) {
							drawTransformEditor(*((Transform*) v.get()));
						} else if (k == getTypeIndex<Camera>()) {
							drawCameraEditor(*((Camera*) v.get()));
						} else if (k == getTypeIndex<Drawable3D>()) {
							drawDrawable3DEditor(title, *((Drawable3D*) v.get()));
						}
						ImGui::TreePop();
					}
					id++;
				}
			}
		}

		ImGui::EndDock();
	}

	void render() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		eworld.render(&sceneFbo);

//		sceneFbo.bind();

//		Mat4 projMat(1.0f);
//		Mat4 viewMat(1.0f);
//		if (camera) {
//			Camera *cam = camera->get<Camera>();
//			projMat = cam->getProjection(viewportW == 0 ? ww : viewportW, viewportH == 0 ? wh : viewportH);
//			if (camera->has<Transform>()) {
//				Transform* t = camera->get<Transform>();
//				Quat qRot = glm::conjugate(t->worldRotation());
//				Mat4 rot = glm::mat4_cast(qRot);
//				Mat4 loc = glm::translate(Mat4(1.0f), t->worldPosition() * -1.0f);
//				viewMat = rot * loc;
//			}
//		}

//		sceneFbo.unbind();
	}

	Mesh model;
	Texture rme, alb0, nrm, disp;

	Entity* camera;
	float sensitivity;
	bool mouseLocked, screenResizing;

	RendererSystem* rsys;
	EntityWorld eworld;
	float t;

	FrameBuffer sceneFbo;

	u32 ww, wh;
	u32 viewportW, viewportH;
	ImVec2 lastSz;
};

int main(int argc, char** argv) {
	ApplicationConfig conf;
	conf.width = 960;
	conf.height = 640;
	conf.notifyResize = false;
	conf.maximized = true;

	Application app(new TestApp(), conf);
	app.run();
	return 0;
}
