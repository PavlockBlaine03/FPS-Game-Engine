#include <core/Application.h>
#include <iostream>

int main()
{
	try
	{
		Application app(2560, 1440, "OpenGL FPS Game");
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
		return 1;
	}
	return 0;
}