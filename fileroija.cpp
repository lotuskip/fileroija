#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctime>
#ifndef NO_REGEXP
#include <pcrecpp.h>
#else
#include <cstdlib>
#endif

using namespace std;

const char VERSION[] = "3";
const string nosubs_flag = "-nosub";

vector<string> foundfiles;
set<size_t> last_gen;
#ifndef NO_REGEXP
pcrecpp::RE *re = NULL;
#endif
bool do_subdirs = true;

// Recursively collect the names of regular files in a directory
void go_thru_dir(const char* name)
{
	DIR *dp;
	if((dp = opendir(name)) == NULL)
		return;
    struct dirent *dirp;
	string s = name;
	s += '/';
	while((dirp = readdir(dp)) != NULL)
	{
		if(dirp->d_name[0] == '.' && (dirp->d_name[1] == '\0'
			|| (dirp->d_name[1] == '.' && dirp->d_name[2] == '\0')))
			continue;
		if(dirp->d_type == DT_DIR) // a subdirectory
		{
			if(do_subdirs)
				go_thru_dir((s + dirp->d_name).c_str());
		}
		else if(dirp->d_type == DT_REG)
		{
#ifndef NO_REGEXP
			if(!re || re->FullMatch(dirp->d_name))
#endif
				foundfiles.push_back(s + dirp->d_name);
		}
	}
	closedir(dp);
}


int main(int argc, char* argv[])
{
	struct stat st_file_info;
	if(argc < 2 || stat(argv[1], &st_file_info)
		|| !(S_ISDIR(st_file_info.st_mode)))
	{
		cerr << "No (valid) directory given." << endl;
		return 1;
	}
	if(argc >= 3)
	{
		if(argc == 3 && string(argv[2]) == nosubs_flag)
			do_subdirs = false;
#ifndef NO_REGEXP
		else
		{
			re = new pcrecpp::RE(argv[2], pcrecpp::UTF8());
			if(!re->error().empty())
			{
				cerr << "Error constructing regular expression:" << endl
					<< re->error() << endl;
				return 1;
			}
			if(argc >= 4 && string(argv[3]) == nosubs_flag)
				do_subdirs = false;
		}
#endif
	}
	cerr << "fileroija v. " << VERSION << endl;
	cerr << "dir: " << argv[1] << endl;
#ifndef NO_REGEXP
	cerr << "regexp: " << (re ? argv[2] : "(none)") << endl;
#endif
	cerr << "do subdirs: " << do_subdirs << endl;

	go_thru_dir(argv[1]);

	if(foundfiles.size() <= 1)
	{
		cerr << "Less than 2 files read! Quitting..." << endl;
		return 1;
	}
	cerr << "Read " << foundfiles.size() << " files." << endl
		<< "Enter a number > 0 to produce that many " << endl
		<< "files to stdout. ^D to quit." << endl;
	srandom(time(NULL));

	int in;
	size_t next;
	for(;;)
	{
		if(!(cin >> in))
			break;
		if(in == 0)
		{
			last_gen.clear();
			cerr << "Generation memory cleared!" << endl;
		}
		while(in-- > 0)
		{
			// Collect previously generated numbers so as to not
			// generate same results again too soon:
			do next = random()%foundfiles.size();
			while(last_gen.find(next) != last_gen.end());
			cout << foundfiles[next] << endl;
			last_gen.insert(next);
			// Only collect last_gens until half of the total set
			// has been generated:
			if(last_gen.size() > foundfiles.size()/2)
				last_gen.clear();
		}
	}
	return 0;
}
