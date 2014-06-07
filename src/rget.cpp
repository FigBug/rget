#include "mainheaders.h"
#include "rget.h"
#include "url.h"
#include "json.h"
#include "ultragetopt.h"
#include "tinyxml.h"

#ifdef _WIN32
#define DIR_SEP "\\"
#endif

#ifdef __APPLE__
#include <CoreFoundation/CFBundle.h>
#include <ApplicationServices/ApplicationServices.h>
#define DIR_SEP "/"
#endif

string toStr(int v)
{
    std::stringstream ss;
    ss << v;
    return ss.str();
}

json_value* toJson(string json, block_allocator& allocator)
{
	char* errorPos  = 0;
	char* errorDesc = 0;

	int errorLine   = 0;

	char* source = new char[json.size() + 1];        
	strcpy(source, json.c_str());
	json_value* root = json_parse(source, &errorPos, &errorDesc, &errorLine, &allocator);

	return root;
}

void openUrl(string url)
{
#ifdef _WIN32
	wstring wurl(url.begin(), url.end());
	ShellExecute(NULL, L"open", wurl.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#endif
#ifdef __APPLE__
  	CFURLRef urlref = CFURLCreateWithBytes(NULL, (UInt8*)url.c_str(), url.length(), kCFStringEncodingASCII, NULL);
	LSOpenCFURLRef(urlref,0);
	CFRelease(urlref);
#endif
}

string getDataDir()
{
#ifdef _WIN32
	wchar_t path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);

	wcscat(path, L"\\rget");

	CreateDirectory(path, NULL);

	wstring tmp(path);

	return string(tmp.begin(), tmp.end());
#endif
#ifdef __APPLE__
	char path[1000] = { 0 };

	strcpy(path, "~/.rget");

	wordexp_t exp_result;
	wordexp(path, &exp_result, 0);
    if (exp_result.we_wordc > 0)
        strcpy(path, exp_result.we_wordv[0]);
	wordfree(&exp_result);

	mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	return path;
#endif
}

int doMkdir(const char* path, mode_t mode)
{
    struct stat st;
    int  status = 0;

    if (stat(path, &st) != 0)
    {
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = -1;
    }

    return status;
}

int mkpath(const char* path, mode_t mode)
{
    char* pp;
    char* sp;
    int status;
    char* copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            *sp = '\0';
            status = doMkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = doMkdir(path, mode);
    free(copypath);
    return status;
}

void loadHistory(string subreddit, vector<HistoryItem>& history)
{
	string dataDir = getDataDir();
	string xmlFile = dataDir + DIR_SEP + subreddit + ".xml";

	TiXmlDocument doc(xmlFile.c_str());
	if (doc.LoadFile())
	{
		TiXmlElement* root = doc.RootElement();
		if (root)
		{
			TiXmlElement* historyNode = root->FirstChildElement("history");
			while (historyNode)
			{
				HistoryItem itm;

				itm.id   = historyNode->Attribute("id");
				itm.date = atoi(historyNode->Attribute("date"));

				history.push_back(itm);
				
				historyNode = historyNode->NextSiblingElement("history");
			}
		}
	}
}

void saveHistory(string subreddit, vector<HistoryItem>& history)
{
	string dataDir = getDataDir();
	string xmlFile = dataDir + DIR_SEP + subreddit + ".xml";

	TiXmlDocument doc(xmlFile.c_str());
	
	TiXmlElement root("histories");

	for (int i = 0; i < (int)history.size(); i++)
	{
		TiXmlElement historyNode("history");

		historyNode.SetAttribute("id", history[i].id.c_str());
		historyNode.SetAttribute("date", (int)history[i].date);

		root.InsertEndChild(historyNode);
	}
	doc.InsertEndChild(root);

	doc.SaveFile();
}

bool isInHistory(vector<HistoryItem>& history, string id)
{
	for (int i = 0; i < (int)history.size(); i++)
	{
		if (history[i].id == id)
			return true;
	}
	return false;
}

bool hasEnding(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) 
    {
        return (fullString.compare (fullString.length() - ending.length(), ending.length(), ending) == 0);
    } 
    else 
    {
        return false;
    }
}

bool isImageUrl(string url)
{
	std::transform(url.begin(), url.end(), url.begin(), ::tolower);
	
	if (hasEnding(url, ".gif")) return true;	
	if (hasEnding(url, ".jpg")) return true;	
	if (hasEnding(url, ".jpe")) return true;	
	if (hasEnding(url, ".jpeg")) return true;	
	if (hasEnding(url, ".png")) return true;	
	
	return false;
}

bool isImgurUrl(string url)
{
	if (url.find("imgur.com") == string::npos)
		return false;
		
	return true;
}		

bool saveImage(const string& subreddit, const Options& options, string url, string prefix = "")
{
	string imgData = fetchUrl(url);
	if (imgData.empty())
		return false;
	
	string dir;
	if (options.createSubdir)
		dir = subreddit + DIR_SEP;
	if (options.output.size() > 0)
		dir = options.output + DIR_SEP + dir;
		
	string fname = prefix + string(strrchr(url.c_str(), '/') + 1);
	if (dir.size() > 0)
	{
		mkpath(dir.c_str(), 0755);
		fname = dir + fname;
	}	
	
	FILE* fp = fopen(fname.c_str(), "wb");
	if (fp)
	{
		fwrite(imgData.data(), imgData.size(), 1, fp);
		fclose(fp);
	}
	return true;
}

bool saveImgurUrl(const string& subreddit, const Options& options, string url)
{
	if (strstr(url.c_str(), "/a/"))
	{
		string key = strstr(url.c_str(), "/a/") + 3;
		
		string jsonUrl = "https://api.imgur.com/3/album/" + key + "/images";
		
		string page = fetchImgurUrl(jsonUrl);
		
		int idx = 0;
		
		if (page.size() > 0)
		{
			// convert to json
			block_allocator allocator(1 << 10);
			json_value* root = toJson(page, allocator);
			if (root)
			{
				for (json_value* it = root->first_child; it; it = it->next_sibling)
				{
					if (!strcmp(it->name, "data") && it->type == JSON_ARRAY)
					{
						for (json_value* image = it->first_child; image; image = image->next_sibling)
						{
							idx++;
							for (json_value* it = image->first_child; it; it = it->next_sibling)
							{
								if (!strcmp(it->name, "link")) 
								{
									string link = it->string_value;
									
									if (link.size() > 0)
									{
										char buffer[1024];
										sprintf(buffer, "%s-%d-", key.c_str(), idx);
									
										saveImage(subreddit, options, link, buffer);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
        string key = strrchr(url.c_str(), '/') + 1;
        
        string jsonUrl = "https://api.imgur.com/3/image/" + key;
		
		string page = fetchImgurUrl(jsonUrl);
		
		if (page.size() > 0)
		{
			// convert to json
			block_allocator allocator(1 << 10);
			json_value* root = toJson(page, allocator);
			if (root)
			{
                for (json_value* it = root->first_child; it; it = it->next_sibling)
				{
					if (!strcmp(it->name, "data") && it->type == JSON_OBJECT)
					{
                        json_value* data = it;
                        
                        for (json_value* it = data->first_child; it; it = it->next_sibling)
                        {
                            if (!strcmp(it->name, "link"))
                            {
                                string link = it->string_value;
                                
                                if (link.size() > 0)
                                {
                                    saveImage(subreddit, options, link);
                                }
                            }
                        }
                    }
                }

            }
        }
	}
	return true;
}

void handleStory(const string& subreddit, const Options& options, vector<HistoryItem>& history, json_value* story)
{
	string id;
	string title;
	string selftext;
	int score;
	bool over_18;
	string thumbnail;
	string url;
	double created_utc;
	string permalink;

	for (json_value* it = story->first_child; it; it = it->next_sibling)
	{
		if (!strcmp(it->name, "id")) 
			id = it->string_value;
		if (!strcmp(it->name, "title")) 
			title = it->string_value;
		if (!strcmp(it->name, "selftext")) 
			selftext = it->string_value;
		if (!strcmp(it->name, "score")) 
			score = it->int_value;
		if (!strcmp(it->name, "over_18")) 
			over_18 = it->int_value ? true : false;
		if (!strcmp(it->name, "thumbnail")) 
			thumbnail = it->string_value;
		if (!strcmp(it->name, "url")) 
			url = it->string_value;
		if (!strcmp(it->name, "created_utc")) 
			created_utc = it->float_value;
		if (!strcmp(it->name, "permalink")) 
			permalink = it->string_value;
	}

	bool inHistory = isInHistory(history, id);

	if (inHistory && !options.openAll)
		return;

	if (isImageUrl(url))
	{
		//saveImage(subreddit, options, url);	
	}
	else if (isImgurUrl(url))
	{
		saveImgurUrl(subreddit, options, url);
	}

	if (!inHistory)
	{
		HistoryItem hi;
		hi.id   = id;
		hi.date = time(NULL);
		history.push_back(hi);
	}
}

bool readSubreddits(const Options& options)
{
	for (int i = 0; i < (int)options.subreddits.size(); i++)
	{
		vector<HistoryItem> history;
		loadHistory(options.subreddits[i], history);

		if (options.clearHistory)
			history.clear();

		// fetch the url from the server
		string server = string("www.reddit.com");
		string path   = string("/r/") + options.subreddits[i] + string("/.json?limit=") + toStr(options.numToOpen);

		string page = fetchUrl(server, path);

		if (page.size() > 0)
		{
			// convert to json
			block_allocator allocator(1 << 10);
			json_value* root = toJson(page, allocator);
			if (root)
			{
				// find the data we want. this is gross.
				for (json_value* it = root->first_child; it; it = it->next_sibling)
				{
					if (!strcmp(it->name, "data") && it->type == JSON_OBJECT)
					{
						json_value* data = it;
						for (json_value* it = data->first_child; it; it = it->next_sibling)
						{
							if (!strcmp(it->name, "children") && it->type == JSON_ARRAY)
							{
								json_value* child = it;
								for (json_value* it = child->first_child; it; it = it->next_sibling)
								{
									json_value* rec = it;
									for (json_value* it = rec->first_child; it; it = it->next_sibling)
									{
										if (!strcmp(it->name, "data"))
											handleStory(options.subreddits[i], options, history, it);
									}						
								}
							}
						}
					}
				}
			}
		}

		if (history.size() || options.clearHistory)
			saveHistory(options.subreddits[i], history);
	}

	return true;
}

void printUsage()
{
	printf("r: Copyright 2012 Roland Rabien\n");
	printf("  usage: rget [-cashv] [-n max] [-o folder] subreddit\n");
	printf("  -c Clear history\n");
	printf("  -a Fetch all (including previously opened)\n");
	printf("  -n Maximum number of links to fetch (default 30)\n");
	printf("  -o Output Folder\n");
	printf("  -s Create folder for subreddit\n");
	printf("  -h Display help\n");
	printf("  -v Display version\n");
}

Options parseOptions(int argc, char* argv[])
{
	Options options;

	ultraopterr = 0;

	int c;	
	while ((c = ultragetopt (argc, argv, "can:o:shv")) != -1)
	{
		switch (c)
		{
			case 'c': options.clearHistory      = true; break;
			case 'a': options.openAll			= true; break;
			case 'n': options.numToOpen			= atoi(ultraoptarg); break;
			case 'o': options.output			= ultraoptarg; break;
			case 's': options.createSubdir      = true; break;
			case 'h': options.displayHelp		= true; break;
			case 'v': options.displayVersion	= true; break;
			case '?':
				if (ultraoptopt == 'c')
					fprintf (stderr, "Option -%c requires an argument.\n", ultraoptopt);
				else if (isprint (ultraoptopt))
					fprintf (stderr, "Unknown option `-%c'.\n", ultraoptopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", ultraoptopt);
				break;
		}
	}

	if (options.numToOpen == 0)
		options.numToOpen = 30;

	for (int index = ultraoptind; index < argc; index++)
		options.subreddits.push_back(argv[index]);

	return options;
}

bool showVersion()
{
	printf("r v1.0.0 by Roland Rabien (figbug@gmail.com) %s.\n", __DATE__);

	return true;
}

int main(int argc, char* argv[])
{
	int res = EXIT_SUCCESS;

#ifdef __APPLE__
	curl_global_init(CURL_GLOBAL_ALL);
#endif

	if (argc == 0)
	{
		printUsage();
		return res;
	}

	Options options = parseOptions(argc, argv);

	if (options.displayVersion)
	{
		showVersion();
		return res;
	}

	if (options.displayHelp || options.subreddits.size() == 0)
	{
		printUsage();
		return res;
	}

	readSubreddits(options);

#ifdef __APPLE__
	curl_global_cleanup();
#endif

	return res;
}

