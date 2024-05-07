#pragma once
#include "Core.h"
#include "FormatsAndTypes.h"
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif
namespace RHI
{
	class RHI_API Surface
	{
	public:
#ifdef _WIN32
		void InitWin32(HWND hwnd, Internal_ID instance);
#endif

#ifdef __ANDROID__
		void InitAndroid();
#endif

#ifdef __linux__
		//void InitLinux();
#endif

#ifdef USE_GLFW
		void InitGLFW(GLFWwindow* window, Internal_ID instance);
#endif
		Internal_ID ID = 0;
	};
}