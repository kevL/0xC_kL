// This file was copied from the bsnes project.

// This is the license info, from ruby.hpp:

/*
	ruby
	version: 0.08 (2011-11-25)
	license: public domain
*/

#ifndef OPENXCOM_OPENGL_H
#define OPENXCOM_OPENGL_H

#ifndef __NO_OPENGL

#include <string>

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "Logger.h"


namespace OpenXcom
{

class Surface;


#ifndef __APPLE__
	extern PFNGLCREATEPROGRAMPROC		glCreateProgram;
	extern PFNGLDELETEPROGRAMPROC		glDeleteProgram;
	extern PFNGLUSEPROGRAMPROC			glUseProgram;
	extern PFNGLISPROGRAMPROC			glIsProgram;
	extern PFNGLISSHADERPROC			glIsShader;
	extern PFNGLCREATESHADERPROC		glCreateShader;
	extern PFNGLDELETESHADERPROC		glDeleteShader;
	extern PFNGLSHADERSOURCEPROC		glShaderSource;
	extern PFNGLCOMPILESHADERPROC		glCompileShader;
	extern PFNGLATTACHSHADERPROC		glAttachShader;
	extern PFNGLDETACHSHADERPROC		glDetachShader;
	extern PFNGLGETATTACHEDSHADERSPROC	glGetAttachedShaders;
	extern PFNGLGETPROGRAMINFOLOGPROC	glGetProgramInfoLog;
	extern PFNGLGETPROGRAMIVPROC		glGetProgramiv;
	extern PFNGLGETSHADERINFOLOGPROC	glGetShaderInfoLog;
	extern PFNGLGETSHADERIVPROC			glGetShaderiv;
	extern PFNGLLINKPROGRAMPROC			glLinkProgram;
	extern PFNGLGETUNIFORMLOCATIONPROC	glGetUniformLocation;
	extern PFNGLUNIFORM1IPROC			glUniform1i;
	extern PFNGLUNIFORM2FVPROC			glUniform2fv;
	extern PFNGLUNIFORM4FVPROC			glUniform4fv;
#endif

/// Returns an OpenGL error-string.
std::string strGLError(GLenum glErr);

// GL-error checker.
#define glErrorCheck() { \
	static bool reported = false; \
	GLenum glErr; \
	if (OpenGL::checkErrors && !reported && (glErr = glGetError()) != GL_NO_ERROR) \
	{ \
		reported = true; \
		do \
		{ \
			Log(LOG_WARNING) << __FILE__ << ":" << __LINE__ << ": glGetError() complaint: " << strGLError(glErr); \
		} while (((glErr = glGetError()) != GL_NO_ERROR)); \
	} \
}


class OpenGL
{

private:
	/// Exits - because destructors are uncool for people.
	void terminate();

	/// Makes all the pixels go away.
	void clear();

	/// Resizes the internal buffer.
	void resize(
			int width,
			int height);

	/// Sets a fragment-shader.
	void set_fragment_shader(const char* source);
	/// Sets a vertex-shader.
	void set_vertex_shader(const char* source);


	public:
		static bool checkErrors;

		bool
			linear,
			shader_support;
		int
			iwidth,
			iheight;
		unsigned
			iformat,
			ibpp;

		uint32_t* buffer;

		GLuint
			gltexture,
			glprogram,
			fragmentshader,
			vertexshader;

		Surface* buffer_surface;

		/// Constructor ... like we said you're too cool to actually construct things.
		OpenGL();
		/// dTor.
		~OpenGL();

		/// Because you're too cool to initialize everything in the constructor.
		void init(
				int width,
				int height);

		/// Makes the buffer show up on-screen.
		void refresh(
				bool smooth,
				int inwidth,
				int inheight,
				int outwidth,
				int outheight,
				int topBlackBand,
				int bottomBlackBand,
				int leftBlackBand,
				int rightBlackBand);

		/// Sets a shader!
		void set_shader(const char* source_yaml_filename);

		/// Tries to set VSync! decidedly uncool if it doesn't work.
		void setVSync(bool sync);

		/// Sets a pointer to a data-buffer where an image is stored.
//		bool lock(
//				uint32_t*& data,
//				unsigned& pitch);
};

}

#else // !__NO_OPENGL
namespace OpenXcom
{ class OpenGL{}; }
#endif // !__NO_OPENGL

#endif
