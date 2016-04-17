// This file was copied from the bsnes project.

// This is the license info, from ruby.hpp:

/*
	ruby
	version: 0.08 (2011-11-25)
	license: public domain
*/

#ifndef __NO_OPENGL

#include "OpenGL.h"

#include <cstring>
//#include <fstream>

#include <yaml-cpp/yaml.h>

#include "Surface.h"


namespace OpenXcom
{

bool OpenGL::checkErrors = true;

/**
 * Returns an OpenGL error-string.
 * @param glErr - GL-error enumerated key
 * @return, error-string
 */
std::string strGLError(GLenum glErr)
{
	std::string err;
	switch (glErr)
	{
		case GL_INVALID_ENUM:		err = "GL_INVALID_ENUM";		break;
		case GL_INVALID_VALUE:		err = "GL_INVALID_VALUE";		break;
		case GL_INVALID_OPERATION:	err = "GL_INVALID_OPERATION";	break;
		case GL_STACK_OVERFLOW:		err = "GL_STACK_OVERFLOW";		break;
		case GL_STACK_UNDERFLOW:	err = "GL_STACK_UNDERFLOW";		break;
		case GL_OUT_OF_MEMORY:		err = "GL_OUT_OF_MEMORY";		break;

		case GL_NO_ERROR:			err = "No error! How did you even reach this code.";
			break;
		default:					err = "Unknown error code!";
	}
	return err;
}

/**
 * Helper typedefs to convert between object-pointers and function-pointers.
 * @note Although ignored by some compilers this conversion is an extension and
 * not guaranteed to be sane for all architectures.
 */
typedef void (*GenericFunctionPointer)();

/**
 *
 */
typedef union
{
	GenericFunctionPointer FunctionPointer;
	void* ObjectPointer;
} UnsafePointerContainer;

/**
 *
 */
inline static GenericFunctionPointer glGetProcAddress(const char* label)
{
	UnsafePointerContainer pc;
	pc.ObjectPointer = SDL_GL_GetProcAddress(label);
	return pc.FunctionPointer;
}

#ifndef __APPLE__
PFNGLCREATEPROGRAMPROC		glCreateProgram			= nullptr;
PFNGLUSEPROGRAMPROC			glUseProgram			= nullptr;
PFNGLCREATESHADERPROC		glCreateShader			= nullptr;
PFNGLDELETESHADERPROC		glDeleteShader			= nullptr;
PFNGLSHADERSOURCEPROC		glShaderSource			= nullptr;
PFNGLCOMPILESHADERPROC		glCompileShader			= nullptr;
PFNGLATTACHSHADERPROC		glAttachShader			= nullptr;
PFNGLDETACHSHADERPROC		glDetachShader			= nullptr;
PFNGLLINKPROGRAMPROC		glLinkProgram			= nullptr;
PFNGLGETUNIFORMLOCATIONPROC	glGetUniformLocation	= nullptr;
PFNGLUNIFORM1IPROC			glUniform1i				= nullptr;
PFNGLUNIFORM2FVPROC			glUniform2fv			= nullptr;
PFNGLUNIFORM4FVPROC			glUniform4fv			= nullptr;
#endif // !__APPLE__

void*  (APIENTRYP glXGetCurrentDisplay)()	= nullptr;
Uint32 (APIENTRYP glXGetCurrentDrawable)()	= nullptr;
void   (APIENTRYP glXSwapIntervalEXT)(
									void* display,
									Uint32 GLXDrawable,
									int interval);

Uint32 (APIENTRYP wglSwapIntervalEXT)(int interval);


/**
 * Creates OpenXcom's GL-interface layer.
 */
OpenGL::OpenGL()
	:
		gltexture(0u),
		glprogram(0u),
		fragmentshader(0u),
		linear(false),
		vertexshader(0u),
		buffer(nullptr),
		buffer_surface(nullptr),
		iwidth(0u),
		iheight(0u),
		iformat(GL_UNSIGNED_INT_8_8_8_8_REV),	// this didn't seem to be set anywhere before...
		ibpp(32u)								// ...nor this
{}

/**
 * dTor
 */
OpenGL::~OpenGL()
{
	terminate();
}

/**
 * Because you're too cool to initialize everything in the constructor.
 */
void OpenGL::init(
		int w,
		int h)
{
	// disable unused features
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_STENCIL_TEST);

	// enable useful and required features
	glEnable(GL_DITHER);
	glEnable(GL_TEXTURE_2D);

	// bind shader functions
#ifndef __APPLE__
	glCreateProgram			= (PFNGLCREATEPROGRAMPROC)					glGetProcAddress("glCreateProgram");
	glUseProgram			= (PFNGLUSEPROGRAMPROC)						glGetProcAddress("glUseProgram");
	glCreateShader			= (PFNGLCREATESHADERPROC)					glGetProcAddress("glCreateShader");
	glDeleteShader			= (PFNGLDELETESHADERPROC)					glGetProcAddress("glDeleteShader");
	glShaderSource			= (PFNGLSHADERSOURCEPROC)					glGetProcAddress("glShaderSource");
	glCompileShader			= (PFNGLCOMPILESHADERPROC)					glGetProcAddress("glCompileShader");
	glAttachShader			= (PFNGLATTACHSHADERPROC)					glGetProcAddress("glAttachShader");
	glDetachShader			= (PFNGLDETACHSHADERPROC)					glGetProcAddress("glDetachShader");
	glLinkProgram			= (PFNGLLINKPROGRAMPROC)					glGetProcAddress("glLinkProgram");
	glGetUniformLocation	= (PFNGLGETUNIFORMLOCATIONPROC)				glGetProcAddress("glGetUniformLocation");
	glUniform1i				= (PFNGLUNIFORM1IPROC)						glGetProcAddress("glUniform1i");
	glUniform2fv			= (PFNGLUNIFORM2FVPROC)						glGetProcAddress("glUniform2fv");
	glUniform4fv			= (PFNGLUNIFORM4FVPROC)						glGetProcAddress("glUniform4fv");
#endif
	glXGetCurrentDisplay	= (void* (APIENTRYP)())						glGetProcAddress("glXGetCurrentDisplay");
	glXGetCurrentDrawable	= (Uint32 (APIENTRYP)())					glGetProcAddress("glXGetCurrentDrawable");
	glXSwapIntervalEXT		= (void (APIENTRYP)(void*, Uint32, int))	glGetProcAddress("glXSwapIntervalEXT");

	wglSwapIntervalEXT		= (Uint32 (APIENTRYP)(int))					glGetProcAddress("wglSwapIntervalEXT");
	// kL_note: f*ck I hate re-direction.


	shader_support = glCreateProgram		!= nullptr
				  && glUseProgram			!= nullptr
				  && glCreateShader			!= nullptr
				  && glDeleteShader			!= nullptr
				  && glShaderSource			!= nullptr
				  && glCompileShader		!= nullptr
				  && glAttachShader			!= nullptr
				  && glDetachShader			!= nullptr
				  && glLinkProgram			!= nullptr
				  && glGetUniformLocation	!= nullptr
				  && glUniform1i			!= nullptr
				  && glUniform2fv			!= nullptr
				  && glUniform4fv			!= nullptr;

	if (shader_support == true)
		glprogram = glCreateProgram();

	// create surface texture
	resize(static_cast<unsigned>(w), static_cast<unsigned>(h));
}

/**
 * Exits - because destructors are uncool for people.
 * @note Helps the destructor.
 */
void OpenGL::terminate() // private.
{
	if (gltexture != 0u)
	{
		glDeleteTextures(1, &gltexture);
		gltexture = 0u;
	}

	if (buffer != nullptr)
	{
		buffer	= nullptr;
		iwidth	=
		iheight	= 0u;
	}

	delete buffer_surface;
}

/**
 * Makes all the pixels go away.
 */
void OpenGL::clear() // private.
{
	glClearColor(0.f,0.f,0.f,1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();

	glErrorCheck();
}

/**
 * Makes the buffer show up on-screen.
 */
void OpenGL::refresh(
		bool smooth,
		unsigned inwidth,
		unsigned inheight,
		unsigned outwidth,
		unsigned outheight,
		int topBlackBand,
		int bottomBlackBand,
		int leftBlackBand,
		int rightBlackBand)
{
	while (glGetError() != GL_NO_ERROR); // clear possible error from Lord knows where

	clear();

	if (shader_support == true
		&& (fragmentshader != 0u || vertexshader != 0u))
	{
		glUseProgram(glprogram);
		GLint location;

		float inputSize[2]
		{
			static_cast<float>(inwidth),
			static_cast<float>(inheight)
		};

		location = glGetUniformLocation(glprogram, "rubyInputSize");
		glUniform2fv(
					location,
					1,
					inputSize);

		float outputSize[2]
		{
			static_cast<float>(outwidth),
			static_cast<float>(outheight)
		};

		location = glGetUniformLocation(glprogram, "rubyOutputSize");
		glUniform2fv(
					location,
					1,
					outputSize);

		float textureSize[2]
		{
			static_cast<float>(iwidth),
			static_cast<float>(iheight)
		};

		location = glGetUniformLocation(glprogram, "rubyTextureSize");
		glUniform2fv(
					location,
					1,
					textureSize);
	}

	glErrorCheck();

	glTexParameteri(
				GL_TEXTURE_2D,
				GL_TEXTURE_WRAP_S,
				GL_CLAMP_TO_BORDER);
	glTexParameteri(
				GL_TEXTURE_2D,
				GL_TEXTURE_WRAP_T,
				GL_CLAMP_TO_BORDER);
	glTexParameteri(
				GL_TEXTURE_2D,
				GL_TEXTURE_MAG_FILTER,
				(smooth == true) ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(
				GL_TEXTURE_2D,
				GL_TEXTURE_MIN_FILTER,
				(smooth == true) ? GL_LINEAR : GL_NEAREST);

	glErrorCheck();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(
			0., static_cast<double>(outwidth),
			0., static_cast<double>(outheight),
			-1., 1.);
	glViewport(
			0,0,
			outwidth,
			outheight);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glErrorCheck();

	glPixelStorei(
				GL_UNPACK_ROW_LENGTH,
				static_cast<int>(buffer_surface->getSurface()->pitch)
			  / static_cast<int>(buffer_surface->getSurface()->format->BytesPerPixel));

	glErrorCheck();

	glTexSubImage2D(
				GL_TEXTURE_2D,
				/* mip-map level = */ 0,
				/* x = */ 0,
				/* y = */ 0,
				iwidth, iheight,
				GL_BGRA,
				iformat,
				buffer);

	// OpenGL projection sets 0,0 as *bottom-left* of screen.
	// therefore, below vertices flip image to support top-left source.
	// texture range = x1:0.0, y1:0.0, x2:1.0, y2:1.0
	// vertex range = x1:0, y1:0, x2:width, y2:height
	const float
		w (static_cast<float>(inwidth)  / static_cast<float>(iwidth)),
		h (static_cast<float>(inheight) / static_cast<float>(iheight));
	const int
		u1 (leftBlackBand),
		u2 (outwidth - rightBlackBand),
		v1 (outheight - topBlackBand),
		v2 (bottomBlackBand);

	glBegin(GL_TRIANGLE_STRIP);

	glTexCoord2f(0.f, 0.f);
	glVertex3i(u1, v1, 0);

	glTexCoord2f(w, 0.f);
	glVertex3i(u2, v1, 0);

	glTexCoord2f(0.f, h);
	glVertex3i(u1, v2, 0);

	glTexCoord2f(w, h);
	glVertex3i(u2, v2, 0);

	glEnd();

	glErrorCheck();

	glFlush();

	glErrorCheck();

	if (shader_support == true)
		glUseProgram(0u);
}

/**
 * Resizes the internal buffer.
 */
void OpenGL::resize( // private.
		unsigned width,
		unsigned height)
{
	if (gltexture == 0u)
		glGenTextures(1, &gltexture);

	glErrorCheck();

	iwidth = width;
	iheight = height;

	if (buffer_surface)
		delete buffer_surface;

	buffer_surface = new Surface( // use OpenXcom's Surface class to get an aligned-buffer with bonus SDL_Surface
								iwidth,
								iheight,
								0,0,
								ibpp);

	buffer = static_cast<uint32_t*>(buffer_surface->getSurface()->pixels);

	glBindTexture(
				GL_TEXTURE_2D,
				gltexture);
	glErrorCheck();
	glPixelStorei(
				GL_UNPACK_ROW_LENGTH,
				iwidth);
	glErrorCheck();
	glTexImage2D(
				GL_TEXTURE_2D,
				/* mip-map level = */ 0,
				/* internal format = */ GL_RGB16_EXT,
				width,
				height,
				/* border = */ 0,
				/* format = */ GL_BGRA,
				iformat,
				buffer);
	glErrorCheck();
}

/**
 * Sets a shader!
 */
void OpenGL::set_shader(const char* source_yaml_filename)
{
	if (shader_support == true)
	{
		if (fragmentshader != 0u)
		{
			glDetachShader(
						glprogram,
						fragmentshader);
			glDeleteShader(fragmentshader);
			fragmentshader = 0u;
		}

		if (vertexshader != 0u)
		{
			glDetachShader(
						glprogram,
						vertexshader);
			glDeleteShader(vertexshader);
			vertexshader = 0u;
		}

		if (source_yaml_filename != nullptr
			&& std::strlen(source_yaml_filename) != 0u)
		{
			try
			{
				YAML::Node document (YAML::LoadFile(source_yaml_filename));

				linear = document["linear"].as<bool>(false); // some shaders want texture-linear-interpolation and some don't

				if (document["language"].as<std::string>() == "GLSL")
				{
					const std::string
						fragment_source	(document["fragment"]	.as<std::string>("")), // NOTE: YAML needs those quotes here.
						vertex_source	(document["vertex"]		.as<std::string>(""));

					if (fragment_source.empty() == false)
						set_fragment_shader(fragment_source.c_str());

					if (vertex_source.empty() == false)
						set_vertex_shader(vertex_source.c_str());
				}
			}
			catch (YAML::Exception &e)
			{
				Log(LOG_ERROR) << source_yaml_filename << ": " << e.what();
			}
		}

		glLinkProgram(glprogram);
	}
}

/**
 * Sets a fragment-shader.
 */
void OpenGL::set_fragment_shader(const char* source) // private.
{
	fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(
				fragmentshader,
				1,
				&source,
				0);
	glCompileShader(fragmentshader);
	glAttachShader(
				glprogram,
				fragmentshader);
}

/**
 * Sets a vertex-shader.
 */
void OpenGL::set_vertex_shader(const char* source) // private.
{
	vertexshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(
				vertexshader,
				1,
				&source,
				0);
	glCompileShader(vertexshader);
	glAttachShader(
				glprogram,
				vertexshader);
}

/**
 * Tries to set VSync.
 * @note Currently not working in Battlescape ... or it is but doesn't get set.
 * Or I'm using hardware or software or this or that or double-buffering or
 * SDL with OpenGL or SDL without OpenGL or OpenGL without SDL ... anyway my
 * screen tears. I don't like it.
 *
 * Later: It's not v-sync per se. The tearing is caused elsewhere, and appears
 * related to CPU and/or code-flow-looping-agorithic complications. Eg,
 * horizontal scrolling over the top-level of a battleship (with increased
 * looping-over-tiles inside the UFO, but lower frame-rates) causes more tearing;
 * while horizontal scrolling on the ground-level (less tiles to loop over) shows
 * no tearing (although frame-rates then exceed monitor-refresh rate, supposedly
 * the only time that tearing can occur). Similarly, using edge-scroll (with lots
 * of event-calls) causes more tearing for respective levels than drag-scroll.
 *
 * And/or it's just my danged video-card(s). It strikes as sort of a
 * RAM-bottleneck, whether video-RAM or system-RAM .... Changing the nVidia
 * settings "Vertical sync" and/or "Maximum pre-rendered frames" doesn't affect
 * it; only a few initial jitters after changing values but then things settle
 * down to a baseline.
 *
 * btw, this setVSync() option doesn't affect it either; on the other hand the
 * tearing *is a lot worse* if not using OpenGL at all.
 *
 * Even later: I'd say definitely CPU: it gets worse when the lexer is parsing @ 80%.
 */
void OpenGL::setVSync(bool sync)
{
	int interval;
	if (sync == true) interval = 1;
	else			  interval = 0;

	if (   glXGetCurrentDisplay		!= nullptr
		&& glXGetCurrentDrawable	!= nullptr
		&& glXSwapIntervalEXT		!= nullptr)
	{
		void* display (glXGetCurrentDisplay());

		const Uint32 drawable (glXGetCurrentDrawable());
		if (drawable != 0u)
			glXSwapIntervalEXT(
							display,
							drawable,
							interval);
	}
	else if (wglSwapIntervalEXT != 0u)
		wglSwapIntervalEXT(interval);	// Other:
										// SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
										// SDL_GL_SetSwapInterval(0)
										// cf. Screen::setVideoFlags()
}

/**
 * Sets a pointer to a data-buffer where an image is stored.
 * @param data	- reference to a pointer to data
 * @param pitch	- reference to pitch
 * @return, true or false
 *
bool OpenGL::lock(
		uint32_t*& data,
		unsigned& pitch)
{
	pitch = iwidth * ibpp;
	return (data = buffer) != nullptr;
} */

}

#endif
