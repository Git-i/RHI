#pragma once
#include "Object.h"
#include "FormatsAndTypes.h"
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif
namespace RHI
{
	class Instance;
	class RHI_API Surface : public Object
	{
	public:
#ifdef _WIN32
		RHI::CreationError InitWin32(HWND hwnd, Internal_ID instance);
#endif

#ifdef __ANDROID__
		void InitAndroid();
#endif

#ifdef __linux__
		//void InitLinux();
#endif

#ifdef USE_GLFW
		static creation_result<Surface> InitGLFW(GLFWwindow* window, Ptr<Instance> instance);
#endif
		Internal_ID ID = 0;
	};
}