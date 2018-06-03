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
#include "imgui/imgui_internal.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "math/glm/gtx/matrix_decompose.hpp"

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
		ImGuizmo::SetOrthographic(false);
	} else {
		float os = t.orthoScale;
		if (ImGui::InputFloat("Scale", &os, 0.01f, 0.1f)) {
			t.orthoScale = os;
		}
		ImGuizmo::SetOrthographic(true);
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

void editTransform(
		const Mat4& cameraView,
		const Mat4& cameraProjection,
		Transform& tr,
		const AABB& aabb,
		u32 vx, u32 vy, u32 vw, u32 vh
) {
	ImGui::Begin("Manipulator", nullptr,
				 ImGuiWindowFlags_NoCollapse |
				 ImGuiWindowFlags_NoResize |
				 ImGuiWindowFlags_AlwaysAutoResize
				 );
		static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
		static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
		static bool useSnap = false;
		static float snap[] = { 1.f, 1.f, 1.f };
		static float bounds[] = { aabb.min().x, aabb.min().y, aabb.min().z, aabb.max().x, aabb.max().y, aabb.max().z };
		static float boundsSnap[] = { 0.1f, 0.1f, 0.1f };
		static bool boundSizing = false;
		static bool boundSizingSnap = false;

		if (ImGui::RadioButton("Tr", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
			mCurrentGizmoOperation = ImGuizmo::TRANSLATE;

		ImGui::SameLine();

		if (ImGui::RadioButton("Rt", mCurrentGizmoOperation == ImGuizmo::ROTATE))
			mCurrentGizmoOperation = ImGuizmo::ROTATE;

		ImGui::SameLine();

		if (ImGui::RadioButton("Sc", mCurrentGizmoOperation == ImGuizmo::SCALE))
			mCurrentGizmoOperation = ImGuizmo::SCALE;

		if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
			if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
				mCurrentGizmoMode = ImGuizmo::LOCAL;
			ImGui::SameLine();
			if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
				mCurrentGizmoMode = ImGuizmo::WORLD;
		}

		ImGui::Checkbox("", &useSnap);
		ImGui::SameLine();

		switch (mCurrentGizmoOperation) {
			case ImGuizmo::TRANSLATE:
				ImGui::InputFloat3("Snap", &snap[0]);
				break;
			case ImGuizmo::ROTATE:
				ImGui::InputFloat("Angle Snap", &snap[0]);
				break;
			case ImGuizmo::SCALE:
				ImGui::InputFloat("Scale Snap", &snap[0]);
				break;
		}

		ImGui::Checkbox("Bound Sizing", &boundSizing);
		if (boundSizing) {
			ImGui::PushID(3);
			ImGui::Checkbox("", &boundSizingSnap);
			ImGui::SameLine();
			ImGui::InputFloat3("Snap", boundsSnap);
			ImGui::PopID();
		}

		ImGuizmo::SetRect(vx, vy, vw, vh);

		Mat4 matrix = tr.getTransformation();

		ImGui::PushClipRect(ImVec2(vx, vy), ImVec2(vx+vw, vy+vh), false);
		ImGuizmo::Manipulate(
					glm::value_ptr(cameraView),
					glm::value_ptr(cameraProjection),
					mCurrentGizmoOperation,
					mCurrentGizmoMode,
					glm::value_ptr(matrix),
					NULL,
					useSnap ? &snap[0] : NULL,
					boundSizing ? bounds : NULL,
					boundSizingSnap ? boundsSnap : NULL
		);
		ImGui::PopClipRect();

		tr.setFromMatrix(matrix);
	ImGui::End();
}

#define ICON_LIGHT 0
#define ICON_CAMERA 1
#define ICON_OBJECT 2

#define ICON_SIZE 0.15f

class TestApp : public IApplicationAdapter {
public:
	void init() {
		VFS::get().mountDefault(); // mounts to where the application resides

		sensitivity = 0.005f;

		rsys = &eworld.registerSystem<RendererSystem>(config.width, config.height);

		sceneFbo = Builder<FrameBuffer>::build()
				.setSize(config.width, config.height)
				.addRenderBuffer(TextureFormat::Depthf, Attachment::DepthAttachment)
				.addColorAttachment(TextureFormat::RGB, TextureTarget::Texture2D)
				.addDepthAttachment();

		cameraFbo = Builder<FrameBuffer>::build()
					.setSize(config.width, config.height)
					.addColorAttachment(TextureFormat::RGB, TextureTarget::Texture2D);

		psys = &eworld.registerSystem<PhysicsSystem>();

		Texture envMap = Builder<Texture>::build()
				.bind(TextureTarget::CubeMap)
				.setCubemap("skybox.jpg")
				.generateMipmaps();

		rsys->setEnvironmentMap(envMap);

		icons = Builder<Texture>::build()
				.bind(TextureTarget::Texture2D)
				.setFromFile("icons.png")
				.generateMipmaps();

		// Floor
		Material floorMat;
		floorMat.baseColor = Vec3(0.7f, 1.0f, 0.7f);
		floorMat.metallic = 0.0f;
		floorMat.roughness = 1.0f;

		Mesh floor = Builder<Mesh>::build();
		floor.addPlane(Axis::Y, 32.0f, Vec3(0.0f)).calculateNormals().calculateTangents().flush();

		Entity& floorEnt = eworld.create("ground");
		floorEnt.assign<Drawable3D>(floor, floorMat);
		floorEnt.assign<Transform>();

		// Physics
		floorEnt.assign<CollisionShape>(PlaneShape::create(Vec3(0, -1, 0), 0.0f));
		floorEnt.assign<RigidBody>(0.0f);
		//

		model = Builder<Mesh>::build();
		model.addFromFile("fcube.obj");
		model.flush();

		// Default Camera
		defaultCamera = uptr<Entity>(new Entity());
		defaultCamera->assign<Camera>(0.02f, 100.0f, glm::radians(45.0f));
		Transform &dct = defaultCamera->assign<Transform>();
		dct.position.z = 10.0f;
		dct.position.y = 0.0f;

		cameraPivot = uptr<Entity>(new Entity());
		Transform &piv = cameraPivot->assign<Transform>();
		dct.setParent(&piv);
		//

		// Example camera
		Entity &cam = eworld.create("camera");
		cam.assign<Camera>(0.1f, 50.0f, glm::radians(50.0f));
		Transform& ct = cam.assign<Transform>();
		ct.position.z = 4.0f;

		// Models
		Material def;
		def.roughness = 0.04f;
		def.metallic = 0.25f;
		def.heightScale = 0.02f;

		ShapeWrapper sw = BoxShape::create(Vec3(1.0f));

		const i32 COUNT = 10;
		const i32 COUNT_1 = COUNT > 1 ? COUNT-1 : 1;
		for (i32 i = 0; i < COUNT; i++) {
			float fact = float(i) / float(COUNT_1);

			Entity& mod1 = eworld.create(Util::strCat("box_", i));
			mod1.assign<Drawable3D>(model, def);
			Transform& modt = mod1.assign<Transform>();
			modt.rotate(Vec3(0, 0, 1), glm::radians(180.0f));
			modt.rotate(Vec3(0, 1, 0), glm::radians(45.0f));
			modt.position = Vec3((fact * 2.0f - 1.0f) * COUNT * 1.6f, 1.2f, 0);

			mod1.assign<CollisionShape>(sw);
			mod1.assign<RigidBody>(2.0f);
		}

		// Lights
		Entity& s0 = eworld.create("dir_light0");
		Transform& s0t = s0.assign<Transform>();
		DirectionalLight& s0p = s0.assign<DirectionalLight>();

		s0t.position = Vec3(0.0f, 4.0f, 4.0f);
		s0t.rotate(Vec3(1, 0, 0), glm::radians(45.0f));
		s0p.intensity = 1.0f;
		s0p.shadows = true;
		s0p.shadowFrustumSize = 20.0f;

		Entity& s1 = eworld.create("spot_light0");
		Transform& s1t = s1.assign<Transform>();
		SpotLight& s1p = s1.assign<SpotLight>();

		s1t.position = Vec3(-4.0f, 4.0f, 4.0f);
		s1t.rotate(Vec3(1, 0, 0), glm::radians(45.0f));
		s1p.intensity = 1.0f;
		s1p.shadows = true;
		s1p.spotCutOff = glm::radians(30.0f);

		ImGui::LoadDock();
		Imm::initialize();
	}

	void update(float timeDelta) {
		if (Input::isMouseButtonPressed(SDL_BUTTON_RIGHT) && mouseLocked) {
			Vec2 mpos = Input::getMousePosition();
			mpos.x -= viewportX;
			mpos.y -= viewportY;
			rsys->pickingBuffer().bind(FrameBufferTarget::ReadFramebuffer, Attachment::ColorAttachment);

			u8 e[3] = { 255, 255, 255 };

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glReadPixels(
						i32(mpos.x),
						viewportH - i32(mpos.y),
						1, 1,
						GL_RGB,
						GL_UNSIGNED_BYTE,
						e
			);

			// Reconstruct entity ID
			u64 eid = ((e[0] & 0xFF) << 16) | ((e[1] & 0xFF) << 8) | (e[2] & 0xFF);
			selected = eworld.getEntity(eid);

			rsys->pickingBuffer().unbind();
		}

		if (Input::isMouseButtonDown(SDL_BUTTON_MIDDLE) && mouseLocked) {
			Transform* piv = cameraPivot->get<Transform>();
			Vec2 cen(config.width/2, config.height/2);
			Vec2 mp = Input::getMousePosition();

			Vec2 delta = (cen - mp) * sensitivity;
			piv->rotate(Vec3(0, 1, 0), -delta.x);
			piv->rotate(Vec3(1, 0, 0), delta.y);

			Input::setMousePosition(cen);
		}

		i32 scr = Input::getScrollOffset();
		if (std::abs(scr) > 0 && mouseLocked) {
			Transform* cam = defaultCamera->get<Transform>();
			float fac = -0.3f * float(scr);
			cam->position.z += fac;
		}

		if (playing) {
			eworld.update(timeDelta);
		} else {
			eworld.each([&](Entity& ent, Transform& t, RigidBody& b) {
				Vec3 pos = t.worldPosition();
				Quat rot = t.worldRotation();
				btTransform xform(
						btQuaternion(rot.x, rot.y, rot.z, rot.w),
						btVector3(pos.x, pos.y, pos.z)
				);
				if (b.rigidBody()) {
					b.rigidBody()->setWorldTransform(xform);
					b.rigidBody()->getMotionState()->setWorldTransform(xform);
				}
			});
		}
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

		float docky = 20.0f;
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Exit", "CTRL+Q")) {
					MessageSystem::get().submit("app_quit");
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Game")) {
				if (!playing) {
					if (ImGui::MenuItem("Play", "CTRL+P")) {
						playing = true;
						psys->bulletWorld()->clearForces();
						btCollisionObjectArray obs = psys->bulletWorld()->getCollisionObjectArray();
						for (u32 i = 0; i < obs.size(); i++) {
							btCollisionObject* ob = obs.at(i);
							ob->activate(true);
						}
					}
				} else {
					if (ImGui::MenuItem("Stop", "CTRL+P")) {
						playing = false;
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		ImGui::RootDock(ImVec2(0, docky), ImVec2(ww, wh-docky));

		mouseLocked = false;
		if (ImGui::BeginDock("Scene")) {
			mouseLocked = true;

			ImVec2 wps = ImGui::GetCursorScreenPos();
			ImVec2 wsz = ImGui::GetWindowSize();
			wsz.x -= 5;
			wsz.y -= 36;

			u32 nw = u32(wsz.x), nh = u32(wsz.y);
			viewportX = u32(wps.x);
			viewportY = u32(wps.y);
			viewportW = nw;
			viewportH = nh;

			if (!screenResizing) {
				ImGui::Image(
							(ImTextureID)sceneFbo.getColorAttachment(0).id(),
							wsz,
							ImVec2(0, 1), ImVec2(1, 0)
				);

				if (selected && selected->has<Camera>()) {
					ImVec2 sz(nw / 5, nh / 5);
					ImVec2 a(wsz.x - sz.x - 20, wsz.y - sz.y - 10);
					ImVec2 b(wsz.x - 20, wsz.y - 10);

					ImGui::GetWindowDrawList()->AddRectFilled(a-ImVec2(5,5), b+ImVec2(5,5), 0xAA000000, 0.2f);
					ImGui::GetWindowDrawList()->AddImageRounded(
								(ImTextureID)cameraFbo.getColorAttachment(0).id(),
								a, b, ImVec2(1, 0), ImVec2(0, 1), 0xFFFFFFFF, 0.2f
					);
				}
			}

			i32 dx = i32(lastSz.x - wsz.x);
			i32 dy = i32(lastSz.y - wsz.y);
			if (std::abs(dx) > 0 || std::abs(dy) > 0) {
				LogInfo("Resizing...");
				rsys->resizeBuffers(nw, nh);
				sceneFbo.resize(nw, nh);
				cameraFbo.resize(nw, nh);
				screenResizing = true;
			} else {
				screenResizing = false;
			}
			lastSz = wsz;

			// Gizmo
			Mat4 projMat(1.0f);
			Mat4 viewMat(1.0f);
			Camera *cam = defaultCamera->get<Camera>();
			projMat = cam->getProjection(viewportW == 0 ? ww : viewportW, viewportH == 0 ? wh : viewportH);
			if (defaultCamera->has<Transform>()) {
				Transform* t = defaultCamera->get<Transform>();
				viewMat = glm::inverse(t->getTransformation());
			}
			if (selected) {
				if (selected->has<Transform>()) {
					AABB aabb(Vec3(-0.5f), Vec3(0.5f));
					if (selected->has<Drawable3D>()) {
						aabb = selected->get<Drawable3D>()->mesh.aabb();
					}
					editTransform(
							viewMat,
							projMat,
							*selected->get<Transform>(),
							aabb,
							viewportX, viewportY, viewportW, viewportH
					);
				}
			}
		}
		ImGui::EndDock();

		if (ImGui::BeginDock("GBuffer")) {
			const u32 downscale = 4;
			u32 inW = rsys->finalBuffer().width();
			u32 inH = rsys->finalBuffer().height();
			u32 outW = inW / downscale;
			u32 outH = inH / downscale;

			ImVec2 sz(outW, outH);

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

			u32 sw = rsys->shadowBuffer().width() / downscale;
			u32 sh = rsys->shadowBuffer().height() / downscale;
			ImGui::Text("Shadow Map");
			ImGui::Image(
						(ImTextureID)rsys->shadowBuffer().getDepthAttachment().id(),
						ImVec2(sw, sh), ImVec2(0, 1), ImVec2(1, 0)
			);
		}
		ImGui::EndDock();

		if (ImGui::BeginDock("Entity World")) {
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
		}
		ImGui::EndDock();
	}

	void render() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		eworld.render(&sceneFbo, defaultCamera.get());

		sceneFbo.bind();

		if (playing)
			psys->bulletWorld()->debugDrawWorld();

		eworld.each([&](Entity& ent, Transform& t, Drawable3D& d) {
			Vec3 he = ((d.mesh.aabb().max() - d.mesh.aabb().min()) * 0.5f) * t.scale;

			Vec3 cubePos[] = {
				Vec3(-1.0f, -1.0f, -1.0f),
				Vec3(1.0f, -1.0f, -1.0f),
				Vec3(1.0f, 1.0f, -1.0f),
				Vec3(-1.0f, 1.0f, -1.0f),
				Vec3(-1.0f, -1.0f, 1.0f),
				Vec3(1.0f, -1.0f, 1.0f),
				Vec3(1.0f, 1.0f, 1.0f),
				Vec3(-1.0f, 1.0f, 1.0f)
			};
			u32 cubeInd[] = {
				0, 4, 1, 5,
				2, 6, 3, 7,
				0, 3, 1, 2,
				4, 7, 5, 6,
				0, 1, 2, 3,
				4, 5, 6, 7
			};

			const Vec4 cubecol(0.6f);

			Imm::begin(PrimitiveType::Lines);
			Imm::setModel(t.getTransformation());
			for (u32 i = 0; i < 24; i++) {
				Imm::vertex(cubePos[cubeInd[i]]*he, cubecol);
			}
			Imm::end();

		});

		eworld.each([&](Entity& ent, Transform& t, Drawable3D& d) {
			Imm::begin(PrimitiveType::Triangles);
			Imm::disableDepth();
			Imm::texture(icons);
			Imm::billboardAtlas(t.worldPosition(), 8, 8, ICON_OBJECT, ICON_SIZE+0.05f, Vec4(0.0f, 1.0f, 1.0f, 0.5f));
			Imm::end();
		});

		eworld.each([&](Entity& ent, Transform& t, Camera& cam) {
			float hn, wn, hf, wf, n = cam.zNear, f = cam.zFar;
			if (cam.type == CameraType::Perspective) {
				float ratio = viewportH > 0 ? float(viewportW / viewportH) : 1.0f;
				float tthf = 2.0f * std::tan(cam.FOV / 2.0f);
				hn = tthf * cam.zNear;
				wn = hn * ratio;
				hf = tthf * cam.zFar;
				wf = hf * ratio;
			} else {
				wn = wf = cam.orthoScale * 2.0f;
				hn = hf = cam.orthoScale * 2.0f;
			}

			wn *= 0.5f;
			wf *= 0.5f;
			hn *= 0.5f;
			hf *= 0.5f;

			Vec3 cubePos[] = {
				Vec3(-wn, -hn, -n),
				Vec3(wn, -hn, -n),
				Vec3(wn, hn, -n),
				Vec3(-wn, hn, -n),
				Vec3(-wf, -hf, -f),
				Vec3(wf, -hf, -f),
				Vec3(wf, hf, -f),
				Vec3(-wf, hf, -f)
			};

			u32 cubeInd[] = {
				0, 4, 1, 5,
				2, 6, 3, 7,
				0, 3, 1, 2,
				4, 7, 5, 6,
				0, 1, 2, 3,
				4, 5, 6, 7
			};

			const Vec4 cubecol(0.6f);

			Imm::begin(PrimitiveType::Lines);
			Imm::setModel(t.getTransformation());
			for (u32 i = 0; i < 24; i++) {
				Imm::vertex(cubePos[cubeInd[i]], cubecol);
			}
			Imm::end();

			Imm::begin(PrimitiveType::Triangles);
			Imm::disableDepth();
			Imm::texture(icons);
			Imm::billboardAtlas(t.worldPosition(), 8, 8, ICON_CAMERA, ICON_SIZE, Vec4(1.0f, 1.0f, 1.0f, 0.5f));
			Imm::end();
		});

		eworld.each([&](Entity& ent, Transform& t, DirectionalLight& l) {
			Mat4 viewMatLight = t.getTransformation();

			Imm::begin(PrimitiveType::Triangles);
			Imm::setModel(viewMatLight);
			Imm::arrow(0.25f, Vec4(1.0f, 1.0f, 0.0f, 0.5f), 0.15f);
			Imm::end();

			Imm::begin(PrimitiveType::Triangles);
			Imm::disableDepth();
			Imm::texture(icons);
			Imm::billboardAtlas(t.worldPosition(), 8, 8, ICON_LIGHT, ICON_SIZE, Vec4(1.0f, 1.0f, 1.0f, 0.5f));
			Imm::end();
		});

		eworld.each([&](Entity& ent, Transform& t, SpotLight& l) {
			Mat4 viewMatLight = t.getTransformation();

			Imm::begin(PrimitiveType::Triangles);
			Imm::setModel(viewMatLight);
			Imm::cone(
						l.spotCutOff*l.radius*2.0f,
						l.radius*2.0f,
						Vec4(1.0f, 1.0f, 1.0f, 0.15f),
						Vec3(0.0f, 0.0f, -l.radius*2.0f),
						16, true
			);
			Imm::end();

			Imm::begin(PrimitiveType::Triangles);
			Imm::disableDepth();
			Imm::texture(icons);
			Imm::billboardAtlas(t.worldPosition(), 8, 8, ICON_LIGHT, ICON_SIZE, Vec4(1.0f, 1.0f, 1.0f, 0.5f));
			Imm::end();
		});

		Mat4 projMat(1.0f);
		Mat4 viewMat(1.0f);
		Camera *cam = defaultCamera->get<Camera>();
		projMat = cam->getProjection(viewportW == 0 ? ww : viewportW, viewportH == 0 ? wh : viewportH);
		if (defaultCamera->has<Transform>()) {
			Transform* t = defaultCamera->get<Transform>();
			Quat qRot = glm::conjugate(t->worldRotation());
			Mat4 rot = glm::mat4_cast(qRot);
			Mat4 loc = glm::translate(Mat4(1.0f), t->worldPosition() * -1.0f);
			viewMat = rot * loc;
		}
		Imm::render(viewMat, projMat);
		sceneFbo.unbind();

		if (selected && selected->has<Camera>()) {
			eworld.render(&cameraFbo, selected);
		}
	}

	Mesh model;
	Texture icons;

	Entity* selected;
	uptr<Entity> defaultCamera;
	uptr<Entity> cameraPivot;

	float sensitivity;
	bool mouseLocked, screenResizing, playing;

	RendererSystem* rsys;
	PhysicsSystem* psys;
	EntityWorld eworld;
	float t;

	FrameBuffer sceneFbo, cameraFbo;

	u32 ww, wh;
	u32 viewportX, viewportY, viewportW, viewportH;
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
