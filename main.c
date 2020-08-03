#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>

#ifndef __cplusplus
#define nullptr ((void*) 0)
#endif

#define QUERY_COUNT 2
#define INIT_WINDOW_W 512
#define INIT_WINDOW_H 512

typedef void (*glViewportFUNC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (*glClearFUNC) (GLbitfield mask);
typedef void (*glClearColorFUNC) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef const GLubyte* (*glGetStringFUNC) (GLenum name);

struct Context {
	SDL_Window *win;
	SDL_GLContext glCtx;
	GLuint vbo, dibo[2];
	GLuint vao;
	GLuint program;
	GLuint query[QUERY_COUNT];
	int quitFlag;

	glViewportFUNC glViewport;
	glClearFUNC glClear;
	glClearColorFUNC glClearColor;
	glGetStringFUNC glGetString;
};

static const char vertSource[] =
        "#version 110\n"
        "attribute vec4 a_pos;"
        "attribute vec4 a_color;"
        "varying vec4 v_color;"
        "void main() {"
        "  v_color = a_color;"
        "  gl_Position = vec4(a_pos.xy, 0.0, 1.0);"
        "}"
;

static const char fragSource[] =
        "#version 110\n"
        "varying vec4 v_color;"
        "void main() {"
        "  gl_FragColor = v_color;"
        "}"
;

typedef struct {
	float x, y, z, w;
} Vec4;

typedef struct {
	Vec4 pos, color;
} Point;

typedef struct {
	uint32_t count;
	uint32_t instanceCount;
	uint32_t first;
	uint32_t baseInstance;
} DrawArraysIndirectCommand;

void setupPointVertAttribs(GLint posLocation, GLint colorLocation) {
	glEnableVertexAttribArray(posLocation);
	glVertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
	glVertexAttribDivisor(posLocation, 1);

	glEnableVertexAttribArray(colorLocation);
	glVertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (const void*) sizeof(Vec4));
	glVertexAttribDivisor(colorLocation, 1);
}

float randNorm() {
	const float max = RAND_MAX;
	float r = rand();

	return r / max;
}

float randNdcCoord() {
	return randNorm() * 2 - 1;
}

void writeRandomPoint(Point *pointData) {
	pointData->pos = (Vec4) {
	    randNdcCoord(),
	    randNdcCoord(),
	    0.0f, 0.0f
    };

	pointData->color = (Vec4) {
	    randNorm(),
	    randNorm(),
	    randNorm(),
	    1.0f
    };
}

#define FUNNEH_POINT_COUNT (1024 * 128)

void uploadFunnieBuffa() {
	Point *buf = malloc(sizeof(Point) * FUNNEH_POINT_COUNT);
	for (size_t i = 0; i < FUNNEH_POINT_COUNT; ++i) {
		writeRandomPoint(buf + i); // cursed af X)
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * FUNNEH_POINT_COUNT, buf, GL_STATIC_DRAW);
	free(buf);
}

// 1 in holeRatio draws will not be a hole
void uploadIndirectBuffer(GLenum target, size_t count, uint32_t maxInstance, int holeRatio) {
	size_t bufferSize = sizeof(DrawArraysIndirectCommand) * count;
	DrawArraysIndirectCommand *cmdBuf = malloc(bufferSize);
	for (size_t i = 0; i < count; ++i) {
		cmdBuf[i].count = 1;
		cmdBuf[i].first = 0;
		cmdBuf[i].baseInstance = ((uint32_t) rand()) % maxInstance;
		cmdBuf[i].instanceCount = 1;

		if (holeRatio == 0) {
			continue;
		} else if (rand() % holeRatio < (holeRatio-1)) {
			cmdBuf[i].count = 0;
			cmdBuf[i].instanceCount = 0;
		}
	}

	glBufferData(target, bufferSize, cmdBuf, GL_STATIC_DRAW);
	free(cmdBuf);
}

GLuint compileShader(unsigned type, const char *src, GLint len) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, &len);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	assert(success != GL_FALSE);

	return shader;
}

GLuint linkProgram(GLuint vert, GLuint frag) {
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);

	GLint success = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	assert(success != GL_FALSE);

	return prog;
}

void initContext(struct Context *ctx) {
	ctx->glClearColor(0, 0, 0, 1);

	// Init shader program
	GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSource, sizeof(vertSource));
	GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource, sizeof(fragSource));
	ctx->program = linkProgram(vertShader, fragShader);
	glDetachShader(ctx->program, vertShader);
	glDeleteShader(vertShader);
	glDetachShader(ctx->program, fragShader);
	glDeleteShader(fragShader);

	GLint posLocation = glGetAttribLocation(ctx->program, "a_pos");
	GLint colorLocation = glGetAttribLocation(ctx->program, "a_color");

	// Init buffers / vao
	glGenBuffers(2, ctx->dibo);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ctx->dibo[0]);
	uploadIndirectBuffer(GL_DRAW_INDIRECT_BUFFER, FUNNEH_POINT_COUNT, FUNNEH_POINT_COUNT, 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ctx->dibo[1]);
	uploadIndirectBuffer(GL_DRAW_INDIRECT_BUFFER, FUNNEH_POINT_COUNT, FUNNEH_POINT_COUNT, 1000);

	glGenBuffers(1, &ctx->vbo);

	glGenVertexArrays(1, &ctx->vao);
	glBindVertexArray(ctx->vao);

	glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
	uploadFunnieBuffa();
	setupPointVertAttribs(posLocation, colorLocation);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Init timer query
	glGenQueries(QUERY_COUNT, ctx->query);
}

void teardownContext(struct Context *ctx) {
	glDeleteQueries(QUERY_COUNT, ctx->query);
	glDeleteProgram(ctx->program);
	glDeleteVertexArrays(1, &ctx->vao);
	glDeleteBuffers(1, &ctx->vbo);
	glDeleteBuffers(2, ctx->dibo);
}

void handleEvent(struct Context *ctx, SDL_Event e) {
	switch (e.type) {
	case SDL_WINDOWEVENT:
		switch (e.window.event) {
		case SDL_WINDOWEVENT_RESIZED: {
			ctx->glViewport(0, 0, e.window.data1, e.window.data2);
			break;
		}
		case SDL_WINDOWEVENT_CLOSE:
			ctx->quitFlag = true;
			break;
		}
	}
}

void pollEvents(struct Context *ctx) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		handleEvent(ctx, event);
	}
}

void drawMultiInstance(struct Context *ctx, unsigned diboIdx, const char *msg) {
	ctx->glClear(GL_COLOR_BUFFER_BIT);

	pollEvents(ctx);
	if (ctx->quitFlag) {
		return;
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ctx->dibo[diboIdx]);

	glQueryCounter(ctx->query[0], GL_TIMESTAMP);
	glMultiDrawArraysIndirect(GL_POINTS, nullptr, FUNNEH_POINT_COUNT, 0);
	glQueryCounter(ctx->query[1], GL_TIMESTAMP);

	pollEvents(ctx);
	if (ctx->quitFlag) {
		return;
	}

	SDL_GL_SwapWindow(ctx->win);

	pollEvents(ctx);
	if (ctx->quitFlag) {
		return;
	}

	GLint resultsAvail = 0;
	while (resultsAvail == 0) {
		glGetQueryObjectiv(ctx->query[1], GL_QUERY_RESULT_AVAILABLE, &resultsAvail);

		pollEvents(ctx);
		if (ctx->quitFlag) {
			return;
		}
	}

	GLuint64 begin, end;
	glGetQueryObjectui64v(ctx->query[0], GL_QUERY_RESULT, &begin);
	glGetQueryObjectui64v(ctx->query[1], GL_QUERY_RESULT, &end);

	printf("%s took: %f ms\n", msg, (end-begin) / 1000000.0);
	fflush(stdout);
}

void runBenchmarks(struct Context *ctx) {
	glBindVertexArray(ctx->vao);
	glUseProgram(ctx->program);

	while (1) {
//		glDrawArrays(GL_POINTS, 0, FUNNEH_POINT_COUNT);

		drawMultiInstance(ctx, 0, "   no holes");
		drawMultiInstance(ctx, 1, "99.9% holes");

		pollEvents(ctx);
		if (ctx->quitFlag) {
			break;
		}
	}
}

void initCursedGlewCompat() {
	if (glMultiDrawArraysIndirect == nullptr) {
		if (glMultiDrawArraysIndirectEXT != nullptr) {
			glMultiDrawArraysIndirect = glMultiDrawArraysIndirectEXT;
		} else {
			assert(!"glMultiDrawArraysIndirect not available");
		}
	}

	if (glVertexAttribDivisor == nullptr) {
		if (glVertexAttribDivisorARB != nullptr) {
			glVertexAttribDivisor = glVertexAttribDivisorARB;
		} else {
			assert(!"glVertexAttribDivisor not available");
		}
	}
}

int main(int argc, char *argv[]) {
	(void) argc; (void) argv;

	struct Context ctx;
	ctx.quitFlag = false;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	ctx.win = SDL_CreateWindow("IndirectDrawCompactBenchmark",
	                           SDL_WINDOWPOS_UNDEFINED,
	                           SDL_WINDOWPOS_UNDEFINED,
	                           INIT_WINDOW_W, INIT_WINDOW_H,
	                           SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	ctx.glCtx = SDL_GL_CreateContext(ctx.win);

	ctx.glViewport = (glViewportFUNC) SDL_GL_GetProcAddress("glViewport");
	ctx.glClear = (glClearFUNC) SDL_GL_GetProcAddress("glClear");
	ctx.glClearColor = (glClearColorFUNC) SDL_GL_GetProcAddress("glClearColor");
	ctx.glGetString = (glGetStringFUNC) SDL_GL_GetProcAddress("glGetString");

	SDL_GL_SetSwapInterval(1);
	glewInit();

	// shhh, you never saw this :tiny-potato:
	initCursedGlewCompat();

	const unsigned char *renderer = ctx.glGetString(GL_RENDERER);
	printf("Renderer: %s\n", renderer);
	ctx.glViewport(0, 0, INIT_WINDOW_W, INIT_WINDOW_H);

	initContext(&ctx);
	runBenchmarks(&ctx);
	teardownContext(&ctx);

	SDL_GL_DeleteContext(ctx.glCtx);
	SDL_DestroyWindow(ctx.win);
	SDL_Quit();
	return 0;
}
