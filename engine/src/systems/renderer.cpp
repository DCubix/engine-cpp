#include "renderer.h"

#include <vector>

NS_BEGIN

Mat4 Camera::getProjection(u32 width, u32 height) {
	width = width == 0 ? 1 : width;
	height = height == 0 ? 1 : height;
	if (type == CameraType::Orthographic) {
		return glm::ortho(-orthoScale, orthoScale, -orthoScale, orthoScale, zNear, zFar);
	}
	return glm::perspective(FOV, float(width) / float(height), zNear, zFar);
}

const String RendererSystem::POST_FX_VS =
#include "../shaders/lightingV.glsl"
;

RendererSystem::RendererSystem() : RendererSystem(640, 480) {
}

RendererSystem::RendererSystem(u32 width, u32 height) {
	m_IBLGenerated = false;
	m_renderWidth = width;
	m_renderHeight = height;
	m_pov = nullptr;
	m_materialID = 0;

	m_gbuffer = Builder<FrameBuffer>::build()
			.setSize(width, height)
			.addRenderBuffer(TextureFormat::Depthf, Attachment::DepthAttachment)
			.addColorAttachment(TextureFormat::RGf) // Normals
			.addColorAttachment(TextureFormat::RGB) // Albedo
			.addColorAttachment(TextureFormat::RGB) // RME
			.addDepthAttachment();

	m_finalBuffer = Builder<FrameBuffer>::build()
			.setSize(width, height)
			.addColorAttachment(TextureFormat::RGBf)
			.addDepthAttachment();

	m_pingPongBuffer = Builder<FrameBuffer>::build()
			.setSize(width, height)
			.addColorAttachment(TextureFormat::RGBf)
			.addColorAttachment(TextureFormat::RGBf);

	m_captureBuffer = Builder<FrameBuffer>::build()
			.setSize(128, 128)
			.addRenderBuffer(TextureFormat::Depthf, Attachment::DepthAttachment);

	m_pickingBuffer = Builder<FrameBuffer>::build()
			.setSize(width, height)
			.addColorAttachment(TextureFormat::RGB);

	m_shadowBuffer = Builder<FrameBuffer>::build()
			.setSize(2048, 2048)
			.addRenderBuffer(TextureFormat::Depthf, Attachment::DepthAttachment)
			.addDepthAttachment();

	String common =
#include "../shaders/common.glsl"
			;
	String brdf =
#include "../shaders/brdf.glsl"
			;

	String gVS =
#include "../shaders/gbufferV.glsl"
			;
	String gFS =
#include "../shaders/gbufferF.glsl"
			;

	gFS = Util::replace(gFS, "#include common", common);
	gFS = Util::replace(gFS, "#include brdf", brdf);

	m_gbufferShader = Builder<ShaderProgram>::build()
			.add(gVS, ShaderType::VertexShader)
			.add(gFS, ShaderType::FragmentShader);
	m_gbufferShader.link();

	String giVS =
#include "../shaders/gbufferInstV.glsl"
			;

	m_gbufferInstancedShader = Builder<ShaderProgram>::build()
			.add(giVS, ShaderType::VertexShader)
			.add(gFS, ShaderType::FragmentShader);
	m_gbufferInstancedShader.link();

	String lVS =
#include "../shaders/lightingV.glsl"
			;
	String lFS =
#include "../shaders/lightingF.glsl"
			;

	lVS = Util::replace(lVS, "#include common", common);
	lFS = Util::replace(lFS, "#include common", common);
	lFS = Util::replace(lFS, "#include brdf", brdf);

	m_lightingShader = Builder<ShaderProgram>::build()
			.add(lVS, ShaderType::VertexShader)
			.add(lFS, ShaderType::FragmentShader);
	m_lightingShader.link();

	String fVS =
#include "../shaders/lightingV.glsl"
			;
	String fFS =
#include "../shaders/screenF.glsl"
			;

	fVS = Util::replace(fVS, "#include common", common);
	fFS = Util::replace(fFS, "#include common", common);
	fFS = Util::replace(fFS, "#include brdf", brdf);

	m_finalShader = Builder<ShaderProgram>::build()
			.add(fVS, ShaderType::VertexShader)
			.add(fFS, ShaderType::FragmentShader);
	m_finalShader.link();

	String cmVS =
#include "../shaders/cmV.glsl"
			;
	String cmFS =
#include "../shaders/cmF.glsl"
			;
	m_cubeMapShader = Builder<ShaderProgram>::build()
			.add(cmVS, ShaderType::VertexShader)
			.add(cmFS, ShaderType::FragmentShader);
	m_cubeMapShader.link();

	String cmiVS =
#include "../shaders/cmIrradianceV.glsl"
			;
	String cmiFS =
#include "../shaders/cmIrradianceF.glsl"
			;
	m_irradianceShader = Builder<ShaderProgram>::build()
			.add(cmiVS, ShaderType::VertexShader)
			.add(cmiFS, ShaderType::FragmentShader);
	m_irradianceShader.link();

	String cmfVS =
#include "../shaders/cmIrradianceV.glsl"
			;
	String cmfFS =
#include "../shaders/cmPreFilterF.glsl"
			;
	m_preFilterShader = Builder<ShaderProgram>::build()
			.add(cmfVS, ShaderType::VertexShader)
			.add(cmfFS, ShaderType::FragmentShader);
	m_preFilterShader.link();

	String brdfFS =
#include "../shaders/brdfLUTF.glsl"
			;

	m_brdfLUTShader = Builder<ShaderProgram>::build()
			.add(fVS, ShaderType::VertexShader)
			.add(brdfFS, ShaderType::FragmentShader);
	m_brdfLUTShader.link();

	String pickFS =
#include "../shaders/pickingF.glsl"
			;
	m_pickingShader = Builder<ShaderProgram>::build()
			.add(gVS, ShaderType::VertexShader)
			.add(pickFS, ShaderType::FragmentShader);
	m_pickingShader.link();

	String sVS =
#include "../shaders/shadowV.glsl"
	;
	String sFS =
#include "../shaders/shadowF.glsl"
	;
	m_shadowShader = Builder<ShaderProgram>::build()
			.add(sVS, ShaderType::VertexShader)
			.add(sFS, ShaderType::FragmentShader);
	m_shadowShader.link();

	String siVS =
#include "../shaders/shadowInstV.glsl"
	;

	m_shadowInstancedShader = Builder<ShaderProgram>::build()
			.add(siVS, ShaderType::VertexShader)
			.add(sFS, ShaderType::FragmentShader);
	m_shadowInstancedShader.link();

	m_plane = Builder<Mesh>::build();
	m_plane.addVertex(Vertex(Vec3(0, 0, 0)))
		.addVertex(Vertex(Vec3(1, 0, 0)))
		.addVertex(Vertex(Vec3(1, 1, 0)))
		.addVertex(Vertex(Vec3(0, 1, 0)))
		.addTriangle(0, 1, 2)
		.addTriangle(2, 3, 0)
		.flush();

	m_cube = Builder<Mesh>::build();
	m_cube.addCube(1.0f);
	m_cube.flush();

	m_instanceBuffer = Builder<VertexBuffer>::build();
	m_instanceBuffer.bind(BufferType::ArrayBuffer).unbind();

	m_screenMipSampler = Builder<Sampler>::build()
			.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);

	m_screenTextureSampler = Builder<Sampler>::build()
			.setFilter(TextureFilter::Linear, TextureFilter::Linear)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);

	m_screenDepthSampler = Builder<Sampler>::build()
			.setFilter(TextureFilter::Nearest, TextureFilter::Nearest)
			.setWrap(TextureWrap::ClampToBorder, TextureWrap::ClampToBorder)
			.setBorderColor(1, 1, 1, 1);

	m_cubeMapSampler = Builder<Sampler>::build()
			.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge)
			.setSeamlessCubemap(true);

	m_cubeMapSamplerNoMip = Builder<Sampler>::build()
			.setFilter(TextureFilter::Linear, TextureFilter::Linear)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge)
			.setSeamlessCubemap(true);
}

void RendererSystem::update(EntityWorld& world, float dt) {
	m_time += dt;
}

void RendererSystem::render(EntityWorld& world, FrameBuffer* target, Entity* pov) {
	Entity* _pov = pov == nullptr ? m_pov : pov;
	if (_pov == nullptr) {
		Entity *cameraEnt = world.find<Camera>();
		if (cameraEnt && cameraEnt->has<Transform>()) {
			_pov = m_pov = cameraEnt;
		}
	}
	m_pov = _pov;

	if (_pov == nullptr) return;

	Mat4 projMat = _pov->get<Camera>()->getProjection(m_renderWidth, m_renderHeight);

	Transform *camT = _pov->get<Transform>();
	Mat4 viewMat(1.0f);
	Quat qRot = glm::conjugate(camT->worldRotation());
	Mat4 rot = glm::mat4_cast(qRot);
	Mat4 loc = glm::translate(Mat4(1.0f), camT->worldPosition() * -1.0f);
	viewMat = rot * loc;

	if (!m_IBLGenerated && m_envMap.id() != 0) {
		computeIBL();
		m_IBLGenerated = true;
	}

	// Get all meshes
	Vector<RenderMesh> renderMeshes;

	world.each([&](Entity &ent, Transform &T, Drawable3D &D) {
		RenderMesh rm;
		rm.mesh = D.mesh;
		rm.materialID = D.materialID;
		rm.modelMatrix = T.getTransformation();
		if (ent.has<Texturer>()) {
			rm.texturer = *ent.get<Texturer>();
		}
		renderMeshes.push_back(rm);
	});

	pickingPass(world, projMat, viewMat, renderMeshes);
	gbufferPass(projMat, viewMat, renderMeshes);
	lightingPass(world, projMat, viewMat, renderMeshes);

	finalPass(projMat, viewMat, renderMeshes, target);

	// Immediate geom
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RendererSystem::messageReceived(EntityWorld& world, const Message& msg) {
	if (msg.type == "app_window_resized") {
		Vec2 size = *((Vec2*)msg.data);
		resizeBuffers(i32(size.x), i32(size.y));
	}
}

void RendererSystem::resizeBuffers(u32 width, u32 height) {
	glViewport(0, 0, width, height);

	m_finalBuffer.resize(width, height);
	m_pickingBuffer.resize(width, height);
	m_pingPongBuffer.resize(width, height);
	m_gbuffer.resize(width, height);

	m_renderWidth = width;
	m_renderHeight = height;
}

void RendererSystem::computeIrradiance() {
	if (m_envMap.id() == 0) return;

	const Mat4 capProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	const Mat4 capViews[6] = {
		glm::lookAt(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, -1, 0)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(-1, 0, 0), Vec3(0, -1, 0)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(0, -1, 0), Vec3(0, 0, -1)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(0, 0, 1), Vec3(0, -1, 0)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(0, 0, -1), Vec3(0, -1, 0))
	};

	if (m_irradiance.id() != 0) {
		Builder<Texture>::destroy(m_irradiance);
	}
	m_irradiance = Builder<Texture>::build()
			.bind(TextureTarget::CubeMap)
			.setCubemapNull(32, 32, TextureFormat::RGBf);

	m_irradianceShader.bind();
	m_irradianceShader.get("mProjection").set(capProj);

	m_envMap.bind(m_cubeMapSampler, 0);
	m_irradianceShader.get("tCubeMap").set(0);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	m_captureBuffer.bind();
	glViewport(0, 0, 32, 32);
	m_cube.bind();
	for (u32 i = 0; i < 6; i++) {
		m_irradianceShader.get("mView").set(capViews[i]);
		m_captureBuffer.setColorAttachment(0, (TextureTarget)(TextureTarget::CubeMapPX+i), m_irradiance);
		clear(ClearBufferMask::ColorBuffer | ClearBufferMask::DepthBuffer);
		m_cube.drawIndexed(PrimitiveType::Triangles, 0);
	}
	m_cube.unbind();
	m_captureBuffer.unbind();

	m_irradianceShader.unbind();

	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void RendererSystem::computeRadiance() {
	if (m_envMap.id() == 0) return;

	const Mat4 capProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	const Mat4 capViews[6] = {
		glm::lookAt(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, -1, 0)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(-1, 0, 0), Vec3(0, -1, 0)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(0, -1, 0), Vec3(0, 0, -1)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(0, 0, 1), Vec3(0, -1, 0)),
		glm::lookAt(Vec3(0, 0, 0), Vec3(0, 0, -1), Vec3(0, -1, 0))
	};

	const u32 maxMip = 8;

	if (m_radiance.id() != 0) {
		Builder<Texture>::destroy(m_radiance);
	}
	m_radiance = Builder<Texture>::build()
			.bind(TextureTarget::CubeMap)
			.setCubemapNull(128, 128, TextureFormat::RGBf)
			.generateMipmaps();

	m_preFilterShader.bind();
	m_preFilterShader.get("mProjection").set(capProj);

	m_envMap.bind(m_cubeMapSampler, 0);
	m_preFilterShader.get("tCubeMap").set(0);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	m_captureBuffer.bind();
	m_cube.bind();
	for (u32 i = 0; i < maxMip; i++) {
		u32 mipWidth = u32(128 * std::pow(0.5, i));
		u32 mipHeight = u32(128 * std::pow(0.5, i));

		m_captureBuffer.setRenderBufferStorage(TextureFormat::Depthf, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = float(i) / float(maxMip - 1);
		m_preFilterShader.get("uRoughness").set(roughness);

		for (u32 j = 0; j < 6; j++) {
			m_preFilterShader.get("mView").set(capViews[j]);
			m_captureBuffer.setColorAttachment(0, (TextureTarget)(TextureTarget::CubeMapPX+j), m_radiance, i);
			clear(ClearBufferMask::ColorBuffer | ClearBufferMask::DepthBuffer);
			m_cube.drawIndexed(PrimitiveType::Triangles, 0);
		}
	}
	m_cube.unbind();
	m_captureBuffer.unbind();

	m_preFilterShader.unbind();

	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void RendererSystem::computeBRDF() {
	if (m_envMap.id() == 0) return;

	if (m_brdf.id() != 0) {
		Builder<Texture>::destroy(m_brdf);
	}
	m_brdf = Builder<Texture>::build()
			.bind(TextureTarget::Texture2D)
			.setNull(512, 512, TextureFormat::RGf);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	m_captureBuffer.bind();
	m_captureBuffer.setRenderBufferStorage(TextureFormat::Depthf, 512, 512);
	glViewport(0, 0, 512, 512);

	m_plane.bind();
	m_brdfLUTShader.bind();

	m_captureBuffer.setColorAttachment(0, TextureTarget::Texture2D, m_brdf);
	clear(ClearBufferMask::ColorBuffer | ClearBufferMask::DepthBuffer);
	m_plane.drawIndexed(PrimitiveType::Triangles, 0);

	m_brdfLUTShader.unbind();
	m_captureBuffer.unbind();
	m_plane.unbind();

	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void RendererSystem::computeIBL() {
	computeIrradiance();
	computeRadiance();
	computeBRDF();
}

void RendererSystem::pickingPass(EntityWorld& world, const Mat4& projection, const Mat4& view, const Vector<RenderMesh>& renderables) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_GREATER);

	/// Fill picking buffer
	m_pickingBuffer.bind();
	clear(ClearBufferMask::ColorBuffer | ClearBufferMask::DepthBuffer, 1, 1, 1, 1);

	m_pickingShader.bind();
	m_pickingShader.get("mProjection").set(projection);
	m_pickingShader.get("mView").set(view);

	// Render everything that has a transform
	m_cube.bind();
	world.each([&](Entity &ent, Transform &T) {
		Mat4 modelMat = T.getTransformation();

		// Scale the cube to the same size of the object
		Vec3 scale = Vec3(1.0f);
		Vec3 center(0.0f);
		if (ent.has<Drawable3D>()) {
			Drawable3D *drw = ent.get<Drawable3D>();
			center = (drw->mesh.aabb().max() + drw->mesh.aabb().min()) * 0.5f;
			scale = (drw->mesh.aabb().max() - drw->mesh.aabb().min()) * 0.5f;
		} else {
			scale = Vec3(0.1f);
		}
		modelMat = glm::translate(modelMat, center);
		modelMat = glm::scale(modelMat, scale);

		m_pickingShader.get("mModel").set(modelMat);
		m_pickingShader.get("uEID").set(ent.id());
		m_cube.drawIndexed(PrimitiveType::Triangles, 0);
	});
	m_pickingShader.unbind();
	m_cube.unbind();
	m_pickingBuffer.unbind();
}

void RendererSystem::gbufferPass(const Mat4& projection, const Mat4& view, const Vector<RenderMesh>& renderables) {
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);

	/// Fill GBuffer
	m_gbuffer.bind();
	clear(ClearBufferMask::ColorBuffer | ClearBufferMask::DepthBuffer, 0, 0, 0, 1);

	m_gbufferShader.bind();
	m_gbufferShader.get("mProjection").set(projection);
	m_gbufferShader.get("mView").set(view);

	if (m_pov) {
		m_gbufferShader.get("uEye").set(m_pov->get<Transform>()->worldPosition());
	}

	if (!renderables.empty()) {
		render(m_gbufferShader, renderables);
	}

	m_gbufferShader.unbind();

	m_gbufferInstancedShader.bind();
	m_gbufferInstancedShader.get("mProjection").set(projection);
	m_gbufferInstancedShader.get("mView").set(view);

	if (m_pov) {
		m_gbufferInstancedShader.get("uEye").set(m_pov->get<Transform>()->worldPosition());
	}

	renderInstanced(m_gbufferInstancedShader, renderables);

	m_gbufferInstancedShader.unbind();

	m_gbuffer.unbind();
}

void RendererSystem::lightingPass(EntityWorld& world, const Mat4& projection, const Mat4& view, const Vector<RenderMesh>& renderables) {
	glDisable(GL_DEPTH_TEST);

	// Lights
	m_finalBuffer.bind();

	clear(ClearBufferMask::ColorBuffer, 0, 0, 0, 1);

	m_lightingShader.bind();
	m_lightingShader.get("mProjection").set(projection);
	m_lightingShader.get("mView").set(view);

	m_gbuffer.getColorAttachment(0).bind(m_screenTextureSampler, 0); // Normals
	m_gbuffer.getColorAttachment(1).bind(m_screenTextureSampler, 1); // Albedo
	m_gbuffer.getColorAttachment(2).bind(m_screenTextureSampler, 2); // RME
	m_gbuffer.getDepthAttachment().bind(m_screenDepthSampler, 3); // Depth

	m_lightingShader.get("tNormals").set(0);
	m_lightingShader.get("tAlbedo").set(1);
	m_lightingShader.get("tRME").set(2);
	m_lightingShader.get("tDepth").set(3);

	if (m_pov) {
		Camera *cam = m_pov->get<Camera>();
		m_lightingShader.get("uEye").set(m_pov->get<Transform>()->worldPosition());
		m_lightingShader.get("uNF").set(Vec2(cam->zNear, cam->zFar));
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	m_plane.bind();

	m_lightingShader.get("uEmit").set(false);
	m_lightingShader.get("uIBL").set(false);

	// IBL
	if (m_IBLGenerated) {
		m_brdf.bind(m_screenTextureSampler, 4);
		m_irradiance.bind(m_cubeMapSamplerNoMip, 5);
		m_radiance.bind(m_cubeMapSampler,6);
		m_lightingShader.get("uIBL").set(true);
		m_lightingShader.get("tBRDFLUT").set(4);
		m_lightingShader.get("tIrradiance").set(5);
		m_lightingShader.get("tRadiance").set(6);
		m_plane.drawIndexed(PrimitiveType::Triangles, 0);
		m_lightingShader.get("uIBL").set(false);
	}

	// Emit
	m_lightingShader.get("uEmit").set(true);
	m_plane.drawIndexed(PrimitiveType::Triangles, 0);
	m_lightingShader.get("uEmit").set(false);

	RenderCondition shadowRenderCond = [&](RenderMesh& rm) {
		return getMaterial(rm.materialID).castsShadow;
	};

	// Directional Lights
	world.each([&](Entity& ent, Transform& T, DirectionalLight& L) {
		Mat4 lightVP(1.0f);
		if (L.shadows) {
			m_finalBuffer.unbind();
			m_shadowBuffer.bind();

			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);

			clear(ClearBufferMask::DepthBuffer);

			float s = L.shadowFrustumSize;
			Mat4 projMatLight = glm::ortho(-s, s, -s, s, -s, s);
			Mat4 viewMatLight = glm::inverse(T.getTransformation());

			lightVP = projMatLight * viewMatLight;

			m_shadowShader.bind();
			m_shadowShader.get("mProjection").set(projMatLight);
			m_shadowShader.get("mView").set(viewMatLight);

			render(m_shadowShader, renderables, false, shadowRenderCond);

			m_shadowShader.unbind();

			m_shadowInstancedShader.bind();
			m_shadowInstancedShader.get("mProjection").set(projMatLight);
			m_shadowInstancedShader.get("mView").set(viewMatLight);

			renderInstanced(m_shadowInstancedShader, renderables, false, shadowRenderCond);

			m_shadowInstancedShader.unbind();

			glCullFace(GL_BACK);
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);

			m_shadowBuffer.unbind();
			m_finalBuffer.bind();

			m_lightingShader.bind();
			m_plane.bind();
		}

		m_lightingShader.get("uLight.type").set(L.getType());
		m_lightingShader.get("uLight.color").set(L.color);
		m_lightingShader.get("uLight.intensity").set(L.intensity);

		m_lightingShader.get("uLight.direction").set(T.forward());

		// Shadow
		m_shadowBuffer.getDepthAttachment().bind(m_screenDepthSampler, 7);
		m_lightingShader.get("tShadowMap").set(7);
		m_lightingShader.get("uShadowEnabled").set(L.shadows ? 1 : 0);
		m_lightingShader.get("uLightViewProj").set(lightVP);
		m_lightingShader.get("uLight.size").set(L.size);

		m_plane.drawIndexed(PrimitiveType::Triangles, 0);

		m_shadowBuffer.getDepthAttachment().unbind();
	});

	// Point Lights
	world.each([&](Entity& ent, Transform& T, PointLight& L) {
		m_lightingShader.get("uLight.type").set(L.getType());
		m_lightingShader.get("uLight.color").set(L.color);
		m_lightingShader.get("uLight.intensity").set(L.intensity);

		m_lightingShader.get("uLight.position").set(T.worldPosition());
		m_lightingShader.get("uLight.radius").set(L.radius);
		m_lightingShader.get("uLight.lightCutoff").set(L.lightCutOff);

		m_plane.drawIndexed(PrimitiveType::Triangles, 0);
	});

	// Spot Lights
	world.each([&](Entity& ent, Transform& T, SpotLight& L) {
		Mat4 lightVP(1.0f);
		if (L.shadows) {
			m_finalBuffer.unbind();
			m_shadowBuffer.bind();

			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			//glCullFace(GL_FRONT);

			clear(ClearBufferMask::DepthBuffer);

			float fov = L.spotCutOff * 2.0f;
			Mat4 projMatLight = glm::perspective(fov, 1.0f, 0.01f, L.radius * 4.0f);
			Mat4 viewMatLight = glm::inverse(T.getTransformation());

			lightVP = projMatLight * viewMatLight;

			m_shadowShader.bind();
			m_shadowShader.get("mProjection").set(projMatLight);
			m_shadowShader.get("mView").set(viewMatLight);

			render(m_shadowShader, renderables, false, shadowRenderCond);

			m_shadowShader.unbind();

			m_shadowInstancedShader.bind();
			m_shadowInstancedShader.get("mProjection").set(projMatLight);
			m_shadowInstancedShader.get("mView").set(viewMatLight);

			renderInstanced(m_shadowInstancedShader, renderables, false, shadowRenderCond);

			m_shadowInstancedShader.unbind();

			//glCullFace(GL_BACK);
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);

			m_shadowBuffer.unbind();
			m_finalBuffer.bind();

			m_lightingShader.bind();
			m_plane.bind();
		}

		m_lightingShader.get("uLight.type").set(L.getType());
		m_lightingShader.get("uLight.color").set(L.color);
		m_lightingShader.get("uLight.intensity").set(L.intensity);

		m_lightingShader.get("uLight.position").set(T.worldPosition());
		m_lightingShader.get("uLight.direction").set(T.forward());
		m_lightingShader.get("uLight.radius").set(L.radius);
		m_lightingShader.get("uLight.lightCutoff").set(L.lightCutOff);
		m_lightingShader.get("uLight.spotCutoff").set(L.spotCutOff);

		// Shadow
		m_shadowBuffer.getDepthAttachment().bind(m_screenDepthSampler, 7);
		m_lightingShader.get("tShadowMap").set(7);
		m_lightingShader.get("uShadowEnabled").set(L.shadows ? 1 : 0);
		m_lightingShader.get("uLightViewProj").set(lightVP);
		m_lightingShader.get("uLight.size").set(L.size);

		m_plane.drawIndexed(PrimitiveType::Triangles, 0);
	});

	m_plane.unbind();
	m_lightingShader.unbind();

	glDisable(GL_BLEND);

	if (m_envMap.id() != 0) {
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL);
		glClear(GL_DEPTH_BUFFER_BIT);

		m_gbuffer.bind(FrameBufferTarget::ReadFramebuffer);
		m_finalBuffer.blit(
				0, 0, m_gbuffer.width(), m_gbuffer.height(),
				0, 0, m_gbuffer.width(), m_gbuffer.height(),
				ClearBufferMask::DepthBuffer,
				TextureFilter::Nearest
		);

		m_cube.bind();
		m_cubeMapShader.bind();
		m_cubeMapShader.get("mProjection").set(projection);
		m_cubeMapShader.get("mView").set(view);

		m_envMap.bind(m_cubeMapSampler, 0);
		m_cubeMapShader.get("tCubeMap").set(0);

		m_cube.drawIndexed(PrimitiveType::Triangles, 0);

		m_gbuffer.unbind();
		m_envMap.unbind();
		m_cube.unbind();
		m_cubeMapShader.unbind();

		glDepthFunc(GL_LESS);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	m_finalBuffer.unbind();
}

void RendererSystem::finalPass(const Mat4& projection, const Mat4& view, const Vector<RenderMesh>& renderables, FrameBuffer* target) {
	/// Final render
	m_finalBuffer.getColorAttachment(0).bind(m_screenMipSampler, 0);
	m_finalBuffer.getColorAttachment(0).generateMipmaps();
	m_finalBuffer.getColorAttachment(0).unbind();

	clear(ClearBufferMask::ColorBuffer);

	m_plane.bind();

	/// Post processing
	if (m_postEffects.empty()) {
		if (target) {
			target->bind();
			i32 mask = ClearBufferMask::ColorBuffer;
			if (target->getDepthAttachment().id() != 0)
				mask |= ClearBufferMask::DepthBuffer;
			clear(mask);
		}
		m_finalShader.bind();

		m_finalBuffer.getColorAttachment(0).bind(m_screenMipSampler, 0);
		m_finalShader.get("tTex").set(0);

		m_plane.drawIndexed(PrimitiveType::Triangles, 0);

		m_finalShader.unbind();
	} else {
		int currActive = 0;
		bool first = true;

		m_pingPongBuffer.bind();
		for (Filter& filter : m_postEffects) {
			filter.shader().bind();
			if (filter.passes().empty()) {
				filter.addPass([](Filter& f){});
			}

			for (Pass pass : filter.passes()) {
				int src = currActive;
				int dest = 1 - currActive;

				m_pingPongBuffer.setDrawBuffer(dest);

				if (first) {
					m_finalBuffer.getColorAttachment(0).bind(m_screenMipSampler, 0);
				} else {
					m_pingPongBuffer.getColorAttachment(src).bind(m_screenMipSampler, 0);
					m_pingPongBuffer.getColorAttachment(src).generateMipmaps();
				}

				filter.shader().get("tScreen").set(0);

				i32 slot = 1;
				if (filter.shader().has("tOriginal")) {
					m_finalBuffer.getColorAttachment(0).bind(m_screenMipSampler, slot);
					filter.shader().get("tOriginal").set(slot);
					slot++;
				}

				if (filter.shader().has("tDepth")) {
					m_gbuffer.getDepthAttachment().bind(m_screenDepthSampler, slot);
					filter.shader().get("tDepth").set(slot);
					slot++;
				}

				if (filter.shader().has("tNormals")) {
					m_gbuffer.getColorAttachment(0).bind(m_screenMipSampler, slot);
					filter.shader().get("tNormals").set(slot);
					slot++;
				}

				if (filter.shader().has("uTime"))
					filter.shader().get("tTime").set(m_time);

				if (filter.shader().has("uResolution")) {
					filter.shader().get("uResolution").set(
						Vec2(m_finalBuffer.width(), m_finalBuffer.height())
					);
				}

				if (filter.shader().has("uNF") && m_pov != nullptr) {
					Camera *cam = m_pov->get<Camera>();
					filter.shader().get("uNF").set(
						Vec2(cam->zNear, cam->zFar)
					);
				}

				pass(filter);

				m_plane.drawIndexed(PrimitiveType::Triangles, 0);

				if (first) {
					m_finalBuffer.getColorAttachment(0).unbind();
					first = false;
				} else {
					m_pingPongBuffer.getColorAttachment(src).unbind();
				}

				currActive = 1 - currActive;
			}
			filter.shader().unbind();
		}
		m_pingPongBuffer.unbind();

		if (target) {
			target->bind();
			i32 mask = ClearBufferMask::ColorBuffer;
			if (target->getDepthAttachment().id() != 0)
				mask |= ClearBufferMask::DepthBuffer;
			clear(mask);
		}
		m_finalShader.bind();

		m_pingPongBuffer.getColorAttachment(currActive).bind(m_screenTextureSampler, 0);
		m_finalShader.get("tTex").set(0);

		m_plane.drawIndexed(PrimitiveType::Triangles, 0);

		m_finalShader.unbind();
	}

	m_plane.unbind();

	m_gbuffer.bind(FrameBufferTarget::ReadFramebuffer);
	if (target && target->getDepthAttachment().id() != 0) {
		target->bind(FrameBufferTarget::DrawFramebuffer);
	} else {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
	glBlitFramebuffer(
			0, 0, m_gbuffer.width(), m_gbuffer.height(),
			0, 0, m_gbuffer.width(), m_gbuffer.height(),
			ClearBufferMask::DepthBuffer,
			TextureFilter::Nearest
	);
	if (target) {
		target->getColorAttachment(0).generateMipmaps();
	}
	m_gbuffer.unbind();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glLineWidth(1.0f);
}

RendererSystem& RendererSystem::addPostEffect(Filter effect) {
	m_postEffects.push_back(effect);
	return *this;
}

RendererSystem& RendererSystem::removePostEffect(u32 index) {
	m_postEffects.erase(m_postEffects.begin() + index);
	return *this;
}

RendererSystem& RendererSystem::setEnvironmentMap(const Texture& tex) {
	if (tex.target() != TextureTarget::CubeMap) {
		LogError("Texture is not a Cube Map.");
		return *this;
	}
	m_envMap = tex;
	m_IBLGenerated = false;
	return *this;
}

void RendererSystem::clear(i32 mask, float r, float g, float b, float a) {
	glClearColor(r, g, b, a);
	glClear(mask);
}

void RendererSystem::renderScreenQuad() {
	m_plane.drawIndexed(PrimitiveType::Triangles, 0);
}

void RendererSystem::render(ShaderProgram& shader, const Vector<RenderMesh>& renderables,
							bool textures, const RenderCondition& cond)
{
	for (RenderMesh rm : renderables) {
		Material& mat = getMaterial(rm.materialID);
		if (mat.instanced) continue;
		if (cond && !cond(rm)) continue;

		shader.get("mModel").set(rm.modelMatrix);

		if (shader.has("material.roughness")) {
			shader.get("material.roughness").set(mat.roughness);
			shader.get("material.metallic").set(mat.metallic);
			shader.get("material.emission").set(mat.emission);
			shader.get("material.baseColor").set(mat.baseColor);
			shader.get("material.heightScale").set(mat.heightScale);
			shader.get("material.discardEdges").set(mat.discardParallaxEdges);
		}

		if (textures) {
			int sloti = 0;
			shader.get("tAlbedo0.opt.enabled").set(false);
			shader.get("tAlbedo1.opt.enabled").set(false);
			shader.get("tNormalMap.opt.enabled").set(false);
			shader.get("tRMEMap.opt.enabled").set(false);
			shader.get("tHeightMap.opt.enabled").set(false);

			for (TextureSlot& slot : rm.texturer.textures) {
				if (!slot.enabled || slot.texture.id() == 0) continue;

				String tname = "";
				switch (slot.type) {
					case TextureSlotType::NormalMap:
						tname = "tNormalMap";
						break;
					case TextureSlotType::RougnessMetallicEmission:
						tname = "tRMEMap";
						break;
					case TextureSlotType::Albedo0:
						tname = "tAlbedo0";
						break;
					case TextureSlotType::Albedo1:
						tname = "tAlbedo1";
						break;
					case TextureSlotType::HeightMap:
						tname = "tHeightMap";
						break;
					default:
						break;
				}

				if (!tname.empty()) {
					slot.texture.bind(slot.sampler, sloti);
					shader.get(tname + String(".img")).set(sloti);
					shader.get(tname + String(".opt.enabled")).set(true);
					shader.get(tname + String(".opt.uv_transform")).set(slot.uvTransform);
					sloti++;
				}
			}
		}

		rm.mesh.bind();
		rm.mesh.drawIndexed(PrimitiveType::Triangles, 0, rm.mesh.indexCount());

	}
}

void RendererSystem::renderInstanced(ShaderProgram& shader, const Vector<RenderMesh>& renderables,
									 bool textures, const RenderCondition& cond)
{
	UMap<u32, Material> instancedMaterials;
	Map<u32, InstancedMesh> instancedMeshes;

	for (RenderMesh rm : renderables) {
		Material& mat = getMaterial(rm.materialID);
		if (cond && !cond(rm)) continue;
		if (mat.instanced) {
			instancedMaterials[rm.materialID] = mat;
			instancedMeshes[rm.materialID].mesh = rm.mesh;
			instancedMeshes[rm.materialID].models.push_back(rm.modelMatrix);
			instancedMeshes[rm.materialID].texturer = rm.texturer;
		} else { continue; }
	}

	if (instancedMeshes.empty()) return;

	for (Map<u32, InstancedMesh>::value_type &e : instancedMeshes) {
		Material mat = instancedMaterials[e.first];
		InstancedMesh mi = e.second;
		Vector<Mat4> modelMats;

		for (Mat4 &m : mi.models) {
			modelMats.push_back(m);
		}

		mi.mesh.bind();
		m_instanceBuffer.bind();

		m_instanceBuffer.addVertexAttrib(5, 4, DataType::Float, false, sizeof(Mat4), 0);
		m_instanceBuffer.addVertexAttrib(6, 4, DataType::Float, false, sizeof(Mat4), sizeof(Vec4));
		m_instanceBuffer.addVertexAttrib(7, 4, DataType::Float, false, sizeof(Mat4), sizeof(Vec4) * 2);
		m_instanceBuffer.addVertexAttrib(8, 4, DataType::Float, false, sizeof(Mat4), sizeof(Vec4) * 3);

		m_instanceBuffer.setAttribDivisor(5, 1);
		m_instanceBuffer.setAttribDivisor(6, 1);
		m_instanceBuffer.setAttribDivisor(7, 1);
		m_instanceBuffer.setAttribDivisor(8, 1);

		m_instanceBuffer.setData(modelMats.size(), modelMats.data(), BufferUsage::Stream);

		if (shader.has("material.roughness")) {
			shader.get("material.roughness").set(mat.roughness);
			shader.get("material.metallic").set(mat.metallic);
			shader.get("material.emission").set(mat.emission);
			shader.get("material.baseColor").set(mat.baseColor);
			shader.get("material.heightScale").set(mat.heightScale);
			shader.get("material.discardEdges").set(mat.discardParallaxEdges);
		}

		if (textures) {
			int sloti = 0;
			shader.get("tAlbedo0.opt.enabled").set(false);
			shader.get("tAlbedo1.opt.enabled").set(false);
			shader.get("tNormalMap.opt.enabled").set(false);
			shader.get("tRMEMap.opt.enabled").set(false);
			shader.get("tHeightMap.opt.enabled").set(false);

			for (TextureSlot& slot : mi.texturer.textures) {
				if (!slot.enabled || slot.texture.id() == 0) continue;

				String tname = "";
				switch (slot.type) {
					case TextureSlotType::NormalMap:
						tname = "tNormalMap";
						break;
					case TextureSlotType::RougnessMetallicEmission:
						tname = "tRMEMap";
						break;
					case TextureSlotType::Albedo0:
						tname = "tAlbedo0";
						break;
					case TextureSlotType::Albedo1:
						tname = "tAlbedo1";
						break;
					case TextureSlotType::HeightMap:
						tname = "tHeightMap";
						break;
					default:
						break;
				}

				if (!tname.empty()) {
					slot.texture.bind(slot.sampler, sloti);
					shader.get(tname + String(".img")).set(sloti);
					shader.get(tname + String(".opt.enabled")).set(true);
					shader.get(tname + String(".opt.uv_transform")).set(slot.uvTransform);
					sloti++;
				}
			}
		}

		mi.mesh.drawIndexedInstanced(
				PrimitiveType::Triangles,
				modelMats.size()
		);

		mi.mesh.unbind();
	}
}

u32 RendererSystem::renderWidth() const {
	return m_renderWidth;
}

Entity* RendererSystem::POV() const {
	return m_pov;
}

void RendererSystem::setPOV(Entity* pov) {
	m_pov = pov;
}

Material& RendererSystem::createMaterial(const String& name) {
	assert(m_materialID < MAX_MATERIALS && "Too many materials!");

	String _name = name;
	if (_name.empty()) {
		_name = Util::strCat("material_", m_materialID);
	}

	m_materials[m_materialID++].name = _name;

	u32 id = m_materialID - 1;
	Material& mat = m_materials[id].mat;
	mat.m_id = id;

	return mat;
}

Material& RendererSystem::getMaterial(u32 id) {
	return m_materials[id].mat;
}

String RendererSystem::getMaterialName(u32 id) const {
	return m_materials[id].name;
}

u32 RendererSystem::renderHeight() const {
	return m_renderHeight;
}


NS_END
