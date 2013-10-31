#include "RunWindow.h"
#include <Application.h>
#include <View.h>
#include <Button.h>
#include <Entry.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Deskbar.h>
#include <Screen.h>
#include <stdlib.h>
#include <stdio.h>


#include "ExeBox.h"
#include "EscapeCancelFilter.h"

#define M_RUN_COMMAND		'rncm'
#define M_COMMAND_CHANGED	'cmch'

RunWindow::RunWindow(void)
 :	BWindow(BRect(100,100,400,300),"Run…",B_TITLED_WINDOW,
 			B_ASYNCHRONOUS_CONTROLS | B_NOT_V_RESIZABLE)
{
	BView *top = new BView(Bounds(),"top",B_FOLLOW_ALL,B_WILL_DRAW);
	top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(top);
	
	AddCommonFilter(new EscapeCancelFilter);
	
	BButton *fOK = new BButton(BRect(0,0,1,1),"okbutton","OK",
								new BMessage(M_RUN_COMMAND),
								B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	top->AddChild(fOK);
	fOK->ResizeToPreferred();
	fOK->MoveTo(Bounds().right - fOK->Bounds().Width() - 10,10);
	fOK->MakeDefault(true);
	
	fRunBox = new ExeBox(BRect(10,10,11,11),"runbox","Command:",NULL,
								new BMessage(M_COMMAND_CHANGED),
								B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	fRunBox->SetDivider(fRunBox->StringWidth("Command:") + 5);
	top->AddChild(fRunBox);
	
	fRunBox->ResizeToPreferred();
	fRunBox->ResizeTo(fOK->Frame().left - 20, fRunBox->Bounds().Height());
	
	ResizeTo(fOK->Frame().right + 10, fOK->Frame().bottom + 10);
	
	SetPosition();
	
	fRunBox->MakeFocus(true);
}


RunWindow::~RunWindow(void)
{
}


bool
RunWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
RunWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case M_EXE_AUTOCOMPLETE:
		{
			BString string;
			int32 start;
			msg->FindInt32("start",&start);
			msg->FindString("string",&string);
			fRunBox->SetText(string.String());
			fRunBox->TextView()->Select(start,string.CountChars());
			break;
		}
		case M_RUN_COMMAND:
		{
			Hide();
			BString cmd, cmdname;
			cmd = cmdname = fRunBox->Text();
			
			int32 pos = cmdname.FindFirst(" ");
			if (pos > 0)
				cmdname.Truncate(pos);
//			RunPathData *match = fRunBox->FindMatch(cmdname.String());
//			if (match)
			entry_ref *matchref = fRunBox->FindMatch(cmdname.String());;
			if (matchref)
			{
				cmd.RemoveFirst(cmdname.String());
				cmd.Prepend("'");
//				cmd.Prepend(match->path.String());
				cmd.Prepend(BPath(matchref).Path());
				cmd.Prepend("'");
			}
			system(cmd.String());
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}


void
RunWindow::SetPosition(void)
{
	BRect dbrect;
	BRect srect = BScreen().Frame();
	BDeskbar db;
	
	dbrect = db.Frame();
	
	switch (db.Location())
	{
		case B_DESKBAR_LEFT_BOTTOM:
		case B_DESKBAR_BOTTOM:
		{
			MoveTo(5,dbrect.top - Frame().Height() - 8);
			break;
		}
		case B_DESKBAR_RIGHT_BOTTOM:
		{
			MoveTo(srect.right - Frame().Width() - 5,dbrect.top - Frame().Height() - 8);
			break;
		}
		case B_DESKBAR_LEFT_TOP:
		case B_DESKBAR_TOP:
		{
			MoveTo(5,dbrect.bottom + 25);
			break;
		}
		case B_DESKBAR_RIGHT_TOP:
		{
			if (db.IsExpanded())
				MoveTo(dbrect.left - Frame().Width() - 5,25);
			else
				MoveTo(srect.right - Frame().Width() - 5,dbrect.bottom + 5);
			break;
		}
		default:
			break;
	}
}

