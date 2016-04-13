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

#include <SDL.h>
#include <SDL_opengl.h>

#include "Logger.h"


namespace OpenXcom
{

class Surface;


#ifndef __APPLE__
	extern PFNGLCREATEPROGRAMPROC		glCreateProgram;
	extern PFNGLUSEPROGRAMPROC			glUseProgram;
	extern PFNGLCREATESHADERPROC		glCreateShader;
	extern PFNGLDELETESHADERPROC		glDeleteShader;
	extern PFNGLSHADERSOURCEPROC		glShaderSource;
	extern PFNGLCOMPILESHADERPROC		glCompileShader;
	extern PFNGLATTACHSHADERPROC		glAttachShader;
	extern PFNGLDETACHSHADERPROC		glDetachShader;
	extern PFNGLLINKPROGRAMPROC			glLinkProgram;
	extern PFNGLGETUNIFORMLOCATIONPROC	glGetUniformLocation;
	extern PFNGLUNIFORM1IPROC			glUniform1i;
	extern PFNGLUNIFORM2FVPROC			glUniform2fv;
	extern PFNGLUNIFORM4FVPROC			glUniform4fv;
#endif

std::string strGLError(GLenum glErr);

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
	/// call to resize internal buffer
	void resize(
			unsigned width,
			unsigned height);


	public:
		static bool checkErrors;

		bool
			linear,
			shader_support;
		unsigned
			iwidth,
			iheight,
			iformat,
			ibpp;

		uint32_t* buffer;

		GLuint
			gltexture,
			glprogram,
			fragmentshader,
			vertexshader;

		Surface* buffer_surface;

		/// actually sets a pointer to a data-buffer where the image is written to
		bool lock(
				uint32_t* &data,
				unsigned &pitch);

		/// make all the pixels go away
		void clear();
		/// make the buffer show up on screen
		void refresh(
				bool smooth,
				unsigned inwidth,
				unsigned inheight,
				unsigned outwidth,
				unsigned outheight,
				int topBlackBand,
				int bottomBlackBand,
				int leftBlackBand,
				int rightBlackBand);

		/// set a shader!
		void set_shader(const char* source);
		/// same but for fragment shader
		void set_fragment_shader(const char* source);
		/// and vertex
		void set_vertex_shader(const char* source);

		/// because you're too cool to initialize everything in the constructor
		void init(
				int width,
				int height);
		/// exit because destructors are uncool for people
		void terminate();

		/// Try to set VSync! decidedly uncool if it doesn't work.
		void setVSync(bool sync);

		/// constructor ... like we said you're too cool to actually construct things
		OpenGL();
		/// dTor
		~OpenGL();
};

}

#else // __NO_OPENGL
namespace OpenXcom
{
class OpenGL{};
}
#endif

#endif
