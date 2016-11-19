#ifndef _GL_UTIL_HPP_
#define _GL_UTIL_HPP_

#include <iostream>

#include <GL\glew.h>

#include <cassert>

#include "intrinsics.hpp"

// TODO(orglofch): Use vbo
void glDrawRect(float left, float right, float bottom, float top, float depth) {
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(left, bottom, depth);
		glTexCoord2f(1, 0); glVertex3f(right, bottom, depth);
		glTexCoord2f(1, 1); glVertex3f(right, top, depth);
		glTexCoord2f(0, 1); glVertex3f(left, top, depth);
	glEnd();
}

void glCreateTexture2D(GLuint *texture, int width, int height, int channels, void *data) {
	assert(data);

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, texture);
	if (*texture < 1) {
		LOG("Failed to load texture\n");
		return;
	}

	GLint format = (channels == 4 ? GL_RGBA : GL_RGB);

	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
		GL_FLOAT, data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void glSetPerspectiveProjection(const size_t width, 
							    const size_t height, 
								const GLdouble fov,
							    const GLdouble n, 
							    const GLdouble f) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float aspect = 1.0f * width / height;
	gluPerspective(fov, 1.0 * width / height, n, f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void glSetOrthographicProjection(const GLdouble left, 
							     const GLdouble right, 
							     const GLdouble bottom, 
							     const GLdouble top, 
							     const GLdouble n, 
							     const GLdouble f) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(left, right, bottom, top, n, f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void glPrintShaderInfo(GLint shader) {
	int info_log_len = 0;
	int char_written = 0;
	GLchar *info_log;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_len);

	if (info_log_len > 0) {
		info_log = new GLchar[info_log_len];
		glGetShaderInfoLog(shader, info_log_len, &char_written, info_log);
		printf("Info: %s\n", info_log);
		delete[] info_log;
	}
}

bool glLoadShader(const char *vs_filename,
	              const char *fs_filename,
	              GLuint &shader_program,
				  GLuint &vertex_shader,
				  GLuint &fragment_shader) {
	shader_program = 0;
	vertex_shader = 0;
	fragment_shader = 0;

	std::string vertex_shader_string = ReadFile(vs_filename);
	std::string fragment_shader_string = ReadFile(fs_filename);
	int vlen = vertex_shader_string.length();
	int flen = fragment_shader_string.length();

	if (vertex_shader_string.empty())
		return false;
	if (fragment_shader_string.empty())
		return false;

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	const char *vertex_shader_cstr = vertex_shader_string.c_str();
	const char *fragment_shader_cstr = fragment_shader_string.c_str();
	glShaderSource(vertex_shader, 1, (const GLchar **)&vertex_shader_cstr, &vlen);
	glShaderSource(fragment_shader, 1, (const GLchar **)&fragment_shader_cstr, &flen);

	GLint compiled;

	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == false) {
		printf("Vertex shader not compliled\n");
		glPrintShaderInfo(vertex_shader);

		glDeleteShader(vertex_shader);
		vertex_shader = 0;
		glDeleteShader(fragment_shader);
		fragment_shader = 0;

		return false;
	}

	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == false) {
		printf("Fragment shader not compiled\n");
		glPrintShaderInfo(fragment_shader);

		glDeleteShader(vertex_shader);
		vertex_shader = 0;
		glDeleteShader(fragment_shader);
		fragment_shader = 0;

		return false;
	}

	shader_program = glCreateProgram();

	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);

	glLinkProgram(shader_program);

	GLint is_linked;
	glGetProgramiv(shader_program, GL_LINK_STATUS, (GLint *)&is_linked);
	if (is_linked == false) {
		printf("Failed to link shader\n");

		GLint max_length;
		glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &max_length);
		if (max_length > 0) {
			char *p_link_info_log = new char[max_length];
			glGetProgramInfoLog(shader_program, max_length, &max_length, p_link_info_log);
			printf("%s\n", p_link_info_log);
			delete[] p_link_info_log;
		}

		glDetachShader(shader_program, vertex_shader);
		glDetachShader(shader_program, fragment_shader);
		glDeleteShader(vertex_shader);
		vertex_shader = 0;
		glDeleteShader(fragment_shader);
		fragment_shader = 0;
		glDeleteProgram(shader_program);
		shader_program = 0;

		return false;
	}
	return true;
}

#endif