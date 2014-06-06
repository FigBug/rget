#pragma once

struct Options
{
	Options()
      : clearHistory(false),
		openAll(false),
		displayHelp(false),
		displayVersion(false),
		numToOpen(30),
		createSubdir(false)
	{
	}

	bool clearHistory;
	bool openAll;
	bool displayHelp;
	bool displayVersion;
	int numToOpen;
	string output;
	bool createSubdir;
	vector<string> subreddits;
};

struct HistoryItem
{
	HistoryItem()
	  : date(0)
	{
	}

	string id;
	time_t date;
};