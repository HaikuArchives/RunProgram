#ifndef EXEBOX_H
#define EXEBOX_H

#include <String.h>
#include "AutoTextControl.h"

// This class is responsible for all the neato autocompletion stuff
/*
class RunPathData
{
public:
	RunPathData(const char *szName, const char *szPath)
	{
		name = szName;
		path = szPath;
	}
	BString name;
	BString path;
};
*/

enum
{
	M_EXE_AUTOCOMPLETE='excp'
};

class ExeBox;

class ExeBoxFilter : public AutoTextControlFilter
{
public:
	ExeBoxFilter(ExeBox *box);
	filter_result KeyFilter(const int32 &key, const int32 &mod);
};

class ExeBox : public AutoTextControl
{
public:
	ExeBox(const BRect &frame, const char *name, const char *label,
			const char *text, BMessage *msg,
			uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	~ExeBox(void);
	
//	RunPathData *FindMatch(const char *name);
	entry_ref	*FindMatch(const char *name);
	
private:
	static	void	InitializeAutocompletion(void);
	static	int32	QueryThread(void *data);
	static	int32	UpdateThread(void *data);
	
	friend ExeBoxFilter;
};

#endif
