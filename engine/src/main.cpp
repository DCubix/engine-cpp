#include <iostream>

#include "core/input.h"
#include "core/app.h"
#include "gfx/api.h"
#include "gfx/shader.h"
#include "gfx/mesher.h"

static const String VS = R"(
#version 330 core
layout (location = 0) in vec3 vPosition;
out vec3 oPosition;
void main() {
	gl_Position = vec4(vPosition * 2.0 - 1.0, 1.0);
	oPosition = vPosition;
}
)";
static const String FS = R"(
#version 330 core
out vec4 fragColor;
in vec3 oPosition;
void main() {
	fragColor = vec4(oPosition, 1.0);
}
)";

template <>
struct VertexAppender<Vec3> {
	static void append(Vector<float>& data, const Vec3& v) {
		data.push_back(v.x);
		data.push_back(v.y);
		data.push_back(v.z);
	}
};

class TestApp : public IApplicationAdapter {
public:
	void init() {
		glClearColor(0.04f, 0.45f, 0.53f, 1.0f);

		prog = uptr<ShaderProgram>(new ShaderProgram());
		prog->add(VS, api::ShaderType::VertexShader);
		prog->add(FS, api::ShaderType::FragmentShader);
		prog->link();

		model = uptr<Model>(new Model());
		model->format()->put("vPosition", AttributeType::AttrVector3, false, 0);

		model->addVertex(Vec3(0, 0, 0));
		model->addVertex(Vec3(1, 0, 0));
		model->addVertex(Vec3(1, 1, 0));
		model->addVertex(Vec3(0, 1, 0));
		model->addTriangle(0, 1, 2);
		model->addTriangle(2, 3, 0);
		model->flush();
		
		glDisable(GL_CULL_FACE);
	}

	void update(float timeDelta) {
		if (Input::isKeyPressed(SDLK_ESCAPE)) {
			MessageSystem::ston().submitAndSend("app_quit");
		}
	}

	void render() {
		glClear(GL_COLOR_BUFFER_BIT);

		prog->bind();
		model->bind();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	}

	uptr<ShaderProgram> prog;
	uptr<Model> model;
};

int main(int argc, char** argv) {
	Application app(new TestApp());
	app.run();
	return 0;
}