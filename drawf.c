#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <glalgebra.h>

void drawf_fv(GLint name, GLfloat *val, int num)
{
	switch (num) {
	case 1: glUniform1fv(name, 1, val); break;
	case 2: glUniform2fv(name, 1, val); break;
	case 3: glUniform3fv(name, 1, val); break;
	case 4: glUniform4fv(name, 1, val); break;
	}
}

void drawf_iv(GLint name, GLint *val, int num)
{
	switch (num) {
	case 1: glUniform1iv(name, 1, val); break;
	case 2: glUniform2iv(name, 1, val); break;
	case 3: glUniform3iv(name, 1, val); break;
	case 4: glUniform4iv(name, 1, val); break;
	}
}

void drawf_uv(GLint name, GLuint *val, int num)
{
	switch (num) {
	case 1: glUniform1uiv(name, 1, val); break;
	case 2: glUniform2uiv(name, 1, val); break;
	case 3: glUniform3uiv(name, 1, val); break;
	case 4: glUniform4uiv(name, 1, val); break;
	}
}

void drawf_mat(GLint name, int transp, GLfloat *val, int x, int y)
{
	//Array of OpenGL matrix functions, x and y will index into this to call the right one efficiently.
	void (*matrix_func[])(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = {
		glUniformMatrix2fv,
		glUniformMatrix2x3fv,
		glUniformMatrix2x4fv,
		glUniformMatrix3x2fv,
		glUniformMatrix3fv,
		glUniformMatrix3x4fv,
		glUniformMatrix4x2fv,
		glUniformMatrix4x3fv,
		glUniformMatrix4fv
	};

	//No parameter checking for x and y because suck my diiiiiiiick
	matrix_func[3*(x-2) + (y-2)](name, 1, transp, val);
}

void drawf(char *format, ...)
{
	char *str = format;
	char *end = NULL;
	char *sep = " %";
	char *valid = "^*1234fiumx."; //Valid format specifier chars.
	GLenum transp = GL_TRUE;
	va_list args;
	va_start(args, format);

	do {
		str = strpbrk(str, sep);
		if (str)
			str += strspn(str, sep); //Skip to start of a format specifier, or end of str.
		else
			break;

		end = str + strspn(str, valid); //Find end of format specifier.
		if (str == end) //str pointed at \0 or an invalid character.
			break;

		end--; //Nudge end back to the last character of this format specifier.

		GLint name = va_arg(args, GLint); //The first arg will be the OpenGL uniform name.

		//Determine if it's a matrix format specifier.
		if (*end == 'm') {
			if (*str ==  '^') { //Check if it's a transpose matrix.
				transp = GL_FALSE;
				str++;
			}
			if (*str == 'm') { //"%m" means an amat4 is being passed.
				amat4 val = va_arg(args, amat4);
				GLfloat buf[16];
				amat4_to_array(val, buf);
				glUniformMatrix4fv(name, 1, transp, buf);
			} else {
				GLfloat *val = va_arg(args, GLfloat *);
				drawf_mat(name, transp, val, *str - '0', *(end-1) - '0');
			}
			continue;
		}

		//Determine if it's a vector type, pointing to a buffer holding the components.
		if (*str == '*') {
			str++; //Advance str to point at the vector size or the type (implying size 3).
			switch (*end) {
			case 'f': {
					GLfloat *val = va_arg(args, GLfloat *);
					drawf_fv(name, val, str == end ? 3 : *str - '0');
				} break;
			case 'i': {
					GLint *val = va_arg(args, GLint *);
					drawf_iv(name, val, str == end ? 3 : *str - '0');
				} break;
			case 'u': {
					GLuint *val = va_arg(args, GLuint *);
					drawf_uv(name, val, str == end ? 3 : *str - '0');
				} break;
			}
			continue;
		}

		//The only remaining possibility is a vector type, passing components directly.
		int vsize = str == end ? 1 : *str - '0';
		switch (*end) {
		case 'f': {
				GLfloat v[4];
				for (int i = 0; i < vsize; i++) 
					v[i] = va_arg(args, double);

				switch (vsize) {
				case 1: glUniform1f(name, v[0]); break;
				case 2: glUniform2f(name, v[0], v[1]); break;
				case 3: glUniform3f(name, v[0], v[1], v[2]); break;
				case 4: glUniform4f(name, v[0], v[1], v[2], v[3]); break;
				}
			} break;
		case 'i': {
				GLint v[4];
				for (int i = 0; i < vsize; i++) 
					v[i] = va_arg(args, GLint);

				switch (vsize) {
				case 1: glUniform1i(name, v[0]); break;
				case 2: glUniform2i(name, v[0], v[1]); break;
				case 3: glUniform3i(name, v[0], v[1], v[2]); break;
				case 4: glUniform4i(name, v[0], v[1], v[2], v[3]); break;
				}
			} break;
		case 'u': {
				GLuint v[4];
				for (int i = 0; i < vsize; i++) 
					v[i] = va_arg(args, GLuint);

				switch (vsize) {
				case 1: glUniform1ui(name, v[0]); break;
				case 2: glUniform2ui(name, v[0], v[1]); break;
				case 3: glUniform3ui(name, v[0], v[1], v[2]); break;
				case 4: glUniform4ui(name, v[0], v[1], v[2], v[3]); break;
				}
			} break;
		default: continue; //Not a valid type.
		}

	} while (str && *str);

	va_end(args);
}
