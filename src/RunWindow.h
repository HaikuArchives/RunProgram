#ifndef RUNWINDOW_H
#define RUNWINDOW_H

#include <Window.h>
#include <String.h>

class ExeBox;

class RunWindow : public BWindow
{
public:
			RunWindow(void);
			~RunWindow(void);
	bool	QuitRequested(void);
	void	MessageReceived(BMessage *msg);
	
private:
	void	SetPosition(void);
	ExeBox		*fRunBox;
};

#endif
