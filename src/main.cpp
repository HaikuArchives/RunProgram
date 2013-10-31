#include <Application.h>
#include "RunWindow.h"

class App : public BApplication
{
public:
	App(void);
};

App::App(void)
 :	BApplication("application/x-vnd.dw-RunProgram")
{
	RunWindow *runwin = new RunWindow;
	runwin->Show();
}


int main(void)
{
	App().Run();
	return 0;
}