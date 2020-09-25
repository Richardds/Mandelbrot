#include <cstdio>
#include <cstdlib>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

const char* vertex_shader_source = ""
    "#version 430 core\n"

	"layout(location = 0) in vec3 position;\n"

	"void main() {\n"
	"    gl_Position = vec4(position, 1.0);\n"
    "}\n";

const char* fragment_shader_source = ""
    "#version 430 core\n"

	"layout(location = 0) uniform int width;\n"
	"layout(location = 1) uniform int height;\n"
	"layout(location = 2) uniform dvec2 area_w;\n"
	"layout(location = 3) uniform dvec2 area_h;\n"
	"layout(location = 4) uniform uint max_iterations;\n"

    "out vec4 color;\n"

	"vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) {\n"
	"    return a + b * cos(6.28318 * (c * t + d));\n"
	"}\n"

    "void main() {\n"
	"    const dvec2 c = dvec2(gl_FragCoord.x * (area_w.y - area_w.x) / width + area_w.x, gl_FragCoord.y * (area_h.y - area_h.x) / height + area_h.x);\n"
	"    dvec2 z = dvec2(0.0, 0.0);\n"
	"    uint iteration;\n"

	"    for (iteration = 0; iteration < max_iterations && z.x * z.x + z.y * z.y <= 4.0; ++iteration) {\n"
	"        z = dvec2(z.x * z.x - z.y * z.y + c.x, 2.0 * z.x * z.y + c.y);\n"
	"    }\n"

	"    color = vec4(iteration == max_iterations ? vec3(0.85, 0.99, 1.0) : palette(float(iteration) / float(max_iterations), vec3(0.0), vec3(0.59,0.55,0.75), vec3(0.1, 0.2, 0.3), vec3(0.75)), 1.0);\n"
    "}\n";

const

const size_t title_buffer_size = 128;
const unsigned int stats_refresh_rate = 3;

int width = 1280;
int height = 720;

const GLfloat screen_vertices[] = {
	-1.0f,  3.0f,
	-1.0f, -1.0f,
	 3.0f, -1.0f
};

GLFWwindow* window = NULL;

struct MandelbrotLocation {
	double x, y, scale;
};

const MandelbrotLocation pois[] = {
	{-0.7104275066275, -0.269772769335717, 3.526e-03}
};

class MandelbrotSet {
private:
	MandelbrotLocation location;
	double iterations_limit;

public:
	MandelbrotSet() : location({ 0.25, 0, 1.25 }), iterations_limit(128) {}

	double getX() const {
		return this->location.x;
	}

	double getY() const {
		return this->location.y;
	}

	double getScale() const {
		return this->location.scale;
	}

	unsigned int getIterationsLimit() const {
		return static_cast<unsigned int>(floor(this->iterations_limit));
	}

	void setX(double x) {
		this->location.x = x;
	}

	void setY(double y) {
		this->location.y = y;
	}

	void setLocation(const MandelbrotLocation& location) {
		this->location = location;
	}

	void setScale(double scale) {
		this->location.scale = scale;
	}

	void setIterationsLimit(double iterations_limit) {
		this->iterations_limit = iterations_limit;
	}

	void reset() {
		this->location = { 0.25, 0, 1.25 };
	}

	void increaseX(double x, double delta) {
		this->location.x += x * delta * this->location.scale;
	}

	void increaseY(double y, double delta) {
		this->location.y += y * delta * this->location.scale;
	}

	void increaseScale(double scale, double delta) {
		this->location.scale += scale * this->location.scale * delta;
	}

	void increaseIterationsLimit(double iterations, double delta) {
		double next_iterations_limit = this->iterations_limit + iterations * delta;

		if (next_iterations_limit >= 0) {
			this->iterations_limit = next_iterations_limit;
		}
	}

} set;

struct Sync {
	double last_interval, last, current, delta;
} sync;

void glfw_error_callback(int error, const char* description)
{
	printf("GLFW error code %d with message: %s\n", error, description);
}

void glfw_window_size_callback(GLFWwindow* window, int width_update, int height_update) {
	// Update rendering resolution
	width = width_update;
	height = height_update;

	// Update viewport
	glViewport(0, 0, width, height);

	// Upload rendering resolution to GPU
	glUniform1i(0, width);
	glUniform1i(1, height);
}

GLuint process_shader(GLenum type, const char* source, GLuint length)
{
	GLuint shader = glCreateShader(type);
	if (shader == 0) {
		puts("Failed to create shader!");

		return 0;
	}

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint shader_compile_status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compile_status);
	if (shader_compile_status != GL_TRUE) {
		puts("Failed to compile shader!");

		GLsizei error_message_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_message_length);
		char* error_message = (char*)malloc((size_t)error_message_length);

		if (error_message == NULL) {
			puts("Failed to allocate buffer for error message!");

			return 0;
		}

		glGetShaderInfoLog(shader, error_message_length, NULL, error_message);
		printf("Error message: %s\n", error_message);
		free(error_message);

		return 0;
	}

	return shader;
}

void handle_input()
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		set.reset();
	}

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		set.setLocation(pois[0]);
	}

	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		set.setLocation(pois[0]);
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		set.increaseIterationsLimit(-75, sync.delta);
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		set.increaseIterationsLimit(75, sync.delta);
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		set.increaseScale(-1.0, sync.delta);
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		set.increaseScale(1.0, sync.delta);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		set.increaseY(1.0, sync.delta);
	}

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		set.increaseY(-1.0, sync.delta);
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		set.increaseX(-1.0, sync.delta);
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		set.increaseX(1.0, sync.delta);
	}
}

int main()
{
	glfwSetErrorCallback(glfw_error_callback);

	// Initialize GLFW

	if (glfwInit() != GLFW_TRUE) {
		puts("GLFW initialization failed!");

		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

	// Create rendering window

	window = glfwCreateWindow(width, height, "Mandelbrot", NULL, NULL);

	if (window == NULL) {
		puts("Failed to create window!");
		glfwTerminate();

		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	glfwSetWindowSizeCallback(window, glfw_window_size_callback);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
	glViewport(0, 0, width, height);

	// Initialize GLEW

	GLenum res = glewInit();
	if (res != GLEW_OK) {
		printf("GLEW initialization failed! Error message: %s\n", glewGetErrorString(res));
		glfwDestroyWindow(window);
		glfwTerminate();

		return EXIT_FAILURE;
	}

	// Load shaders

	GLuint shader_program = glCreateProgram();
	if (shader_program == 0) {
		puts("Failed to create shader program!");
		glfwDestroyWindow(window);
		glfwTerminate();

		return EXIT_FAILURE;
	}

	GLuint vertex_shader = process_shader(GL_VERTEX_SHADER, vertex_shader_source, sizeof(vertex_shader_source));
	GLuint fragment_shader = process_shader(GL_FRAGMENT_SHADER, fragment_shader_source, sizeof(fragment_shader_source));
	if (vertex_shader == 0 || fragment_shader == 0) {
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
		glDeleteProgram(shader_program);

		glfwDestroyWindow(window);
		glfwTerminate();

		return EXIT_FAILURE;
	}

	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	GLint shader_link_status = GL_FALSE;
	glGetProgramiv(shader_program, GL_LINK_STATUS, &shader_link_status);
	if (shader_link_status != GL_TRUE) {
		puts("Failed to link shader program!");

		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
		glDeleteProgram(shader_program);

		glfwDestroyWindow(window);
		glfwTerminate();

		return EXIT_FAILURE;
	}

	// Load screen vertices
	GLuint vao, vbo;
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_STATIC_DRAW);	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Rendering loop

	sync.last_interval = glfwGetTime();
	sync.last = sync.last_interval;
	sync.current = sync.last_interval;
	sync.delta = 0;
	char title[title_buffer_size];

	glClearColor(0.25f, 0.25f, 0.25f, 1.0f);

	glBindVertexArray(vao);
	glUseProgram(shader_program);

	glUniform1i(0, width);
	glUniform1i(1, height);

	while (!glfwWindowShouldClose(window)) {
		handle_input();

		glClear(GL_COLOR_BUFFER_BIT);

		glUniform2d(2, -2.0 * set.getScale() + set.getX(), 1.0 * set.getScale() + set.getX());
		glUniform2d(3, -1.0 * set.getScale() + set.getY(), 1.0 * set.getScale() + set.getY());
		glUniform1ui(4, set.getIterationsLimit());

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
		glfwPollEvents();

		sync.current = glfwGetTime();
		sync.delta = sync.current - sync.last;
		sync.last = sync.current;

		if (sync.current - sync.last_interval > 1 / static_cast<double>(stats_refresh_rate)) {
			sync.last_interval = sync.current;

			// Update title
			sprintf_s(title, title_buffer_size, u8"Mandelbrot | FPS: %u @ %.03lf ms | X: %.016f | Y: %.016f | Iterations limit: %u | Scale: %.03e\0",
				static_cast<unsigned int>(floor(1 / sync.delta)), sync.delta * 1000, set.getX(), set.getY(), set.getIterationsLimit(), set.getScale());
			glfwSetWindowTitle(window, title);
		}
	}

	glUseProgram(0);
	glBindVertexArray(0);

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDetachShader(shader_program, vertex_shader);
	glDetachShader(shader_program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glDeleteProgram(shader_program);

	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;
}
