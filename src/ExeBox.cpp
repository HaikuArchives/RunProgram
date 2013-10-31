#include "ExeBox.h"
#include <Entry.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <storage/Node.h>
#include <OS.h>
#include <File.h>
#include <storage/Path.h>
#include <Locker.h>
#include "ObjectList.h"

#include <Volume.h>
#include <VolumeRoster.h>
#include <Query.h>

static BObjectList<entry_ref> *sPathData = NULL;
BLocker sPathDataLock;
thread_id sThreadID = -1;

void PrintList(const char* s)
{
	sPathDataLock.Lock();
	int len = sPathData->CountItems();

	printf("Count: %i\nsPathData (%s) :\n", len, s);	

	for ( int i = 0; i < len ; i += 3 )
	{	
		printf("%i: %-13.13s\t", i, sPathData->ItemAt(i)->name);
		if ( i+1 < len )
			printf("%i: %-13.13s\t", i+1, sPathData->ItemAt(i+1)->name);
		if ( i+2 < len )
			printf("%i: %-13.13s\t", i+2, sPathData->ItemAt(i+2)->name);

		printf("\n");
	}
	sPathDataLock.Unlock();
}


ExeBoxFilter::ExeBoxFilter(ExeBox *box)
 :	AutoTextControlFilter(box)
{
}


filter_result
ExeBoxFilter::KeyFilter(const int32 &key, const int32 &mod)
{
	if (key < 32 || ( (mod & B_COMMAND_KEY) && !(mod & B_SHIFT_KEY) &&
					!(mod & B_OPTION_KEY) && !(mod & B_CONTROL_KEY)) )
		return B_DISPATCH_MESSAGE;
		

	int32 start, end;
	TextControl()->TextView()->GetSelection(&start,&end);
	if (end == (int32)strlen(TextControl()->Text()))
	{
		TextControl()->TextView()->Delete(start,end);
		
		BString string;
		if( GetCurrentMessage()->FindString("bytes",&string) != B_OK)
			string = "";
		string.Prepend(TextControl()->Text());
				
		entry_ref *match = NULL, *data;;
	
		sPathDataLock.Lock();
		for (int32 i = 0; i < sPathData->CountItems(); i++)
		{	
			data = sPathData->ItemAt(i);
			BString namestr(data->name);
		
			if (!data->name)
				continue;
		
			if (namestr.IFindFirst(string) == 0)
			{
				match = data;
				break;
			}
		}
		sPathDataLock.Unlock();
	
		if(match)
		{
			BMessage automsg(M_EXE_AUTOCOMPLETE);
			automsg.AddInt32("start",strlen(TextControl()->Text())+1);
			automsg.AddString("string",match->name);
			
			BMessenger msgr((BHandler*)TextControl()->Window());
			msgr.SendMessage(&automsg);
		}
	}
	
	
	return B_DISPATCH_MESSAGE;
}


ExeBox::ExeBox(const BRect &frame, const char *name, const char *label,
			const char *text, BMessage *msg, uint32 resize, uint32 flags)
 :	AutoTextControl(frame,name,label,text,msg,resize,flags)
{
	if (!sPathData)
		InitializeAutocompletion();
	
	SetFilter(new ExeBoxFilter(this));
	SetCharacterLimit(32);
}


ExeBox::~ExeBox(void)
{
	int32 value;
	sPathDataLock.Lock();
	if (sThreadID >= 0)
	{
		sPathDataLock.Unlock();
		wait_for_thread(sThreadID,&value);
	}
	else
		sPathDataLock.Unlock();
}


entry_ref *
ExeBox::FindMatch(const char *name)
{
	// Handle the autocompletion
	
	if (!name)
		return NULL;
	
	entry_ref *match = NULL;

	sPathDataLock.Lock();
	for (int32 i = 0; i < sPathData->CountItems(); i++)
	{
		entry_ref *data = sPathData->ItemAt(i);
		if (!data->name)
			continue;
		
		BString refstr(data->name);
		if (refstr.IFindFirst(name) == 0)
		{
			match = data;
			break;
		}
	}
	sPathDataLock.Unlock();

	return match;
}


void
ExeBox::InitializeAutocompletion(void)
{
	sPathData = new BObjectList<entry_ref>(20,true);
	
	// Set up autocomplete
	BEntry entry("/boot/home/config/settings/Run Program");
	BPath path;
	entry_ref ref;
	
	if (entry.Exists())
	{
		BFile settingsFile("/boot/home/config/settings/Run Program",B_READ_ONLY);
		BMessage settings;
		if (settings.Unflatten(&settingsFile) == B_OK)
		{
			int32 index = 0;
			while (settings.FindRef("refs",index,&ref) == B_OK)
			{
				index++;
				
				entry_ref *listref = new entry_ref(ref);
				sPathData->AddItem(listref);
			}
			
			sThreadID = spawn_thread(UpdateThread,
							"updatethread",	B_NORMAL_PRIORITY,	NULL);
			if (sThreadID >= 0)
				resume_thread(sThreadID);
			return;
		}
		
	}
	
	BDirectory dir;
	
	find_directory(B_BEOS_BIN_DIRECTORY,&path);
	dir.SetTo(path.Path());
	dir.Rewind();
	while (dir.GetNextRef(&ref) == B_OK)
	{
		entry.SetTo(&ref,true);
		entry.GetRef(&ref);
		
		entry_ref *match = NULL;
		for (int32 i = 0; i < sPathData->CountItems(); i++)
		{
			entry_ref *listref = sPathData->ItemAt(i);
			if (*listref == ref)
			{
				match = listref;
				break;
			}
		}
		if (!match)
			sPathData->AddItem(new entry_ref(ref));
	}
	
	find_directory(B_COMMON_BIN_DIRECTORY,&path);
	dir.SetTo(path.Path());
	dir.Rewind();
	while (dir.GetNextRef(&ref) == B_OK)
	{
		entry.SetTo(&ref, true);
		entry.GetRef(&ref);
		
		entry_ref *match = NULL;
		for (int32 i = 0; i < sPathData->CountItems(); i++)
		{
			entry_ref *listref = sPathData->ItemAt(i);
			if (*listref == ref)
			{
				match = listref;
				break;
			}
		}
		if (!match)
			sPathData->AddItem(new entry_ref(ref));
	}

	sThreadID = spawn_thread(QueryThread, "querythread", B_NORMAL_PRIORITY, NULL);
	if (sThreadID >= 0)
		resume_thread(sThreadID);
	
}

static
int
CompareRefs(const entry_ref *ref1, const entry_ref *ref2)
{
	if (!ref1)
		return -1;
	else if (!ref2)
		return 1;
	
	return strcmp(ref1->name,ref2->name);
}

int32
ExeBox::QueryThread(void *data)
{
	BVolumeRoster 	roster;
	BVolume 		vol;
	BQuery			query;
	entry_ref		ref;

	roster.GetBootVolume(&vol);
	
	query.SetVolume(&vol);
	query.SetPredicate("((BEOS:APP_SIG==\"application*\")&&"
						"(BEOS:TYPE==\"application/x-vnd.Be-elfexecutable\"))");
	query.Fetch();

	while (query.GetNextRef(&ref) == B_OK)
	{
		if ( ref.directory == B_USER_ADDONS_DIRECTORY 
			|| ref.directory == B_BEOS_ADDONS_DIRECTORY
			|| ref.directory == B_COMMON_ADDONS_DIRECTORY)
			continue;

		if (ref.directory == -1)
		{
			fprintf(stderr, "FATAL : Query returned invalid ref!\n");
			return -1;
		}

		sPathDataLock.Lock();
		sPathData->AddItem(new entry_ref(ref));
		sPathDataLock.Unlock();
	}
	
	BMessage settings;
	
	sPathDataLock.Lock();
	for (int32 i = 0; i < sPathData->CountItems(); i++)
		settings.AddRef("refs", sPathData->ItemAt(i));
	sPathDataLock.Unlock();
	
	BFile settingsFile("/boot/home/config/settings/Run Program", B_READ_WRITE |
																B_ERASE_FILE |
																B_CREATE_FILE);
	settings.Flatten(&settingsFile);
	
	sPathDataLock.Lock();
	sThreadID = -1;
	sPathDataLock.Unlock();
	return 0;
}


int32
ExeBox::UpdateThread(void *data)
{
	// This takes longer, but it's not really time-critical. Most of the time there
	// will only be minor differences between run sessions.
	
	// In order to save as much time as possible:
	// 1) Get a new ref list via scanning the usual places. This won't take long, thanks
	//	  to the file cache. :)
	// 2) For each item in the official list
	//		- if entry doesn't exist, remove it
	//		- if entry does exist, remove it from the temporary list
	// 3) Add any remaining entries in the temporary list to the official one
	// 4) Update the disk version of the settings file
	
	BObjectList<entry_ref> queryList(20,true);
	
	BDirectory 		dir;
	BPath 			path;
	entry_ref 		ref,	*pref;
	BEntry 			entry;
	BVolumeRoster 	roster;
	BVolume 		vol;
	BQuery 			query;

	find_directory(B_BEOS_BIN_DIRECTORY,&path);
	dir.SetTo(path.Path());
	dir.Rewind();
	
	while (dir.GetNextRef(&ref) == B_OK)
	{
		entry.SetTo(&ref, true);
		entry.GetRef(&ref);
		entry.Unset();
		queryList.AddItem(new entry_ref(ref));
	}
	
	find_directory(B_COMMON_BIN_DIRECTORY,&path);
	dir.SetTo(path.Path());
	dir.Rewind();

	while (dir.GetNextRef(&ref) == B_OK)
	{
		entry.SetTo(&ref,true);
		entry.GetRef(&ref);
		entry.Unset();
		queryList.AddItem(new entry_ref(ref));
	}
	
	roster.GetBootVolume(&vol);
	query.SetVolume(&vol);
	query.SetPredicate("((BEOS:APP_SIG==\"application*\")&&"
						"(BEOS:TYPE==\"application/x-vnd.Be-elfexecutable\"))");
	query.Fetch();
	
	while (query.GetNextRef(&ref) == B_OK)
	{
		entry.SetTo(&ref,true);
		entry.GetPath(&path);
		entry.GetRef(&ref);
		entry.Unset();
		if (strstr(path.Path(),"add-ons") || strstr(path.Path(),"beos/servers") ||
			!ref.name)
			continue;
		queryList.AddItem(new entry_ref(ref));
	}
		
	// Scan is finished, so start culling
	sPathDataLock.Lock();
	int32 count = sPathData->CountItems();
	sPathDataLock.Unlock();
	
	for (int32 i = 0; i < count; i++)
	{
		sPathDataLock.Lock();
		pref = sPathData->ItemAt(i);
		entry.SetTo(pref);
		
		if (! entry.Exists() )
			sPathData->RemoveItemAt( i );
		else
		{
			for (int32 j = 0; j < queryList.CountItems(); j++)
			{
				if (*pref == *queryList.ItemAt(j))
				{
					queryList.RemoveItemAt(j);
					break;
				}
			}
		}
		entry.Unset();
		sPathDataLock.Unlock();
	}
	
	// Remaining items in queryList are new entries
	sPathDataLock.Lock();
	for (int32 j = 0; j < queryList.CountItems(); j++)
	{
		entry_ref *qref = queryList.RemoveItemAt(j);
		if (qref &&	qref->directory >= 0)
			sPathData->AddItem(qref);
	}
	sPathDataLock.Unlock();
	
	
	BMessage settings;
	
	sPathDataLock.Lock();
 	sPathData->SortItems(CompareRefs);
	sPathDataLock.Unlock();
	
	sPathDataLock.Lock();
	for (int32 i = 0; i < sPathData->CountItems(); i++)
	{
		entry_ref *listref = sPathData->ItemAt(i);
		settings.AddRef("refs",listref);
	}
	sPathDataLock.Unlock();
		
	BFile settingsFile("/boot/home/config/settings/Run Program", B_READ_WRITE |
																B_ERASE_FILE |
																B_CREATE_FILE);
	settings.Flatten(&settingsFile);
	
	sPathDataLock.Lock();
	sThreadID = -1;
	sPathDataLock.Unlock();
	
	return 0;
}
